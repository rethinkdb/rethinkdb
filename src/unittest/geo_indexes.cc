// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/keys.hpp"
#include "clustering/reactor/namespace_interface.hpp"
#include "concurrency/fifo_checker.hpp"
#include "containers/counted.hpp"
#include "debug.hpp"
#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/geojson.hpp"
#include "geo/lat_lon_types.hpp"
#include "geo/primitives.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/protocol.hpp"
#include "stl_utils.hpp"
#include "unittest/rdb_protocol.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

using ql::datum_t;

namespace unittest {

counted_t<const datum_t> generate_point() {
    double lat = randdouble() * 180.0 - 90.0;
    double lon = randdouble() * 360.0 - 180.0;
    return construct_geo_point(lat_lon_point_t(lat, lon));
}

counted_t<const datum_t> generate_line() {
    lat_lon_line_t l;
    size_t num_vertices = randsize(62) + 2;
    double granularity = randdouble() * 0.1;
    // Pick a starting point
    double lat = randdouble() * 180.0 - 90.0;
    double lon = randdouble() * 360.0 - 180.0;
    l.push_back(lat_lon_point_t(lat, lon));
    for (size_t i = 0; i < num_vertices; ++i) {
        // Then continue from there with relatively small variations...
        lat += randdouble() * 2.0 * granularity - granularity;
        if (lat > 90.0) {
            lat -= 180.0;
        } else if (lat < -90.0) {
            lat += 180.0;
        }
        if (lon > 180.0) {
            lon -= 360.0;
        } else if (lon < -180.0) {
            lon += 360.0;
        }
        lon += lat + randdouble() * 2.0 * granularity - granularity;
        l.push_back(lat_lon_point_t(lat, lon));
    }
    try {
        counted_t<const datum_t> res = construct_geo_point(lat_lon_point_t(lat, lon));
        validate_geojson(res);
        return res;
    } catch (const geo_exception_t &e) {
        // In case we ended up with an illegal line (e.g. with double vertices),
        // we just start over. It shouldn't happen very often.
        return generate_line();
    }
}

counted_t<const datum_t> generate_polygon() {
    // We just construct polygons out of circles for now. One outer circle:
    size_t num_vertices = randsize(61) + 3;
    double lat = randdouble() * 180.0 - 90.0;
    double lon = randdouble() * 360.0 - 180.0;
    double r = randdouble() * 1000.0; // Up to 1 km radius
    lat_lon_line_t shell =
        build_circle(lat_lon_point_t(lat, lon), r, num_vertices, WGS84_ELLIPSOID);

    // And maybe 1 or 2 holes...
    std::vector<lat_lon_line_t> holes;
    size_t num_holes = randsize(2);
    for (size_t i = 0; i < num_holes; ++i) {
        // Just some heuristics. These will not always lead to valid polygons.
        size_t hole_num_vertices = randsize(29) + 3;
        double hole_lat = lat + randdouble() * r - (0.5 * r);
        double hole_lon = lon + randdouble() * r - (0.5 * r);
        double hole_r = randdouble() * (0.5 * r);
        lat_lon_line_t hole =
            build_circle(lat_lon_point_t(hole_lat, hole_lon), hole_r, hole_num_vertices,
                         WGS84_ELLIPSOID);
        holes.push_back(hole);
    }
    try {
        counted_t<const datum_t> res = construct_geo_polygon(shell, holes);
        validate_geojson(res);
        return res;
    } catch (const geo_exception_t &e) {
        // In case we ended up with an illegal polygon (e.g. holes intersected),
        // we just start over.
        return generate_polygon();
    }
}

std::vector<counted_t<const datum_t> > generate_data(size_t num_docs) {
    std::vector<counted_t<const datum_t> > result;
    result.reserve(num_docs);

    for (size_t i = 0; i < num_docs; ++i) {
        if (i % 3 == 0) {
            result.push_back(generate_point());
        } else if (i % 3 == 1) {
            result.push_back(generate_line());
        } else {
            result.push_back(generate_polygon());
        }
    }

    return result;
}

void insert_data(namespace_interface_t *nsi,
                 order_source_t *osource,
                 const std::vector<counted_t<const datum_t> > &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        store_key_t pk(strprintf("%zu", i));
        write_t write(
            point_write_t(pk, data[i]),
            DURABILITY_REQUIREMENT_SOFT,
            profile_bool_t::PROFILE);
        write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::insert_data(geo_indexes.cc"),
                   &interruptor);

        if (!boost::get<point_write_response_t>(&response.response)) {
            ADD_FAILURE() << "got wrong type of result back";
        } else if (boost::get<point_write_response_t>(&response.response)->result
                   != point_write_result_t::STORED) {
            ADD_FAILURE() << "failed to insert document";
        }
    }
}

void prepare_namespace(namespace_interface_t *nsi,
                       order_source_t *osource,
                       const std::vector<counted_t<const datum_t> > &data) {
    // Create an index
    std::string index_id = "geo";

    const ql::sym_t arg(1);
    ql::protob_t<const Term> mapping = ql::r::var(arg).release_counted();
    ql::map_wire_func_t m(mapping, make_vector(arg), get_backtrace(mapping));

    write_t write(sindex_create_t(index_id, m, sindex_multi_bool_t::SINGLE,
                                  sindex_geo_bool_t::GEO),
                  profile_bool_t::PROFILE);
    write_response_t response;

    cond_t interruptor;
    nsi->write(write, &response,
               osource->check_in("unittest::prepare_namespace(geo_indexes.cc"),
               &interruptor);

    if (!boost::get<sindex_create_response_t>(&response.response)) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    // Wait for it to become ready
    wait_for_sindex(nsi, osource, index_id);

    // Insert the test data
    insert_data(nsi, osource, data);
}

void run_get_nearest_test(namespace_interface_t *nsi, order_source_t *osource) {
    const size_t num_docs = 500;
    debugf("Generating test data\n");
    std::vector<counted_t<const datum_t> > data = generate_data(num_docs);
    debugf("  done\n");
    debugf("Inserting data\n");
    prepare_namespace(nsi, osource, data);
    debugf("  done\n");

    try {
        // TODO! Perform the actual test
    } catch (const geo_exception_t &e) {
        debugf("Caught a geo exception: %s\n", e.what());
        FAIL();
    }
}

// Test that `get_nearest` results agree with `distance`
TPTEST(GeoIndexes, GetNearest) {
    run_with_namespace_interface(&run_get_nearest_test);
}

// Test that `get_intersecting` results agree with `intersects`
TPTEST(GeoIndexes, GetIntersecting) {
    try {
        // TODO!
    } catch (const geo_exception_t &e) {
        debugf("Caught a geo exception: %s\n", e.what());
        FAIL();
    }
}

} /* namespace unittest */


