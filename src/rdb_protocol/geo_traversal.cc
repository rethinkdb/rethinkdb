// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo_traversal.hpp"

#include "errors.hpp"
#include <boost/variant/get.hpp>

#include "geo/exceptions.hpp"
#include "geo/intersection.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/profile.hpp"

// How many primary keys to keep in memory for avoiding processing the same
// document multiple times (efficiency optimization).
const static size_t MAX_PROCESSED_SET_SIZE = 10000;

geo_intersecting_cb_t::geo_intersecting_cb_t(
        geo_io_data_t &&_io,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        std::set<store_key_t> *_distinct_emitted_in_out)
    : geo_index_traversal_helper_t(
          compute_index_grid_keys(_sindex.query_geometry, GEO_INDEX_GOAL_GRID_CELLS)),
      io(std::move(_io)),
      sindex(std::move(_sindex)),
      env(_env),
      result_acc(ql::datum_t::R_ARRAY),
      distinct_emitted(_distinct_emitted_in_out) {
    // TODO (daniel): Implement lazy geo rgets, push down transformations and terminals to here
    guarantee(distinct_emitted != NULL);
    disabler.init(new profile::disabler_t(env->trace));
    sampler.init(new profile::sampler_t("Geospatial intersection traversal doc evaluation.",
                                        env->trace));
}

void geo_intersecting_cb_t::finish() THROWS_ONLY(interrupted_exc_t) {
    if (boost::get<ql::exc_t>(&io.response->results_or_error) == NULL) {
        io.response->results_or_error = result_acc.to_counted();
    }
}

void geo_intersecting_cb_t::on_candidate(
        const btree_key_t *key,
        const void *value,
        buf_parent_t parent,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    sampler->new_sample();

    store_key_t store_key(key);
    store_key_t primary_key(ql::datum_t::extract_primary(store_key));
    // Check if the primary key is in the range of the current slice
    if (!sindex.pkey_range.contains_key(primary_key)) {
        return;
    }

    // Check if this document has already been processed (lower bound).
    if (already_processed.count(primary_key) > 0) {
        return;
    }
    // Check if this document has already been emitted.
    if (distinct_emitted->count(primary_key) > 0) {
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
        // TODO (daniel): This is a little inefficient because we re-parse
        //   the query_geometry for each test.
        if (geo_does_intersect(sindex.query_geometry, sindex_val)) {
            if (distinct_emitted->size() > ql::array_size_limit()) {
                io.response->results_or_error = ql::exc_t(ql::base_exc_t::GENERIC,
                        "Result size limit exceeded (array size).", NULL);;
                abort_traversal();
                return;
            }
            distinct_emitted->insert(primary_key);
            result_acc.add(val);
        } else {
            // Mark the document as processed so we don't have to load it again.
            // This is relevant only for polygons and lines, since those can be
            // encountered multiple times in the index.
            if (already_processed.size() < MAX_PROCESSED_SET_SIZE
                && sindex_val->get("type")->as_str().to_std() != "Point") {
                already_processed.insert(primary_key);
            }
        }
    } catch (const ql::exc_t &e) {
        io.response->results_or_error = e;
        abort_traversal();
        return;
    } catch (const geo_exception_t &e) {
        io.response->results_or_error =
            ql::exc_t(ql::base_exc_t::GENERIC, e.what(), NULL);
        abort_traversal();
        return;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        io.response->results_or_error = ql::exc_t(e, NULL);
        abort_traversal();
        return;
#endif // NDEBUG
    }
}
