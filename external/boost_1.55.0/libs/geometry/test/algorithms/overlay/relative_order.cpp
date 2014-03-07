// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>

#include <geometry_test_common.hpp>


#include <boost/foreach.hpp>

#include <boost/geometry/algorithms/intersection.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_relative_order.hpp>

#include <boost/geometry/geometries/point_xy.hpp>

#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif


template <typename P, typename T>
void test_with_point(std::string const& caseid,
                T pi_x, T pi_y, T pj_x, T pj_y,
                T ri_x, T ri_y, T rj_x, T rj_y,
                T si_x, T si_y, T sj_x, T sj_y,
                int expected_order)
{
    P pi = bg::make<P>(pi_x, pi_y);
    P pj = bg::make<P>(pj_x, pj_y);
    P ri = bg::make<P>(ri_x, ri_y);
    P rj = bg::make<P>(rj_x, rj_y);
    P si = bg::make<P>(si_x, si_y);
    P sj = bg::make<P>(sj_x, sj_y);

    int order = bg::detail::overlay::get_relative_order<P>::apply(pi, pj, ri, rj, si, sj);

    BOOST_CHECK_EQUAL(order, expected_order);




    /*
    std::cout << caseid
        << (caseid.find("_") == std::string::npos ? "  " : "")
        << " " << method
        << " " << detected
        << " " << order
        << std::endl;
    */



/*#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename << "get_turn_info_" << caseid
            << "_" << string_from_type<typename bg::coordinate_type<P>::type>::name()
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<P> mapper(svg, 500, 500);
        mapper.add(bg::make<P>(0, 0));
        mapper.add(bg::make<P>(10, 10));

        bg::model::linestring<P> p; p.push_back(pi); p.push_back(pj); p.push_back(pk);
        bg::model::linestring<P> q; q.push_back(qi); q.push_back(qj); q.push_back(qk);
        mapper.map(p, "opacity:0.8;stroke:rgb(0,192,0);stroke-width:3");
        mapper.map(q, "opacity:0.8;stroke:rgb(0,0,255);stroke-width:3");

        std::string style =  ";font-family='Verdana';font-weight:bold";
        std::string align = ";text-anchor:end;text-align:end";
        int offset = 8;

        mapper.text(pi, "pi", "fill:rgb(0,192,0)" + style, offset, offset);
        mapper.text(pj, "pj", "fill:rgb(0,192,0)" + style, offset, offset);
        mapper.text(pk, "pk", "fill:rgb(0,192,0)" + style, offset, offset);

        mapper.text(qi, "qi", "fill:rgb(0,0,255)" + style + align, -offset, offset);
        mapper.text(qj, "qj", "fill:rgb(0,0,255)" + style + align, -offset, offset);
        mapper.text(qk, "qk", "fill:rgb(0,0,255)" + style + align, -offset, offset);


        int factor = 1; // second info, if any, will go left by factor -1
        int ch = '1';
        for (typename tp_vector::const_iterator it = info.begin();
            it != info.end();
            ++it, factor *= -1, ch++)
        {
            bool at_j = it->method == bg::detail::overlay::method_crosses;
            std::string op;
            op += operation_char(it->operations[0].operation);
            align = ";text-anchor:middle;text-align:center";
            mapper.text(at_j ? pj : pk, op, "fill:rgb(255,128,0)" + style + align, offset * factor, -offset);

            op.clear();
            op += operation_char(it->operations[1].operation);
            mapper.text(at_j ? qj : qk, op, "fill:rgb(255,128,0)" + style + align, offset * factor, -offset);

            // Map intersection point + method
            mapper.map(it->point, "opacity:0.8;fill:rgb(255,0,0);stroke:rgb(0,0,100);stroke-width:1");

            op.clear();
            op += method_char(it->method);
            if (info.size() != 1)
            {
                op += ch;
                op += " p:"; op += operation_char(it->operations[0].operation);
                op += " q:"; op += operation_char(it->operations[1].operation);
            }
            mapper.text(it->point, op, "fill:rgb(255,0,0)" + style, offset, -offset);
        }
    }
#endif
*/
}



template <typename P>
void test_all()
{
    test_with_point<P, double>("OLR1",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            3, 3,   7, 2, // s
            1);
    test_with_point<P, double>("OLR2",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            3, 7,   7, 6, // s
            -1);
    test_with_point<P, double>("OLR3",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            4, 2,   9, 6, // s
            1);
    test_with_point<P, double>("OLR4",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            3, 8,   9, 4, // s
            -1);
    test_with_point<P, double>("OLR5",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            4, 2,   8, 6, // s
            1);
    test_with_point<P, double>("OLR6",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            3, 7,   9, 4, // s
            -1);
    test_with_point<P, double>("OLR7",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            1, 4,   7, 7, // s
            -1);
    test_with_point<P, double>("OLR8",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            1, 6,   7, 3, // s
            1);


    test_with_point<P, double>("OD1",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            7, 2,   3, 3, // s
            1);

    test_with_point<P, double>("OD9",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            7, 5,   3, 3, // s
            1);
    test_with_point<P, double>("OD10",
            5, 1,   5, 8, // p
            3, 5,   7, 5, // r
            7, 5,   3, 7, // s
            -1);
    test_with_point<P, double>("OD11",
            5, 1,   5, 8, // p
            7, 5,   3, 5, // r
            3, 5,   7, 7, // s
            -1);
    test_with_point<P, double>("OD12",
            5, 1,   5, 8, // p
            7, 5,   3, 5, // r
            3, 5,   7, 3, // s
            1);
}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    return 0;
}
