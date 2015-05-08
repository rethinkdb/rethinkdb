// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <algorithm>

#include "btree/keys.hpp"
#include "concurrency/fifo_checker.hpp"
#include "containers/counted.hpp"
#include "debug.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/lon_lat_types.hpp"
#include "rdb_protocol/geo/intersection.hpp"
#include "rdb_protocol/geo/primitives.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/store.hpp"
#include "stl_utils.hpp"
#include "unittest/rdb_protocol.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

using geo::S2Point;
using ql::datum_t;

namespace unittest {

datum_t generate_point(rng_t *rng) {
    double lat = rng->randdouble() * 180.0 - 90.0;
    double lon = rng->randdouble() * 360.0 - 180.0;
    return construct_geo_point(lon_lat_point_t(lon, lat), ql::configured_limits_t());
}

datum_t generate_line(rng_t *rng) {
    lon_lat_line_t l;
    size_t num_vertices = rng->randint(63) + 2;
    double granularity = rng->randdouble() * 0.1;
    // Pick a starting point
    double lat = rng->randdouble() * 180.0 - 90.0;
    double lon = rng->randdouble() * 360.0 - 180.0;
    l.push_back(lon_lat_point_t(lon, lat));
    for (size_t i = 0; i < num_vertices; ++i) {
        // Then continue from there with relatively small variations...
        lat += rng->randdouble() * 2.0 * granularity - granularity;
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
        lon += lat + rng->randdouble() * 2.0 * granularity - granularity;
        l.push_back(lon_lat_point_t(lon, lat));
    }
    try {
        datum_t res =
            construct_geo_point(lon_lat_point_t(lon, lat), ql::configured_limits_t());
        validate_geojson(res);
        return res;
    } catch (const geo_exception_t &e) {
        // In case we ended up with an illegal line (e.g. with double vertices),
        // we just start over. It shouldn't happen very often.
        return generate_line(rng);
    }
}

datum_t generate_polygon(rng_t *rng) {
    // We just construct polygons out of circles for now. One outer circle:
    size_t num_vertices = rng->randint(62) + 3;
    double lat = rng->randdouble() * 180.0 - 90.0;
    double lon = rng->randdouble() * 360.0 - 180.0;
    double r = rng->randdouble() * 1000.0; // Up to 1 km radius
    lon_lat_line_t shell =
        build_circle(lon_lat_point_t(lon, lat), r, num_vertices, WGS84_ELLIPSOID);

    // And maybe 1 or 2 holes...
    std::vector<lon_lat_line_t> holes;
    size_t num_holes = rng->randint(3);
    for (size_t i = 0; i < num_holes; ++i) {
        // Just some heuristics. These will not always lead to valid polygons.
        size_t hole_num_vertices = rng->randint(30) + 3;
        double hole_lat = lat + rng->randdouble() * r - (0.5 * r);
        double hole_lon = lon + rng->randdouble() * r - (0.5 * r);
        double hole_r = rng->randdouble() * (0.5 * r);
        lon_lat_line_t hole =
            build_circle(lon_lat_point_t(hole_lon, hole_lat), hole_r, hole_num_vertices,
                         WGS84_ELLIPSOID);
        holes.push_back(hole);
    }
    try {
        datum_t res =
            construct_geo_polygon(shell, holes, ql::configured_limits_t());
        validate_geojson(res);
        return res;
    } catch (const geo_exception_t &e) {
        // In case we ended up with an illegal polygon (e.g. holes intersected),
        // we just start over.
        return generate_polygon(rng);
    }
}

std::vector<datum_t> generate_data(size_t num_docs, rng_t *rng) {
    std::vector<datum_t> result;
    result.reserve(num_docs);

    for (size_t i = 0; i < num_docs; ++i) {
        if (i % 3 == 0) {
            result.push_back(generate_point(rng));
        } else if (i % 3 == 1) {
            result.push_back(generate_line(rng));
        } else {
            result.push_back(generate_polygon(rng));
        }
    }

    return result;
}

void insert_data(namespace_interface_t *nsi,
                 order_source_t *osource,
                 const std::vector<datum_t> &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        store_key_t pk(strprintf("%zu", i));
        write_t write(
            point_write_t(pk, data[i]),
            DURABILITY_REQUIREMENT_SOFT,
            profile_bool_t::PROFILE,
            ql::configured_limits_t());
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
                       const std::vector<scoped_ptr_t<store_t> > *stores,
                       const std::vector<datum_t> &data) {
    // Create an index
    std::string index_id = "geo";

    const ql::sym_t arg(1);
    ql::protob_t<const Term> mapping = ql::r::var(arg).release_counted();
    sindex_config_t sindex(
        ql::map_wire_func_t(mapping, make_vector(arg), ql::backtrace_id_t::empty()),
        reql_version_t::LATEST,
        sindex_multi_bool_t::SINGLE,
        sindex_geo_bool_t::GEO);

    cond_t non_interruptor;
    for (const auto &store : *stores) {
        store->sindex_create(index_id, sindex, &non_interruptor);
    }

    // Wait for it to become ready
    wait_for_sindex(stores, index_id);

    // Insert the test data
    insert_data(nsi, osource, data);
}

std::vector<nearest_geo_read_response_t::dist_pair_t> perform_get_nearest(
        lon_lat_point_t center,
        uint64_t max_results,
        double max_distance,
        namespace_interface_t *nsi,
        order_source_t *osource) {

    std::string table_name = "test_table"; // This is just used to print error messages
    std::string idx_name = "geo";
    read_t read(nearest_geo_read_t(region_t::universe(), center, max_distance,
                                   max_results, WGS84_ELLIPSOID, table_name, idx_name,
                                   std::map<std::string, ql::wire_func_t>()),
                profile_bool_t::PROFILE);
    read_response_t response;

    cond_t interruptor;
    nsi->read(read, &response,
              osource->check_in("unittest::perform_get_nearest(geo_indexes.cc"),
              &interruptor);

    nearest_geo_read_response_t *geo_response =
        boost::get<nearest_geo_read_response_t>(&response.response);
    if (geo_response == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
        return std::vector<nearest_geo_read_response_t::dist_pair_t>();
    }
    if (boost::get<ql::exc_t>(&geo_response->results_or_error) != NULL) {
        ADD_FAILURE() << boost::get<ql::exc_t>(&geo_response->results_or_error)->what();
        return std::vector<nearest_geo_read_response_t::dist_pair_t>();
    }
    return boost::get<nearest_geo_read_response_t::result_t>(geo_response->results_or_error);
}

bool nearest_pairs_less(
        const std::pair<double, ql::datum_t> &p1,
        const std::pair<double, ql::datum_t> &p2) {
    // We only care about the distance, don't compare the actual data.
    return p1.first < p2.first;
}

std::vector<nearest_geo_read_response_t::dist_pair_t> emulate_get_nearest(
        lon_lat_point_t center,
        uint64_t max_results,
        double max_distance,
        const std::vector<datum_t> &data) {

    std::vector<nearest_geo_read_response_t::dist_pair_t> result;
    result.reserve(data.size());
    ql::datum_t point_center =
        construct_geo_point(center, ql::configured_limits_t());
    scoped_ptr_t<S2Point> s2_center = to_s2point(point_center);
    for (size_t i = 0; i < data.size(); ++i) {
        nearest_geo_read_response_t::dist_pair_t entry(
            geodesic_distance(*s2_center, data[i], WGS84_ELLIPSOID), data[i]);
        result.push_back(entry);
    }
    std::sort(result.begin(), result.end(), &nearest_pairs_less);

    // Apply max_results and max_distance
    size_t cut_off = std::min(static_cast<size_t>(max_results), result.size());
    for (size_t i = 0; i < cut_off; ++i) {
        if (result[i].first > max_distance) {
            cut_off = i;
            break;
        }
    }
    result.resize(cut_off);

    return result;
}

void test_get_nearest(lon_lat_point_t center,
                      const std::vector<datum_t> &data,
                      namespace_interface_t *nsi,
                      order_source_t *osource) {
    const uint64_t max_results = 100;
    const double max_distance = 5000000.0; // 5000 km

    // 1. Run get_nearest
    std::vector<nearest_geo_read_response_t::dist_pair_t> nearest_res =
        perform_get_nearest(center, max_results, max_distance, nsi, osource);

    // 2. Compute an equivalent result directly from data
    std::vector<nearest_geo_read_response_t::dist_pair_t> reference_res =
        emulate_get_nearest(center, max_results, max_distance, data);

    // 3. Compare both results
    ASSERT_EQ(nearest_res.size(), reference_res.size());
    for (size_t i = 0; i < nearest_res.size() && i < reference_res.size(); ++i) {
        // We only compare the distances. The odds that a wrong document
        // happens to have the same distance are negligible.
        // Also this avoids having to deal with two documents having the same distance,
        // which could then be re-ordered (though that is extremely unlikely as well).
        ASSERT_EQ(nearest_res[i].first, reference_res[i].first);
    }
}

void run_get_nearest_test(
        namespace_interface_t *nsi,
        order_source_t *osource,
        const std::vector<scoped_ptr_t<store_t> > *stores) {
    // To reproduce a known failure: initialize the rng seed manually.
    const int rng_seed = randint(INT_MAX);
    debugf("Using RNG seed %i\n", rng_seed);
    rng_t rng(rng_seed);

    const size_t num_docs = 500;
    std::vector<datum_t> data = generate_data(num_docs, &rng);
    prepare_namespace(nsi, osource, stores, data);

    try {
        const int num_runs = 20;
        for (int i = 0; i < num_runs; ++i) {
            double lat = rng.randdouble() * 180.0 - 90.0;
            double lon = rng.randdouble() * 360.0 - 180.0;
            test_get_nearest(lon_lat_point_t(lon, lat), data, nsi, osource);
        }
    } catch (const geo_exception_t &e) {
        debugf("Caught a geo exception: %s\n", e.what());
        FAIL();
    }
}

std::vector<datum_t> perform_get_intersecting(
        const datum_t &query_geometry,
        namespace_interface_t *nsi,
        order_source_t *osource) {

    std::string table_name = "test_table"; // This is just used to print error messages
    std::string idx_name = "geo";
    read_t read(intersecting_geo_read_t(boost::optional<changefeed_stamp_t>(),
                                        region_t::universe(),
                                        std::map<std::string, ql::wire_func_t>(),
                                        table_name, ql::batchspec_t::all(),
                                        std::vector<ql::transform_variant_t>(),
                                        boost::optional<ql::terminal_variant_t>(),
                                        sindex_rangespec_t(
                                            idx_name,
                                            region_t::universe(),
                                            ql::datum_range_t::universe()),
                                        query_geometry),
                profile_bool_t::PROFILE);
    read_response_t response;

    cond_t interruptor;
    nsi->read(read, &response,
              osource->check_in("unittest::perform_get_intersecting(geo_indexes.cc"),
              &interruptor);

    rget_read_response_t *geo_response =
        boost::get<rget_read_response_t>(&response.response);
    if (geo_response == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
        return std::vector<datum_t>();
    }
    if (boost::get<ql::exc_t>(&geo_response->result) != NULL) {
        ADD_FAILURE() << boost::get<ql::exc_t>(&geo_response->result)->what();
        return std::vector<datum_t>();
    }

    auto result = boost::get<ql::grouped_t<ql::stream_t> >(&geo_response->result);
    if (result == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
        return std::vector<datum_t>();
    }
    const ql::stream_t &result_stream = (*result)[datum_t::null()];
    std::vector<datum_t> result_datum;
    result_datum.reserve(result_stream.size());
    for (size_t i = 0; i < result_stream.size(); ++i) {
        result_datum.push_back(result_stream[i].data);
    }
    return result_datum;
}

std::vector<datum_t> emulate_get_intersecting(
        const datum_t &query_geometry,
        const std::vector<datum_t> &data) {

    std::vector<datum_t> result;
    result.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        if (geo_does_intersect(query_geometry, data[i])) {
            result.push_back(data[i]);
        }
    }

    return result;
}

void test_get_intersecting(const datum_t &query_geometry,
                           const std::vector<datum_t> &data,
                           namespace_interface_t *nsi,
                           order_source_t *osource) {
    // 1. Run get_intersecting
    std::vector<datum_t> intersecting_res =
        perform_get_intersecting(query_geometry, nsi, osource);

    // 2. Compute an equivalent result directly from data
    std::vector<datum_t> reference_res =
        emulate_get_intersecting(query_geometry, data);

    // 3. Compare both results
    ASSERT_EQ(intersecting_res.size(), reference_res.size());
    for (size_t i = 0; i < intersecting_res.size() && i < reference_res.size(); ++i) {
        ASSERT_EQ(intersecting_res[i], reference_res[i]);
    }
}

void run_get_intersecting_test(
        namespace_interface_t *nsi,
        order_source_t *osource,
        const std::vector<scoped_ptr_t<store_t> > *stores) {
    // To reproduce a known failure: initialize the rng seed manually.
    const int rng_seed = randint(INT_MAX);
    debugf("Using RNG seed %i\n", rng_seed);
    rng_t rng(rng_seed);

    const size_t num_docs = 500;
    std::vector<datum_t> data = generate_data(num_docs, &rng);
    prepare_namespace(nsi, osource, stores, data);

    try {
        const int num_point_runs = 10;
        const int num_line_runs = 10;
        const int num_polygon_runs = 20;
        for (int i = 0; i < num_point_runs; ++i) {
            datum_t query_geometry = generate_point(&rng);
            test_get_intersecting(query_geometry, data, nsi, osource);
        }
        for (int i = 0; i < num_line_runs; ++i) {
            datum_t query_geometry = generate_line(&rng);
            test_get_intersecting(query_geometry, data, nsi, osource);
        }
        for (int i = 0; i < num_polygon_runs; ++i) {
            datum_t query_geometry = generate_polygon(&rng);
            test_get_intersecting(query_geometry, data, nsi, osource);
        }
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
    run_with_namespace_interface(&run_get_intersecting_test);
}

} /* namespace unittest */


