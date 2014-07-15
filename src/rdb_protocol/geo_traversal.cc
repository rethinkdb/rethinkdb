// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo_traversal.hpp"

#include "geo/exceptions.hpp"
#include "geo/intersection.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/profile.hpp"

geo_intersecting_cb_t::geo_intersecting_cb_t(
        geo_io_data_t &&_io,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env)
    : geo_index_traversal_helper_t(compute_index_grid_keys(_sindex.query_geometry, 12)), // TODO! (don't hard-code 12 here)
      io(std::move(_io)),
      sindex(std::move(_sindex)),
      env(_env),
      result_acc(ql::datum_t::R_ARRAY) {
    // TODO (daniel): Implement lazy geo rgets, push down transformations and terminals to here
    disabler.init(new profile::disabler_t(env->trace));
    sampler.init(new profile::sampler_t("Geospatial intersection traversal doc evaluation.",
                                        env->trace));
}

void geo_intersecting_cb_t::finish() THROWS_ONLY(interrupted_exc_t) {
    io.response->results = result_acc.to_counted();
}

void geo_intersecting_cb_t::on_candidate(
        const btree_key_t *key,
        const void *value,
        buf_parent_t parent)
        THROWS_ONLY(interrupted_exc_t) {
    sampler->new_sample();

    store_key_t store_key(key);
    store_key_t primary_key(ql::datum_t::extract_primary(store_key));
    // Check if the primary key is in the range of the current slice
    if (!sindex.pkey_range.contains_key(primary_key)) {
        return;
    }

    // Check if this is a duplicate of a previously emitted value.
    // Note that we sacrifice some performance by only storing primary keys
    // of objects that we have actually *emitted* rather than of each object
    // we have post-filtered. We may load the same document multiple times, but
    // reject it each time. The advantage of this is using memory linear to the
    // size of the actual result set, not in the size of the pre-filtered result
    // set.
    // TODO (daniel): That might not be worth it. Consider doing the opposite.
    if (already_processed.count(primary_key) > 0) {
        return;
    }

    lazy_json_t row(static_cast<const rdb_value_t *>(value), parent);
    counted_t<const ql::datum_t> val;
    val = row.get();
    io.slice->stats.pm_keys_read.record();
    io.slice->stats.pm_total_keys_read += 1;

    try {
        // Post-filter the geometry based on an actual intersection test
        // with query_geometry
        counted_t<const ql::datum_t> sindex_val =
            sindex.func->call(env, val)->as_datum();
        if (sindex.multi == sindex_multi_bool_t::MULTI
            && sindex_val->get_type() == ql::datum_t::R_ARRAY) {
            boost::optional<uint64_t> tag = *ql::datum_t::extract_tag(store_key);
            guarantee(tag);
            sindex_val = sindex_val->get(*tag, ql::NOTHROW);
            guarantee(sindex_val);
        }
        // TODO! Support compound indexes
        // TODO (daniel): This is a little inefficient because we re-parse
        //   the query_geometry for each test.
        if (!geo_does_intersect(sindex.query_geometry, sindex_val)) {
            return;
        }

        // Make sure we don't emit this value again.
        // TODO! Check the size of this.
        already_processed.insert(primary_key);

        result_acc.add(val);
    } catch (const ql::exc_t &e) {
        io.response->error = e;
        // TODO! Abort
        return;
    } catch (const geo_exception_t &e) {
        io.response->error = ql::exc_t(ql::base_exc_t::GENERIC, e.what(), NULL);
        // TODO! Abort
        return;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        io.response->error = ql::exc_t(e, NULL);
        // TODO! Abort
        return;
#endif // NDEBUG
    }
}
