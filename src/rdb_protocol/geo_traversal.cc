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
const size_t MAX_PROCESSED_SET_SIZE = 10000;

// The radius used for the first batch of a get_nearest traversal.
// As a fraction of the equator's radius.
// The current value is equivalent to a radius of 10m on earth.
const double NEAREST_INITIAL_RADIUS_FRACTION = 10.0 / 6378137.0;


/* ----------- geo_intersecting_cb_t -----------*/
geo_intersecting_cb_t::geo_intersecting_cb_t(
        btree_slice_t *_slice,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        std::set<store_key_t> *_distinct_emitted_in_out)
    : geo_index_traversal_helper_t(
          compute_index_grid_keys(_sindex.query_geometry, GEO_INDEX_GOAL_GRID_CELLS)),
      slice(_slice),
      sindex(std::move(_sindex)),
      env(_env),
      distinct_emitted(_distinct_emitted_in_out) {
    guarantee(distinct_emitted != NULL);
    disabler.init(new profile::disabler_t(env->trace));
    sampler.init(new profile::sampler_t("Geospatial intersection traversal.",
                                        env->trace));
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
    slice->stats.pm_keys_read.record();
    slice->stats.pm_total_keys_read += 1;

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
        if (geo_does_intersect(sindex.query_geometry, sindex_val)
            && post_filter(sindex_val, val)) {
            if (distinct_emitted->size() > ql::array_size_limit()) {
                emit_error(ql::exc_t(ql::base_exc_t::GENERIC,
                        "Result size limit exceeded (array size).", NULL));
                abort_traversal();
                return;
            }
            distinct_emitted->insert(primary_key);
            emit_result(sindex_val, val);
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
        emit_error(e);
        abort_traversal();
        return;
    } catch (const geo_exception_t &e) {
        emit_error(ql::exc_t(ql::base_exc_t::GENERIC, e.what(), NULL));
        abort_traversal();
        return;
    } catch (const ql::datum_exc_t &e) {
#ifndef NDEBUG
        unreachable();
#else
        emit_error(ql::exc_t(e, NULL));
        abort_traversal();
        return;
#endif // NDEBUG
    }
}


/* ----------- collect_all_geo_intersecting_cb_t -----------*/
collect_all_geo_intersecting_cb_t::collect_all_geo_intersecting_cb_t(
        btree_slice_t *_slice,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env) :
    geo_intersecting_cb_t(_slice, std::move(_sindex), _env, &distinct_emitted),
    result_acc(ql::datum_t::R_ARRAY) {
    // TODO (daniel): Implement lazy geo rgets,
    //    ...push down transformations and terminals to the traversal
}

void collect_all_geo_intersecting_cb_t::finish(
        intersecting_geo_read_response_t *resp_out) {
    guarantee(resp_out != NULL);
    if (error) {
        resp_out->results_or_error = error.get();
    } else {
        resp_out->results_or_error = result_acc.to_counted();
    }
}

bool collect_all_geo_intersecting_cb_t::post_filter(
        UNUSED const counted_t<const ql::datum_t> &sindex_val,
        UNUSED const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t) {
    return true;
}

void collect_all_geo_intersecting_cb_t::emit_result(
        UNUSED const counted_t<const ql::datum_t> &sindex_val,
        const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t) {
    result_acc.add(val);
}

void collect_all_geo_intersecting_cb_t::emit_error(
        const ql::exc_t &_error)
        THROWS_ONLY(interrupted_exc_t) {
    error = _error;
}


/* ----------- nearest traversal -----------*/
nearest_traversal_state_t::nearest_traversal_state_t(
        const lat_lon_point_t &_center,
        uint64_t _max_results,
        double _max_radius,
        const ellipsoid_spec_t &_reference_ellipsoid) :
    processed_inradius(0.0),
    center(_center),
    max_results(_max_results),
    max_radius(_max_radius),
    reference_ellipsoid(_reference_ellipsoid) { }

// TODO!
#if 0
done_traversing_t compute_next_nearest_batch(
        superblock_t *superblock,
        geo_io_data_t io,
        geo_sindex_data_t sindex,
        ql::env_t *env,
        nearest_traversal_state_t *state_in_out) {
    guarantee(state_in_out != NULL);
    // Limit the radius to the min radius of the ellipsoid.
    // We could allow twice of that, but this is a reasonable safety
    // margin to avoid any potential issues.
    // TODO! Check that this is enough. Verify that we could use twice of that.
    const double radius_limit =
        std::min(state_in_out->reference_ellipsoid.equator_radius(),
                 state_in_out->reference_ellipsoid.poles_radius());
    // TODO! Test what happens if a point lies *exactly* on the
    // circle boundary. In fact write a unit test for that if possible.

    // Construct the query donut
    state_in_out->inner_circle = state_in_out->outer_circle;
    state_in_out->outer_circle = build_circle(
        state_in_out->center, state_in_out->next_radius,
        NEAREST_NUM_CIRCLE_VERTICES, state_in_out->reference_ellipsoid);
    counted_t<const ql::datum_t> query_polygon;
    if (state_in_out->inner_circle.empty()) {
        // This is the first batch. Just compute a ball.
        query_polygon = construct_geo_polygon(state_in_out->outer_circle);
    } else {
        query_polygon = construct_geo_polygon(state_in_out->outer_circle,
                                              {state_in_out->inner_circle});
    }

    // TODO! This way of getting results is very hacky!
    //   Refactor.
    intersecting_geo_read_response_t *intersection_resp = io.response;
    geo_intersecting_cb_t callback(
        std::move(io),
        std::move(sindex),
        env,
        &state_in_out->distinct_emitted);
    btree_parallel_traversal(
        superblock, &callback,
        env->interruptor,
        release_superblock_t::KEEP);
    callback.finish();

    if (intersection_resp->error) {
        return done_traversing_t::YES;
    }

// TODO!!!
// This whole idea doesn't work. Why not?
// We have to post-filter the result to ignore things that are outside
// of the incircle.
// Otherwise we might get the wrong order because we emit things before we have
// seen other ones that are actually closer.
// Here is how we can measure the actual incircle:
//  1. Orientate circle such that it has a "flat" edge in all of north/south/
//     east/west.
//  2. Calculate the distance between the centers of those edges. Take their min.
// TODO!
// Also refactor the whole traversal cb. a) allow passing in a post-filter (so we can filter by distance < inradius)
//    b) Make the output format better. Maybe screw lazy traversal for now.
//
// So here is the plan:
//      Create circles in a way where the inradius is easy to compute.
//      That gives us the *actual* outside radius of the donut, and allows
//      us to create one that is no larger than the previous incircle
//      for the next round's donut's inside.
//      Post filter based on the inradius during the traversal, so we don't
//      skip anything.
//

    // Grab the results for this donut and sort them.


    // TODO! This handling of radius_limit might be confusing.
    // We don't actually guarantee covering radius_limit, since the incircle
    // of our circle approximation is smaller.
    if (state_in_out->next_radius >= radius_limit) {
        return done_traversing_t::YES;
    } else {
        // Use a larger radius for the next batch
        // TODO! I'm not really happy with this exponential progression.
        //   Do something smarter.
        state_in_out->next_radius += state_in_out->next_radius;
        return done_traversing_t::NO;
    }
}
#endif