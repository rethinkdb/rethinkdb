// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/config.hpp>
#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/intersection_segment.hpp>
#include <boost/geometry/io/wkt/aswkt.hpp>

static std::ostream & operator<<(std::ostream &s, const bg::intersection_result& r)
{
    switch(r)
    {
        case bg::is_intersect_no : s << "is_intersect_no"; break;
        case bg::is_intersect : s << "is_intersect"; break;
        case bg::is_parallel : s << "is_parallel"; break;
        case bg::is_collinear_no : s << "is_collinear_no"; break;
        case bg::is_collinear_one : s << "is_collinear_one"; break;
        case bg::is_collinear_connect : s << "is_collinear_connect"; break;
        case bg::is_collinear_overlap : s << "is_collinear_overlap"; break;
        case bg::is_collinear_overlap_opposite : s << "is_collinear_overlap_opposite"; break;
        case bg::is_collinear_connect_opposite : s << "is_collinear_connect_opposite"; break;

        // detailed connection results:
        case bg::is_intersect_connect_s1p1 : s << "is_intersect_connect_s1p1"; break;
        case bg::is_intersect_connect_s1p2 : s << "is_intersect_connect_s1p2"; break;
        case bg::is_intersect_connect_s2p1 : s << "is_intersect_connect_s2p1"; break;
        case bg::is_intersect_connect_s2p2 : s << "is_intersect_connect_s2p2"; break;
    }
    return s;
}

static std::string as_string(const bg::intersection_result& r)
{
    std::stringstream out;
    out << r;
    return out.str();
}

typedef bg::model::point<double> P;
typedef bg::const_segment<P> S;


static void test_intersection(double s1x1, double s1y1, double s1x2, double s1y2,
                       double s2x1, double s2y1, double s2x2, double s2y2,
                       // Expected results
                       bg::intersection_result expected_result,
                       int exptected_count, const P& exp_p1, const P& exp_p2)
{
    S s1(P(s1x1, s1y1), P(s1x2, s1y2));
    S s2(P(s2x1, s2y1), P(s2x2, s2y2));
    std::vector<P> ip;
    double ra, rb;
    bg::intersection_result r = bg::intersection_s(s1, s2, ra, rb, ip);
    r = bg::intersection_connect_result(r, ra, rb);

    BOOST_CHECK_EQUAL(ip.size(), exptected_count);
    BOOST_CHECK_EQUAL(as_string(expected_result), as_string(r));

    if (ip.size() == 2 && ip[0] != exp_p1)
    {
        // Swap results, second point is not as expected, swap results, order is not prescribed,
        // it might be OK.
        std::reverse(ip.begin(), ip.end());
    }

    if (ip.size() >= 1)
    {
        BOOST_CHECK_EQUAL(ip[0], exp_p1);
    }
    if (ip.size() >= 2)
    {
        BOOST_CHECK_EQUAL(ip[1], exp_p2);
    }


    /*
    std::cout << exptected_count << " " << r;
    if (exptected_count >= 1) std::cout << " " << ip[0];
    if (exptected_count >= 2) std::cout << " " << ip[1];
    std::cout << std::endl;
    */
}

//BOOST_AUTO_TEST_CASE( test1 )
int test_main( int , char* [] )
{
    // Identical cases
    test_intersection(0,0, 1,1,  0,0, 1,1,          bg::is_collinear_overlap, 2,  P(0,0), P(1,1));
    test_intersection(1,1, 0,0,  0,0, 1,1,          bg::is_collinear_overlap_opposite, 2,  P(1,1), P(0,0));
    test_intersection(0,1, 0,2,  0,1, 0,2,          bg::is_collinear_overlap, 2,  P(0,1), P(0,2)); // Vertical
    test_intersection(0,2, 0,1,  0,1, 0,2,          bg::is_collinear_overlap_opposite, 2,  P(0,2), P(0,1)); // Vertical
    // Overlap cases
    test_intersection(0,0, 1,1,  -0.5,-0.5, 2,2,    bg::is_collinear_overlap, 2,  P(0,0), P(1,1));
    test_intersection(0,0, 1,1,  0.5,0.5, 1.5,1.5,  bg::is_collinear_overlap, 2,  P(0.5,0.5), P(1,1));
    test_intersection(0,0, 0,1,  0,-10, 0,10,       bg::is_collinear_overlap, 2,  P(0,0), P(0,1)); // Vertical
    test_intersection(0,0, 0,1,  0,10, 0,-10,       bg::is_collinear_overlap_opposite, 2,  P(0,0), P(0,1)); // Vertical
    test_intersection(0,0, 1,1,  1,1, 2,2,          bg::is_collinear_connect, 1,  P(1,1), P(0,0)); // Single point
    // Colinear, non overlap cases
    test_intersection(0,0, 1,1,  1.5,1.5, 2.5,2.5,  bg::is_collinear_no, 0,  P(0,0), P(0,0));
    test_intersection(0,0, 0,1,  0,5, 0,6,          bg::is_collinear_no, 0,  P(0,0), P(0,0)); // Vertical
    // Parallel cases
    test_intersection(0,0, 1,1,  1,0, 2,1,       bg::is_parallel, 0,  P(0,0), P(0,1));
    // Intersect cases
    test_intersection(0,2, 4,2,  3,0, 3,4,       bg::is_intersect, 1,  P(3,2), P(0,0));
    // Non intersect cases

    // Single point cases
    test_intersection(0,0, 0,0,  1,1, 2,2,          bg::is_collinear_no, 0,  P(1,1), P(0,0)); // Colinear/no
    test_intersection(2,2, 2,2,  1,1, 3,3,          bg::is_collinear_one, 1,  P(2,2.01), P(0,0)); // On segment
    test_intersection(1,1, 3,3,  2,2, 2,2,          bg::is_collinear_one, 1,  P(2,2), P(0,0)); // On segment
    test_intersection(1,1, 3,3,  1,1, 1,1,          bg::is_collinear_one, 1,  P(1,1), P(0,0)); // On segment, start
    test_intersection(1,1, 3,3,  3,3, 3,3,          bg::is_collinear_one, 1,  P(3,3), P(0,0)); // On segment, end

    return 0;
}
