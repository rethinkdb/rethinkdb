// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo_traversal.hpp"

#include "geo/exceptions.hpp"
#include "geo/intersection.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/profile.hpp"

rget_geo_cb_t::rget_geo_cb_t(
        geo_io_data_t &&_io,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        const std::vector<ql::transform_variant_t> &_transforms,
        const boost::optional<ql::terminal_variant_t> &_terminal)
    : geo_index_traversal_helper_t(compute_index_grid_keys(_sindex.query_geometry, 12)), // TODO! (don't hard-code 12 here)
      io(std::move(_io)),
      sindex(std::move(_sindex)),
      env(_env),
      // TODO (daniel): Implement lazy geo rgets
      accumulator(_terminal
                  ? ql::make_eager_terminal(*_terminal)
                  : ql::make_to_array()) {
    for (size_t i = 0; i < _transforms.size(); ++i) {
        transformers.push_back(ql::make_op(_transforms[i]));
    }
    guarantee(transformers.size() == _transforms.size());
    disabler.init(new profile::disabler_t(env->trace));
    sampler.init(new profile::sampler_t("Geospatial intersection traversal doc evaluation.",
                                        env->trace));
}

void rget_geo_cb_t::finish() THROWS_ONLY(interrupted_exc_t) {
    // TODO! How does this work?
    //counted_t<val_t> finish_eager(
    //    protob_t<const Backtrace> bt, bool is_grouped)
    //accumulator->finish_eager(&io.response->result);
}

void rget_geo_cb_t::on_candidate(
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

        ql::groups_t data = {{counted_t<const ql::datum_t>(), ql::datums_t{val}}};

        for (auto it = transformers.begin(); it != transformers.end(); ++it) {
            (**it)(env, &data, sindex_val);
        }

        (*accumulator)(env, &data);
    } catch (const ql::exc_t &e) {
        io.response->result = e;
        // TODO! Abort
        return;
    } catch (const geo_exception_t &e) {
        io.response->result = ql::exc_t(ql::base_exc_t::GENERIC, e.what(), NULL);
        // TODO! Abort
        return;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        io.response->result = ql::exc_t(e, NULL);
        // TODO! Abort
        return;
#endif // NDEBUG
    }
}
