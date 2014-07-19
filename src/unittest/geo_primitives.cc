// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <algorithm>

#include "debug.hpp"
#include "geo/distances.hpp"
#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/geojson.hpp"
#include "geo/intersection.hpp"
#include "geo/lat_lon_types.hpp"
#include "geo/primitives.hpp"
#include "geo/s2/s2.h"
#include "geo/s2/s2latlng.h"
#include "geo/s2/s2polygon.h"
#include "rdb_protocol/datum.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

using ql::datum_t;

namespace unittest {

void test_in_ex_radius(const lat_lon_point_t c, double r, const ellipsoid_spec_t &e) {
    // We first generate the polygons we want to test against.
    const counted_t<const datum_t> outer_polygon =
        construct_geo_polygon(build_polygon_with_inradius_at_least(c, r, e));
    const counted_t<const datum_t> inner_polygon =
        construct_geo_polygon(build_polygon_with_exradius_at_most(c, r, e));
    scoped_ptr_t<S2Polygon> outer_s2polygon = to_s2polygon(outer_polygon);
    debugf("Got first\n");
    scoped_ptr_t<S2Polygon> inner_s2polygon = to_s2polygon(inner_polygon);
    debugf("Got second\n");

    // We then generate points at 1000 random azimuths exactly at distance r
    // around c, and check that they
    // a) do intersect with outer_polygon
    // b) do not intersect with inner_polygon
    for (int i = 0; i < 1000; ++i) {
        double azimuth = randdouble() * 360.0 - 180.0;
        lat_lon_point_t test_point =
            geodesic_point_at_dist(c, r, azimuth, e);
        S2Point test_s2point =
            S2LatLng::FromDegrees(test_point.first, test_point.second).ToPoint();
        ASSERT_TRUE(geo_does_intersect(test_s2point, *outer_s2polygon));
        ASSERT_FALSE(geo_does_intersect(test_s2point, *inner_s2polygon));
    }
}

void test_in_ex_radius(double r, const ellipsoid_spec_t &e) {
    // Test different positions:
    // At a pole
    test_in_ex_radius(lat_lon_point_t(90.0, 0.0), r, e);
    // On the equator
    test_in_ex_radius(lat_lon_point_t(0.0, 0.0), r, e);
    // At 5 random positions
    for (int i = 0; i < 5; ++i) {
        double lat = randdouble() * 180.0 - 90.0;
        double lon = randdouble() * 360.0 - 180.0;
        test_in_ex_radius(lat_lon_point_t(lat, lon), r, e);
    }
}

void test_in_ex_radius(const ellipsoid_spec_t e) {
    const double minor_radius = std::min(e.equator_radius(), e.poles_radius());

    // Test different radii:
    // A smallish one, 1/100th of the minor radius
    test_in_ex_radius(minor_radius * 0.01, e);
    // A large one
    // TODO! Make this as large as possible
    test_in_ex_radius(minor_radius * 0.5, e);
}

// Verifies that the constraints described in nearest_traversal_cb_t::init_query_geometry()
// hold
TPTEST(GeoPrimitives, InExRadiusTest) {
    try {
        // This one is easy. Just to verify that things work in general.
        test_in_ex_radius(UNIT_SPHERE);
        // Very relevant, but still almost spherical. Test on earth geometry
        test_in_ex_radius(WGS84_ELLIPSOID);
        // Ellipsoids with very significant flattening
        test_in_ex_radius(ellipsoid_spec_t(1.0, 0.5));
        // ... and stretching (though this wouldn't usually be written that way)
        test_in_ex_radius(ellipsoid_spec_t(1.0, -1));
        // Ellipsoids with extreme flattening
        test_in_ex_radius(ellipsoid_spec_t(1.0, 0.95));
    } catch (const geo_exception_t &e) {
        debugf("Caught a geo exception: %s\n", e.what());
        FAIL();
    }
}

}   /* namespace unittest */

