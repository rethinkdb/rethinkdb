// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo_traversal.hpp"

#include <cmath>

#include "errors.hpp"
#include <boost/variant/get.hpp>

#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/intersection.hpp"
#include "rdb_protocol/geo/lat_lon_types.hpp"
#include "rdb_protocol/geo/primitives.hpp"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2latlng.h"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/profile.hpp"

// How many primary keys to keep in memory for avoiding processing the same
// document multiple times (efficiency optimization).
const size_t MAX_PROCESSED_SET_SIZE = 10000;

// How many grid cells to use for querying a secondary index.
// It typically makes sense to use more grid cells (i.e. finer covering) here
// than it does for inserting data into a geo index, since there is no disk
// overhead involved here and a finer grid avoids unnecessary post-filtering.
const int QUERYING_GOAL_GRID_CELLS = GEO_INDEX_GOAL_GRID_CELLS * 2;

// The radius used for the first batch of a get_nearest traversal.
// As a fraction of the equator's radius.
// The current value is equivalent to a radius of 10m on earth.
const double NEAREST_INITIAL_RADIUS_FRACTION = 10.0 / 6378137.0;

// By which factor to increase the radius between nearest batches (at maximum)
const double NEAREST_MAX_GROWTH_FACTOR = 2.0;

// The desired result size of each nearest batch
const double NEAREST_GOAL_BATCH_SIZE = 100.0;

// Number of vertices used in get_nearest traversal for approximating the
// current search range through a polygon.
const unsigned int NEAREST_NUM_VERTICES = 8;


/* ----------- geo_intersecting_cb_t -----------*/
geo_intersecting_cb_t::geo_intersecting_cb_t(
        btree_slice_t *_slice,
        geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        std::set<store_key_t> *_distinct_emitted_in_out)
    : geo_index_traversal_helper_t(),
      slice(_slice),
      sindex(std::move(_sindex)),
      env(_env),
      distinct_emitted(_distinct_emitted_in_out) {
    guarantee(distinct_emitted != NULL);
    disabler.init(new profile::disabler_t(env->trace));
    sampler.init(new profile::sampler_t("Geospatial intersection traversal.",
                                        env->trace));
}

void geo_intersecting_cb_t::init_query(const counted_t<const ql::datum_t> &_query_geometry) {
    query_geometry = _query_geometry;
    geo_index_traversal_helper_t::init_query(
        compute_index_grid_keys(_query_geometry, QUERYING_GOAL_GRID_CELLS));
}

void geo_intersecting_cb_t::on_candidate(
        const btree_key_t *key,
        const void *value,
        buf_parent_t parent,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    guarantee(query_geometry.has());
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
    counted_t<const ql::datum_t> val = row.get();
    slice->stats.pm_keys_read.record();
    slice->stats.pm_total_keys_read += 1;

    // row.get() might have blocked, and another coroutine could have found the
    // object in the meantime. Re-check distinct_emitted and then disallow
    // blocking for the rest of this function.
    if (distinct_emitted->count(primary_key) > 0) {
        return;
    }
    ASSERT_NO_CORO_WAITING;

    try {
        // Post-filter the geometry based on an actual intersection test
        // with query_geometry
        ql::env_t sindex_env(env->interruptor, sindex.func_reql_version);
        counted_t<const ql::datum_t> sindex_val =
            sindex.func->call(&sindex_env, val)->as_datum();
        if (sindex.multi == sindex_multi_bool_t::MULTI
            && sindex_val->get_type() == ql::datum_t::R_ARRAY) {
            boost::optional<uint64_t> tag = *ql::datum_t::extract_tag(store_key);
            guarantee(tag);
            sindex_val = sindex_val->get(*tag, ql::NOTHROW);
            guarantee(sindex_val);
        }
        // TODO (daniel): This is a little inefficient because we re-parse
        //   the query_geometry for each test.
        if (geo_does_intersect(query_geometry, sindex_val)
            && post_filter(sindex_val, val)) {
            if (distinct_emitted->size() > env->limits().array_size_limit()) {
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
    } catch (const ql::base_exc_t &e) {
        emit_error(ql::exc_t(e, NULL));
        abort_traversal();
        return;
    }
}


/* ----------- collect_all_geo_intersecting_cb_t -----------*/
collect_all_geo_intersecting_cb_t::collect_all_geo_intersecting_cb_t(
        btree_slice_t *_slice,
        geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        const counted_t<const ql::datum_t> &_query_geometry) :
    geo_intersecting_cb_t(_slice, std::move(_sindex), _env, &distinct_emitted),
    result_acc(_env->limits()) {
    init_query(_query_geometry);
    // TODO (daniel): Consider making the traversal resumable, so we can
    //    do it lazily.
    //    Even if we cannot do that, it would be good if we could push down
    //    transformations and terminals into the traversal so their work can
    //    be distributed.
}

void collect_all_geo_intersecting_cb_t::finish(
        intersecting_geo_read_response_t *resp_out) {
    guarantee(resp_out != NULL);
    if (error) {
        resp_out->results_or_error = error.get();
    } else {
        resp_out->results_or_error = std::move(result_acc).to_counted();
    }
}

bool collect_all_geo_intersecting_cb_t::post_filter(
        UNUSED const counted_t<const ql::datum_t> &sindex_val,
        UNUSED const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) {
    return true;
}

void collect_all_geo_intersecting_cb_t::emit_result(
        UNUSED const counted_t<const ql::datum_t> &sindex_val,
        const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) {
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
    previous_size(0),
    processed_inradius(0.0),
    current_inradius(std::min(_max_radius,
            NEAREST_INITIAL_RADIUS_FRACTION * _reference_ellipsoid.equator_radius())),
    center(_center),
    max_results(_max_results),
    max_radius(_max_radius),
    reference_ellipsoid(_reference_ellipsoid) { }

done_traversing_t nearest_traversal_state_t::proceed_to_next_batch() {
    // Estimate the result density based on the previous batch
    const size_t previous_num_results = distinct_emitted.size() - previous_size;
    const double previous_area = M_PI *
        (current_inradius * current_inradius - processed_inradius * processed_inradius);
    const double previous_density =
        static_cast<double>(previous_num_results) / previous_area;

    processed_inradius = current_inradius;
    previous_size = distinct_emitted.size();

    // Adapt the radius based on the density of the previous batch.
    // Solve for current_inradius: NEAREST_GOAL_BATCH_SIZE =
    //   M_PI * (current_inradius^2 - processed_inradius^2) * previous_density
    // <=> current_inradius^2 =
    //   NEAREST_GOAL_BATCH_SIZE / M_PI / previous_density + processed_inradius^2
    if (previous_density != 0.0) {
        current_inradius = sqrt((NEAREST_GOAL_BATCH_SIZE / M_PI / previous_density)
                                + (processed_inradius * processed_inradius));
    } else {
        current_inradius = processed_inradius * NEAREST_MAX_GROWTH_FACTOR;
    }

    // In addition, put a limit on how fast the radius can grow, so we don't
    // increase it to ridiculous sizes just to encounter some cluster of results
    // later.
    current_inradius =
        std::min(current_inradius,
                 std::min(current_inradius * NEAREST_MAX_GROWTH_FACTOR, max_radius));

    if (processed_inradius >= max_radius || distinct_emitted.size() >= max_results) {
        return done_traversing_t::YES;
    } else {
        return done_traversing_t::NO;
    }
}

nearest_traversal_cb_t::nearest_traversal_cb_t(
        btree_slice_t *_slice,
        geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        nearest_traversal_state_t *_state) :
    geo_intersecting_cb_t(_slice, std::move(_sindex), _env, &_state->distinct_emitted),
    state(_state) {
    init_query_geometry();
}

void nearest_traversal_cb_t::init_query_geometry() {
    /*
    Note that S2 performs intersection tests on a sphere, while our distance
    metric is defined on an ellipsoid.

    The following must hold for this to work:

     A polygon constructed through build_polygon_with_exradius_at_most(center, r),
     must *not* intersect (using spherical geometry) with any point x that has
     a distance (on any given oblate ellipsoid) dist(center, x) > r.

     Similarly, a polygon constructed through
     build_polygon_with_inradius_at_least(center, r) must intersect (using
     spherical geometry) with *every* point x that has a distance (on any given
     ellipsoid) dist(center, x) <= r.

    There is a unit test in geo_primitives.cc to verify this numerically.
    */

    try {
        // 1. Construct a shape with an exradius of no more than state->processed_inradius.
        //    This is what we don't have to process again.
        std::vector<lat_lon_line_t> holes;
        if (state->processed_inradius > 0.0) {
            holes.push_back(build_polygon_with_exradius_at_most(
                state->center, state->processed_inradius,
                NEAREST_NUM_VERTICES, state->reference_ellipsoid));
        }

        // 2. Construct the outer shell, a shape with an inradius of at least
        //    state->current_inradius.
        lat_lon_line_t shell = build_polygon_with_inradius_at_least(
            state->center, state->current_inradius,
            NEAREST_NUM_VERTICES, state->reference_ellipsoid);

        counted_t<const ql::datum_t> query_geometry =
            construct_geo_polygon(shell, holes, ql::configured_limits_t::unlimited);
        init_query(query_geometry);
    } catch (const geo_range_exception_t &e) {
        // The radius has become too large for constructing the query geometry.
        // Abort.
        throw geo_range_exception_t(strprintf(
            "The distance has become too large for continuing the indexed nearest "
            "traversal.  Consider specifying a smaller `max_dist` parameter.  "
            "(%s)", e.what()));
    } catch (const geo_exception_t &e) {
        crash("Geo exception thrown while initializing nearest query geometry: %s",
              e.what());
    }
}

bool nearest_traversal_cb_t::post_filter(
        const counted_t<const ql::datum_t> &sindex_val,
        UNUSED const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) {

    // Filter out results that are outside of the current inradius
    const S2Point s2center =
        S2LatLng::FromDegrees(state->center.first, state->center.second).ToPoint();
    const double dist = geodesic_distance(s2center, sindex_val, state->reference_ellipsoid);
    return dist <= state->current_inradius;
}

void nearest_traversal_cb_t::emit_result(
        const counted_t<const ql::datum_t> &sindex_val,
        const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) {
    // TODO (daniel): Could we avoid re-computing the distance? We have already
    //   done it in post_filter().
    const S2Point s2center =
        S2LatLng::FromDegrees(state->center.first, state->center.second).ToPoint();
    const double dist = geodesic_distance(s2center, sindex_val, state->reference_ellipsoid);
    result_acc.push_back(std::make_pair(dist, val));
}

void nearest_traversal_cb_t::emit_error(
        const ql::exc_t &_error)
        THROWS_ONLY(interrupted_exc_t) {
    error = _error;
}

bool nearest_pairs_less(
        const std::pair<double, counted_t<const ql::datum_t> > &p1,
        const std::pair<double, counted_t<const ql::datum_t> > &p2) {
    // We only care about the distance, don't compare the actual data.
    return p1.first < p2.first;
}

void nearest_traversal_cb_t::finish(
        nearest_geo_read_response_t *resp_out) {
    guarantee(resp_out != NULL);
    if (error) {
        resp_out->results_or_error = error.get();
    } else {
        std::sort(result_acc.begin(), result_acc.end(), &nearest_pairs_less);
        resp_out->results_or_error = std::move(result_acc);
    }
}
