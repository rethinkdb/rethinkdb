// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/detail/algorithms/path_intersection.hpp>

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/segment.hpp>

//#include <boost/geometry/io/wkt/read.hpp>

template <typename Box, typename Linestring>
void test_path_intersection(Box const& box, Linestring const& path,
                            bool expected_result,
                            typename bg::default_length_result<Linestring>::type expected_dist)
{
    typename bgi::detail::default_path_intersection_distance_type<Box, Linestring>::type dist;

    bool value = bgi::detail::path_intersection(box, path, dist);
    BOOST_CHECK(value == expected_result);
    if ( value && expected_result )
        BOOST_CHECK_CLOSE(dist, expected_dist, 0.0001);

    if ( ::boost::size(path) == 2 )
    {
        typedef typename ::boost::range_value<Linestring>::type P;
        typedef bg::model::segment<P> Seg;
        typename bgi::detail::default_path_intersection_distance_type<Box, Seg>::type dist;
        Seg seg(*::boost::begin(path), *(::boost::begin(path)+1));
        bool value = bgi::detail::path_intersection(box, seg, dist);
        BOOST_CHECK(value == expected_result);
        if ( value && expected_result )
            BOOST_CHECK_CLOSE(dist, expected_dist, 0.0001);
    }
}

template <typename Box, typename Linestring>
void test_geometry(std::string const& wkt_g, std::string const& wkt_path,
                   bool expected_result,
                   typename bg::default_length_result<Linestring>::type expected_dist)
{
    Box box;
    bg::read_wkt(wkt_g, box);
    Linestring path;
    bg::read_wkt(wkt_path, path);
    test_path_intersection(box, path, expected_result, expected_dist);
}

void test_large_integers()
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> int_point_type;
    typedef bg::model::point<double, 2, bg::cs::cartesian> double_point_type;

    bg::model::box<int_point_type> int_box;
    bg::model::box<double_point_type> double_box;
    typedef bg::model::linestring<int_point_type> IP;
    IP int_path;
    typedef bg::model::linestring<double_point_type> DP;
    DP double_path;

    std::string const str_box = "POLYGON((1536119 192000, 1872000 528000))";
    std::string const str_path = "LINESTRING(1535000 191000, 1873000 191000, 1873000 300000, 1536119 300000)";
    bg::read_wkt(str_box, int_box);
    bg::read_wkt(str_box, double_box);
    bg::read_wkt(str_path, int_path);
    bg::read_wkt(str_path, double_path);

    bg::default_length_result<IP>::type int_value;
    bool int_result = bgi::detail::path_intersection(int_box, int_path, int_value);
    bg::default_length_result<DP>::type double_value;
    bool double_result = bgi::detail::path_intersection(double_box, double_path, double_value);

    BOOST_CHECK(int_result == double_result);
    if ( int_result && double_result )
        BOOST_CHECK_CLOSE(int_value, double_value, 0.0001);
}

int test_main(int, char* [])
{
    typedef bg::model::point<int, 2, bg::cs::cartesian> P2ic;
    typedef bg::model::point<float, 2, bg::cs::cartesian> P2fc;
    typedef bg::model::point<double, 2, bg::cs::cartesian> P2dc;

    typedef bg::model::point<int, 3, bg::cs::cartesian> P3ic;
    typedef bg::model::point<float, 3, bg::cs::cartesian> P3fc;
    typedef bg::model::point<double, 3, bg::cs::cartesian> P3dc;

    typedef bg::model::linestring<P2ic> L2ic;
    typedef bg::model::linestring<P2fc> L2fc;
    typedef bg::model::linestring<P2dc> L2dc;

    typedef bg::model::linestring<P3ic> L3ic;
    typedef bg::model::linestring<P3fc> L3fc;
    typedef bg::model::linestring<P3dc> L3dc;
    
    // IMPORTANT! For 2-point linestrings comparable distance optimization is enabled!

    test_geometry<bg::model::box<P2ic>, L2ic>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 2 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P2fc>, L2fc>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 2 5)", true, 1.0f/5);
    test_geometry<bg::model::box<P2dc>, L2dc>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 2 5)", true, 1.0/5);
    test_geometry<bg::model::box<P3ic>, L3ic>("POLYGON((0 1 2,2 4 6))", "LINESTRING(0 0 0, 2 5 7)", true, 2.0f/7);
    test_geometry<bg::model::box<P3fc>, L3fc>("POLYGON((0 1 2,2 4 6))", "LINESTRING(0 0 0, 2 5 7)", true, 2.0f/7);
    test_geometry<bg::model::box<P3dc>, L3dc>("POLYGON((0 1 2,2 4 6))", "LINESTRING(0 0 0, 2 5 7)", true, 2.0/7);

    test_geometry<bg::model::box<P2fc>, L2fc>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 1 0, 1 5)", true, 2);
    test_geometry<bg::model::box<P2fc>, L2fc>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 3 0, 3 2, 0 2)", true, 6);
    test_geometry<bg::model::box<P2fc>, L2fc>("POLYGON((0 1,2 4))", "LINESTRING(1 2, 3 3, 0 3)", true, 0);
    
#ifdef HAVE_TTMATH
    typedef bg::model::point<ttmath_big, 2, bg::cs::cartesian> P2ttmc;
    typedef bg::model::point<ttmath_big, 3, bg::cs::cartesian> P3ttmc;

    typedef bg::model::linestring<P2ttmc> L2ttmc;
    typedef bg::model::linestring<P3ttmc> L3ttmc;

    test_geometry<bg::model::box<P2ttmc>, L2ttmc>("POLYGON((0 1,2 4))", "LINESTRING(0 0, 2 5)", true, 1.0/5);
    test_geometry<bg::model::box<P3ttmc>, L3ttmc>("POLYGON((0 1 2,2 4 6))", "LINESTRING(0 0 0, 2 5 7)", true, 2.0/7);
#endif

    test_large_integers();

    return 0;
}
