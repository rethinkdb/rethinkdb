// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <algorithm>

#include "debug.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/intersection.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/geo/primitives.hpp"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2latlng.h"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/datum.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

using geo::S2LatLng;
using geo::S2Point;
using geo::S2Polygon;
using ql::datum_t;

namespace unittest {

void test_in_ex_radius(
        const lon_lat_point_t c, double r, const ellipsoid_spec_t &e, rng_t *rng) {
    // We first generate the polygons we want to test against.
    const datum_t outer_polygon =
        construct_geo_polygon(build_polygon_with_inradius_at_least(c, r, 8, e),
                              ql::configured_limits_t());
    const datum_t inner_polygon =
        construct_geo_polygon(build_polygon_with_exradius_at_most(c, r, 8, e),
                              ql::configured_limits_t());
    scoped_ptr_t<S2Polygon> outer_s2polygon = to_s2polygon(outer_polygon);
    scoped_ptr_t<S2Polygon> inner_s2polygon = to_s2polygon(inner_polygon);

    // We then generate points at 1000 random azimuths exactly at distance r
    // around c, and check that they
    // a) do intersect with outer_polygon
    // b) do not intersect with inner_polygon
    for (int i = 0; i < 1000; ++i) {
        double azimuth = rng->randdouble() * 360.0 - 180.0;
        lon_lat_point_t test_point =
            geodesic_point_at_dist(c, r, azimuth, e);
        S2Point test_s2point =
            S2LatLng::FromDegrees(test_point.latitude, test_point.longitude).ToPoint();
        ASSERT_TRUE(geo_does_intersect(test_s2point, *outer_s2polygon));
        ASSERT_FALSE(geo_does_intersect(test_s2point, *inner_s2polygon));
    }
}

void test_in_ex_radius(double r, const ellipsoid_spec_t &e, rng_t *rng) {
    // Test different positions:
    // At a pole
    test_in_ex_radius(lon_lat_point_t(0.0, 90.0), r, e, rng);
    // On the equator
    test_in_ex_radius(lon_lat_point_t(0.0, 0.0), r, e, rng);
    // At 10 random positions
    for (int i = 0; i < 10; ++i) {
        double lat = rng->randdouble() * 180.0 - 90.0;
        double lon = rng->randdouble() * 360.0 - 180.0;
        test_in_ex_radius(lon_lat_point_t(lon, lat), r, e, rng);
    }
}

void test_in_ex_radius(const ellipsoid_spec_t e, rng_t *rng) {
    const double minor_radius = std::min(e.equator_radius(), e.poles_radius());

    // Test different radii:
    // A very small one, 1/10,000,000th of the minor radius
    test_in_ex_radius(minor_radius * 0.0000001, e, rng);
    // A medium one, 1/100th of the minor radius
    test_in_ex_radius(minor_radius * 0.01, e, rng);
    // A large one
    test_in_ex_radius(minor_radius * 0.5, e, rng);
    // A very large one
    try {
        test_in_ex_radius(minor_radius, e, rng);
    } catch (const geo_range_exception_t &) {
        // Ignore. This one is too large for the more extreme ellipsoids.
    }
}

// Verifies that the constraints described in nearest_traversal_cb_t::init_query_geometry()
// hold
TPTEST(GeoPrimitives, InExRadiusTest) {
    // To reproduce a known failure: initialize the rng seed manually.
    const int rng_seed = randint(INT_MAX);
    debugf("Using RNG seed %i\n", rng_seed);
    rng_t rng(rng_seed);
    try {
        // This one is easy. Just to verify that things work in general.
        test_in_ex_radius(UNIT_SPHERE, &rng);
        // Very relevant, but still almost spherical. Test on earth geometry
        test_in_ex_radius(WGS84_ELLIPSOID, &rng);
        // Ellipsoids with very significant flattening
        // TODO (daniel): Something currently becomes unstable at a small distances
        //   when the flattening is too large (from ~0.5). What and why?
        test_in_ex_radius(ellipsoid_spec_t(1.0, 0.4), &rng);
        // ... and stretching (though this wouldn't usually be written that way)
        test_in_ex_radius(ellipsoid_spec_t(1.0, -0.5), &rng);
    } catch (const geo_exception_t &e) {
        ADD_FAILURE() << "Caught a geo exception: " << e.what();
    }
}

}   /* namespace unittest */

