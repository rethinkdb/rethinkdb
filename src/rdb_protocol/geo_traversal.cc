// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo_traversal.hpp"

#include "errors.hpp"
#include <boost/variant/get.hpp>

#include "geo/distances.hpp"
#include "geo/exceptions.hpp"
#include "geo/geojson.hpp"
#include "geo/intersection.hpp"
#include "geo/lat_lon_types.hpp"
#include "geo/s2/s2.h"
#include "geo/s2/s2latlng.h"
#include "rdb_protocol/batching.hpp"
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


/* ----------- geo_intersecting_cb_t -----------*/
geo_intersecting_cb_t::geo_intersecting_cb_t(
        btree_slice_t *_slice,
        const geo_sindex_data_t &&_sindex,
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
        if (geo_does_intersect(query_geometry, sindex_val)
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
        ql::env_t *_env,
        const counted_t<const ql::datum_t> &_query_geometry) :
    geo_intersecting_cb_t(_slice, std::move(_sindex), _env, &distinct_emitted),
    result_acc(ql::datum_t::R_ARRAY) {
    init_query(_query_geometry);
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

done_traversing_t nearest_traversal_state_t::proceed_to_next_batch() {
    processed_inradius = current_inradius;
    // TODO! Do something smarter
    current_inradius *= 1.5;

    if (processed_inradius >= max_radius || distinct_emitted.size() >= max_results) {
        return done_traversing_t::YES;
    } else {
        return done_traversing_t::NO;
    }
}

nearest_traversal_cb_t::nearest_traversal_cb_t(
        btree_slice_t *_slice,
        const geo_sindex_data_t &&_sindex,
        ql::env_t *_env,
        nearest_traversal_state_t *_state) :
    geo_intersecting_cb_t(_slice, std::move(_sindex), _env, &_state->distinct_emitted),
    state(_state) {
    init_query_geometry();
}

void nearest_traversal_cb_t::init_query_geometry() {
    // TODO! Ensure that covering semantics assume a closed polygon

    // 1. Construct a shape with an exradius of no more than state->processed_inradius.
    //    This is what we don't have to process again.
    std::vector<lat_lon_line_t> holes;
    if (state->processed_inradius > 0.0) {
        holes.push_back(construct_hole());
    }

    // 2. Construct the outer shell, a shape with an inradius of at least
    //    state->current_inradius.
    lat_lon_line_t shell = construct_shell();

    counted_t<const ql::datum_t> query_geometry = construct_geo_polygon(shell, holes);
    init_query(query_geometry);
}

lat_lon_line_t nearest_traversal_cb_t::construct_hole() const {
    guarantee(state->processed_inradius > 0.0);
    // TODO! Do we have to use the min of both axes as the spherical exradius?
    //       I think not. But double-check.

    // Make the hole slightly shorter, just to make sure we don't miss anything
    // due to limited numeric precision and such.
    const double leeway_factor = 0.99;
    double r = state->processed_inradius * leeway_factor;

    // TODO! Use a better shape than a rectangle
    lat_lon_line_t result;
    result.reserve(5);
    result.push_back(geodesic_point_at_dist(
        state->center, r, 0, state->reference_ellipsoid));
    result.push_back(geodesic_point_at_dist(
        state->center, r, 180, state->reference_ellipsoid));
    result.push_back(geodesic_point_at_dist(
        state->center, r, -90, state->reference_ellipsoid));
    result.push_back(geodesic_point_at_dist(
        state->center, r, 90, state->reference_ellipsoid));
    result.push_back(result[0]);

    return result;
}

lat_lon_line_t nearest_traversal_cb_t::construct_shell() const {
    guarantee(state->current_inradius > 0.0);
    // TODO! Do we have to use the max of both axes as the spherical inradius?

    // Make the shell slightly larger, just to make sure we don't miss anything
    // due to limited numeric precision and such.
    const double leeway_factor = 1.01;
    double r = state->current_inradius * leeway_factor;

    // Here's a trick to construct a rectangle with the given inradius:
    // We first compute points that are in the center of each of the
    // rectangle's edges. Then we compute the vertices from those.

    // TODO! Use a better shape than a rectangle
    // Assume azimuth 0 is north, 90 is east
    const lat_lon_point_t north = geodesic_point_at_dist(
        state->center, r, 0, state->reference_ellipsoid);
    const lat_lon_point_t south = geodesic_point_at_dist(
        state->center, r, 180, state->reference_ellipsoid);
    const lat_lon_point_t west = geodesic_point_at_dist(
        state->center, r, -90, state->reference_ellipsoid);
    const lat_lon_point_t east = geodesic_point_at_dist(
        state->center, r, 90, state->reference_ellipsoid);
    // Note that min is not necessarily smaller than max here, since we have
    // modulo arithmetics.
    const double max_lat = north.first;
    const double min_lat = south.first;
    const double max_lon = east.second;
    const double min_lon = west.second;

    lat_lon_line_t result;
    result.reserve(5);
    result.push_back(lat_lon_point_t(max_lat, max_lon));
    result.push_back(lat_lon_point_t(max_lat, min_lon));
    result.push_back(lat_lon_point_t(min_lat, min_lon));
    result.push_back(lat_lon_point_t(min_lat, max_lon));
    result.push_back(lat_lon_point_t(max_lat, max_lon));

    return result;
}

bool nearest_traversal_cb_t::post_filter(
        const counted_t<const ql::datum_t> &sindex_val,
        UNUSED const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t) {

    // Filter out results that are outside of the current inradius
    const S2Point s2center =
        S2LatLng::FromDegrees(state->center.first, state->center.second).ToPoint();
    const double dist = geodesic_distance(s2center, sindex_val, state->reference_ellipsoid);
    return dist <= state->current_inradius;
}

void nearest_traversal_cb_t::emit_result(
        const counted_t<const ql::datum_t> &sindex_val,
        const counted_t<const ql::datum_t> &val)
        THROWS_ONLY(interrupted_exc_t) {
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
        intersecting_geo_read_response_t *resp_out) {
    guarantee(resp_out != NULL);
    if (error) {
        resp_out->results_or_error = error.get();
    } else {
        std::sort(result_acc.begin(), result_acc.end(), &nearest_pairs_less);
        std::vector<counted_t<const ql::datum_t> > result_vec;
        result_vec.reserve(result_acc.size());
        for (size_t i = 0; i < result_acc.size(); ++i) {
            result_vec.push_back(result_acc[i].second);
        }
        resp_out->results_or_error = make_counted<const ql::datum_t>(std::move(result_vec));
    }
}
