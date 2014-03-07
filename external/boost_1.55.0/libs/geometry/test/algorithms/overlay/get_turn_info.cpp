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
#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif



template <typename P, typename T>
void test_with_point(std::string const& caseid,
                T pi_x, T pi_y, T pj_x, T pj_y, T pk_x, T pk_y,
                T qi_x, T qi_y, T qj_x, T qj_y, T qk_x, T qk_y,
                bg::detail::overlay::method_type expected_method,
                T ip_x, T ip_y,
                std::string const& expected,
                T ip_x2, T ip_y2)
{
    P pi = bg::make<P>(pi_x, pi_y);
    P pj = bg::make<P>(pj_x, pj_y);
    P pk = bg::make<P>(pk_x, pk_y);
    P qi = bg::make<P>(qi_x, qi_y);
    P qj = bg::make<P>(qj_x, qj_y);
    P qk = bg::make<P>(qk_x, qk_y);


    typedef bg::detail::overlay::turn_info<P> turn_info;
    typedef std::vector<turn_info> tp_vector;
    turn_info model;
    tp_vector info;
    bg::detail::overlay::get_turn_info
        <
            P, P, turn_info,
            bg::detail::overlay::assign_null_policy
        >::apply(pi, pj, pk, qi, qj, qk,
        model, std::back_inserter(info));


    if (info.size() == 0)
    {
        BOOST_CHECK_EQUAL(expected_method,
                bg::detail::overlay::method_none);
    }

    std::string detected;
    std::string method;
    for (typename tp_vector::const_iterator it = info.begin(); it != info.end(); ++it)
    {
        for (int t = 0; t < 2; t++)
        {
            detected += bg::operation_char(it->operations[t].operation);
            method += bg::method_char(it->method);
        }
    }


    /*
    std::cout << caseid
        << (caseid.find("_") == std::string::npos ? "  " : "")
        << " " << method
        << " " << detected
        << " " << order
        << std::endl;
    */


    BOOST_CHECK_EQUAL(detected, expected);

    if (! info.empty())
    {
        BOOST_CHECK_EQUAL(info[0].method, expected_method);
        BOOST_CHECK_CLOSE(bg::get<0>(info[0].point), ip_x, 0.001);
        BOOST_CHECK_CLOSE(bg::get<1>(info[0].point), ip_y, 0.001);

        if (info.size() > 1)
        {
            BOOST_CHECK_EQUAL(info.size(), 2u);
            BOOST_CHECK_EQUAL(info[1].method, expected_method);
            BOOST_CHECK_CLOSE(bg::get<0>(info[1].point), ip_x2, 0.001);
            BOOST_CHECK_CLOSE(bg::get<1>(info[1].point), ip_y2, 0.001);
        }
    }

#if defined(TEST_WITH_SVG)
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
            op += bg::operation_char(it->operations[0].operation);
            align = ";text-anchor:middle;text-align:center";
            mapper.text(at_j ? pj : pk, op, "fill:rgb(255,128,0)" + style + align, offset * factor, -offset);

            op.clear();
            op += bg::operation_char(it->operations[1].operation);
            mapper.text(at_j ? qj : qk, op, "fill:rgb(255,128,0)" + style + align, offset * factor, -offset);

            // Map intersection point + method
            mapper.map(it->point, "opacity:0.8;fill:rgb(255,0,0);stroke:rgb(0,0,100);stroke-width:1");

            op.clear();
            op += bg::method_char(it->method);
            if (info.size() != 1)
            {
                op += ch;
                op += " p:"; op += bg::operation_char(it->operations[0].operation);
                op += " q:"; op += bg::operation_char(it->operations[1].operation);
            }
            mapper.text(it->point, op, "fill:rgb(255,0,0)" + style, offset, -offset);
        }
    }
#endif

}

template <typename P, typename T>
void test_both(std::string const& caseid,
                T pi_x, T pi_y, T pj_x, T pj_y, T pk_x, T pk_y,
                T qi_x, T qi_y, T qj_x, T qj_y, T qk_x, T qk_y,
                bg::detail::overlay::method_type method
                    = bg::detail::overlay::method_none,
                T ip_x = -1, T ip_y = -1,
                std::string const& expected = "",
                T ip_x2 = -1, T ip_y2 = -1)
{
    test_with_point<P, double>(caseid,
            pi_x, pi_y, pj_x, pj_y, pk_x, pk_y,
            qi_x, qi_y, qj_x, qj_y, qk_x, qk_y,
            method, ip_x, ip_y, expected, ip_x2, ip_y2);

    //return;

    std::string reverse;
    for (int i = expected.size() - 1; i >= 0; i--)
    {
        reverse += expected[i];
    }
    if (ip_x2 >= 0 && ip_y2 >= 0)
    {
        std::swap(ip_x, ip_x2);
        std::swap(ip_y, ip_y2);
    }

    test_with_point<P, double>(caseid + "_r",
            qi_x, qi_y, qj_x, qj_y, qk_x, qk_y, // q
            pi_x, pi_y, pj_x, pj_y, pk_x, pk_y, // p
            method, ip_x, ip_y, reverse, ip_x2, ip_y2);
}


template <typename P>
void test_all()
{
    using namespace bg::detail::overlay;

    // See powerpoint "doc/testcases/get_turn_info.ppt"


    // ------------------------------------------------------------------------
    // "Real" intersections ("i"), or, crossing
    // ------------------------------------------------------------------------
    test_both<P, double>("il1",
            5, 1,   5, 6,   7, 8, // p
            3, 3,   7, 5,   8, 3, // q
            method_crosses, 5, 4, "ui");

    test_both<P, double>("il2",
            5, 1,   5, 6,   7, 8, // p
            3, 5,   7, 5,   3, 3, // q
            method_crosses, 5, 5, "ui");

    test_both<P, double>("il3",
            5, 1,   5, 6,   7, 8, // p
            3, 3,   7, 5,   3, 5, // q
            method_crosses, 5, 4, "ui");

    test_both<P, double>("il4",
            5, 1,   5, 6,   7, 8, // p
            3, 3,   7, 5,   4, 8, // q
            method_crosses, 5, 4, "ui");

    test_both<P, double>("ir1",
            5, 1,   5, 6,   7, 8, // p
            7, 5,   3, 3,   2, 5, // q
            method_crosses, 5, 4, "iu");


    // ------------------------------------------------------------------------
    // TOUCH INTERIOR or touch in the middle ("m")
    // ------------------------------------------------------------------------
    test_both<P, double>("ml1",
            5, 1,   5, 6,   7, 8, // p
            3, 3,   5, 4,   7, 3, // q
            method_touch_interior, 5, 4, "ui");

    test_both<P, double>("ml2",
            5, 1,   5, 6,   7, 8, // p
            3, 3,   5, 4,   3, 6, // q
            method_touch_interior, 5, 4, "iu");

    test_both<P, double>("ml3",
            5, 1,   5, 6,   7, 8, // p
            3, 6,   5, 4,   3, 3, // q
            method_touch_interior, 5, 4, "uu");

    test_both<P, double>("mr1",
            5, 1,   5, 6,   7, 8, // p
            7, 3,   5, 4,   3, 3, // q
            method_touch_interior, 5, 4, "iu");

    test_both<P, double>("mr2",
            5, 1,   5, 6,   7, 8, // p
            7, 3,   5, 4,   7, 6, // q
            method_touch_interior, 5, 4, "ui");

    test_both<P, double>("mr3",
            5, 1,   5, 6,   7, 8, // p
            7, 6,   5, 4,   7, 3, // q
            method_touch_interior, 5, 4, "ii");

    test_both<P, double>("mcl",
            5, 1,   5, 6,   7, 8, // p
            3, 2,   5, 3,   5, 5, // q
            method_touch_interior, 5, 3, "cc");

    test_both<P, double>("mcr",
            5, 1,   5, 6,   7, 8, // p
            7, 2,   5, 3,   5, 5, // q
            method_touch_interior, 5, 3, "cc");

    test_both<P, double>("mclo",
            5, 1,   5, 6,   7, 8, // p
            3, 4,   5, 5,   5, 3, // q
            method_touch_interior, 5, 5, "ux");

    test_both<P, double>("mcro",
            5, 1,   5, 6,   7, 8, // p
            7, 4,   5, 5,   5, 3, // q
            method_touch_interior, 5, 5, "ix");

    // ------------------------------------------------------------------------
    // COLLINEAR
    // ------------------------------------------------------------------------
    test_both<P, double>("cll1",
            5, 1,   5, 6,   3, 8, // p
            5, 5,   5, 7,   3, 8, // q
            method_collinear, 5, 6, "ui");
    test_both<P, double>("cll2",
            5, 1,   5, 6,   3, 8, // p
            5, 3,   5, 5,   3, 6, // q
            method_collinear, 5, 5, "iu");
    test_both<P, double>("clr1",
            5, 1,   5, 6,   3, 8, // p
            5, 5,   5, 7,   6, 8, // q
            method_collinear, 5, 6, "ui");
    test_both<P, double>("clr2",
            5, 1,   5, 6,   3, 8, // p
            5, 3,   5, 5,   6, 6, // q
            method_collinear, 5, 5, "ui");

    test_both<P, double>("crl1",
            5, 1,   5, 6,   7, 8, // p
            5, 5,   5, 7,   3, 8, // q
            method_collinear, 5, 6, "iu");
    test_both<P, double>("crl2",
            5, 1,   5, 6,   7, 8, // p
            5, 3,   5, 5,   3, 6, // q
            method_collinear, 5, 5, "iu");
    test_both<P, double>("crr1",
            5, 1,   5, 6,   7, 8, // p
            5, 5,   5, 7,   6, 8, // q
            method_collinear, 5, 6, "iu");
    test_both<P, double>("crr2",
            5, 1,   5, 6,   7, 8, // p
            5, 3,   5, 5,   6, 6, // q
            method_collinear, 5, 5, "ui");

    test_both<P, double>("ccx1",
            5, 1,   5, 6,   5, 8, // p
            5, 5,   5, 7,   3, 8, // q
            method_collinear, 5, 6, "cc");
    test_both<P, double>("cxc1",
            5, 1,   5, 6,   7, 8, // p
            5, 3,   5, 5,   5, 7, // q
            method_collinear, 5, 5, "cc");

    // Bug in case #54 of "overlay_cases.hpp"
    test_both<P, double>("c_bug1",
            5, 0,   2, 0,   2, 2, // p
            4, 0,   1, 0,   1, 2, // q
            method_collinear, 2, 0, "iu");


    // ------------------------------------------------------------------------
    // COLLINEAR OPPOSITE
    // ------------------------------------------------------------------------

    test_both<P, double>("clo1",
            5, 2,   5, 6,   3, 8, // p
            5, 7,   5, 5,   3, 3, // q
            method_collinear, 5, 6, "ixxu", 5, 5);
    test_both<P, double>("clo2",
            5, 2,   5, 6,   3, 8, // p
            5, 7,   5, 5,   5, 2, // q
            method_collinear, 5, 6, "ix");
                // actually "xxix", xx is skipped everywhere
    test_both<P, double>("clo3",
            5, 2,   5, 6,   3, 8, // p
            5, 7,   5, 5,   7, 3, // q
            method_collinear, 5, 6, "ixxi", 5, 5);

    test_both<P, double>("cco1",
            5, 2,   5, 6,   5, 8, // p
            5, 7,   5, 5,   3, 3, // q
            method_collinear, 5, 5, "xu"); // "xuxx"
    test_both<P, double>("cco2",
            5, 2,   5, 6,   5, 8, // p
            5, 7,   5, 5,   5, 2); // q "xxxx"
    test_both<P, double>("cco3",
            5, 2,   5, 6,   5, 8, // p
            5, 7,   5, 5,   7, 3, // q
            method_collinear, 5, 5, "xi"); // "xixx"


    test_both<P, double>("cro1",
            5, 2,   5, 6,   7, 8, // p
            5, 7,   5, 5,   3, 3, // q
            method_collinear, 5, 6, "uxxu", 5, 5);
    test_both<P, double>("cro2",
            5, 2,   5, 6,   7, 8, // p
            5, 7,   5, 5,   5, 2, // q
            method_collinear, 5, 6, "ux"); // "xxux"
    test_both<P, double>("cro3",
            5, 2,   5, 6,   7, 8, // p
            5, 7,   5, 5,   7, 3, // q
            method_collinear, 5, 6, "uxxi", 5, 5);

    test_both<P, double>("cxo1",
            5, 2,   5, 6,   3, 8, // p
            5, 5,   5, 3,   3, 1, // q
            method_collinear, 5, 3, "xu");
    test_both<P, double>("cxo2",
            5, 2,   5, 6,   3, 8, // p
            5, 5,   5, 3,   5, 0); // q   "xx"
    test_both<P, double>("cxo3",
            5, 2,   5, 6,   3, 8, // p
            5, 5,   5, 3,   7, 1, // q
            method_collinear, 5, 3, "xi");

    test_both<P, double>("cxo4",
            5, 2,   5, 6,   3, 8, // p
            5, 7,   5, 1,   3, 0, // q
            method_collinear, 5, 6, "ix");
    test_both<P, double>("cxo5",
            5, 2,   5, 6,   5, 8, // p
            5, 7,   5, 1,   3, 0); // q  "xx"
    test_both<P, double>("cxo6",
            5, 2,   5, 6,   7, 8, // p
            5, 7,   5, 1,   3, 0, // q
            method_collinear, 5, 6, "ux");


    // Verify
    test_both<P, double>("cvo1",
            5, 3,   5, 7,   7, 9, // p
            5, 5,   5, 3,   3, 1 // q
            );
    test_both<P, double>("cvo2",
            5, 3,   5, 7,   7, 9, // p
            5, 4,   5, 2,   3, 0 // q
            );


    // ------------------------------------------------------------------------
    // TOUCH - both same
    // ------------------------------------------------------------------------
    // Both left, Q turns right
    test_both<P, double>("blr1",
            5, 1,   5, 6,   4, 4, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("blr2",
            5, 1,   5, 6,   1, 4, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("blr3",
            5, 1,   5, 6,   3, 6, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("blr4",
            5, 1,   5, 6,   1, 8, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "xu");
    test_both<P, double>("blr5",
            5, 1,   5, 6,   4, 8, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "uu");
    test_both<P, double>("blr6",
            5, 1,   5, 6,   6, 4, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "uu");

    test_both<P, double>("blr7",
            5, 1,   5, 6,   3, 6, // p
            3, 7,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ix");
    test_both<P, double>("blr8",
            5, 1,   5, 6,   3, 6, // p
            3, 6,   5, 6,   5, 3, // q
            method_touch, 5, 6, "xx");
    test_both<P, double>("blr9",
            5, 1,   5, 6,   3, 6, // p
            3, 5,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ux");

    // Variants
    test_both<P, double>("blr7-a",
            5, 1,   5, 6,   3, 6, // p
            5, 8,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ix");
    test_both<P, double>("blr7-b", // in fact NOT "both-left"
            5, 1,   5, 6,   3, 6, // p
            6, 8,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ix");

    // To check if "collinear-check" on other side
    // does not apply to this side
    test_both<P, double>("blr6-c1",
            5, 1,   5, 6,   7, 5, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "uu");
    test_both<P, double>("blr6-c2",
            5, 1,   5, 6,   7, 7, // p
            3, 7,   5, 6,   3, 5, // q
            method_touch, 5, 6, "uu");



    // Both right, Q turns right
    test_both<P, double>("brr1",
            5, 1,   5, 6,   6, 4, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "uu");
    test_both<P, double>("brr2",
            5, 1,   5, 6,   9, 4, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "xu");
    test_both<P, double>("brr3",
            5, 1,   5, 6,   7, 6, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("brr4",
            5, 1,   5, 6,   9, 8, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("brr5",
            5, 1,   5, 6,   6, 8, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("brr6",
            5, 1,   5, 6,   4, 4, // p
            7, 5,   5, 6,   7, 7, // q
            method_touch, 5, 6, "ui");

    // Both right, Q turns left
    test_both<P, double>("brl1",
            5, 1,   5, 6,   6, 4, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("brl2",
            5, 1,   5, 6,   9, 4, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("brl3",
            5, 1,   5, 6,   7, 6, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("brl4",
            5, 1,   5, 6,   9, 8, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "xi");
    test_both<P, double>("brl5",
            5, 1,   5, 6,   6, 8, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "ii");
    test_both<P, double>("brl6",
            5, 1,   5, 6,   4, 4, // p
            7, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "ii");
    test_both<P, double>("brl7",
            5, 1,   5, 6,   7, 6, // p
            7, 7,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ux");
    test_both<P, double>("brl8",
            5, 1,   5, 6,   7, 6, // p
            7, 6,   5, 6,   5, 3, // q
            method_touch, 5, 6, "xx");
    test_both<P, double>("brl9",
            5, 1,   5, 6,   7, 6, // p
            7, 5,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ix");

    // Variants
    test_both<P, double>("brl7-a",
            5, 1,   5, 6,   7, 6, // p
            5, 8,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ux");
    test_both<P, double>("brl7-b", // in fact NOT "both right"
            5, 1,   5, 6,   7, 6, // p
            4, 8,   5, 6,   5, 3, // q
            method_touch, 5, 6, "ux");



    // Both left, Q turns left
    test_both<P, double>("bll1",
            5, 1,   5, 6,   4, 4, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "ii");
    test_both<P, double>("bll2",
            5, 1,   5, 6,   1, 4, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "xi");
    test_both<P, double>("bll3",
            5, 1,   5, 6,   3, 6, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("bll4",
            5, 1,   5, 6,   1, 8, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("bll5",
            5, 1,   5, 6,   4, 8, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("bll6",
            5, 1,   5, 6,   6, 4, // p
            3, 5,   5, 6,   3, 7, // q
            method_touch, 5, 6, "iu");

    // TOUCH - COLLINEAR + one side
    // Collinear/left, Q turns right
    test_both<P, double>("t-clr1",
            5, 1,   5, 6,   4, 4, // p
            5, 8,   5, 6,   3, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("t-clr2",
            5, 1,   5, 6,   1, 4, // p
            5, 8,   5, 6,   3, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("t-clr3",
            5, 1,   5, 6,   3, 6, // p
            5, 8,   5, 6,   3, 5, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("t-clr4",
            5, 1,   5, 6,   5, 8, // p
            5, 8,   5, 6,   3, 5, // q
            method_touch, 5, 6, "xu");
    // 5 n.a.
    test_both<P, double>("t-clr6",
            5, 1,   5, 6,   6, 4, // p
            5, 8,   5, 6,   3, 5, // q
            method_touch, 5, 6, "uu");

    // Collinear/right, Q turns right
    test_both<P, double>("t-crr1",
            5, 1,   5, 6,   6, 4, // p
            7, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "uu");
    test_both<P, double>("t-crr2",
            5, 1,   5, 6,   9, 4, // p
            7, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "xu");
    test_both<P, double>("t-crr3",
            5, 1,   5, 6,   7, 6, // p
            7, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("t-crr4",
            5, 1,   5, 6,   5, 9, // p
            7, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "cc");
    // 5 n.a.
    test_both<P, double>("t-crr6",
            5, 1,   5, 6,   4, 4, // p
            7, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "ui");

    // Collinear/right, Q turns left
    test_both<P, double>("t-crl1",
            5, 1,   5, 6,   6, 4, // p
            5, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("t-crl2",
            5, 1,   5, 6,   9, 4, // p
            5, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("t-crl3",
            5, 1,   5, 6,   7, 6, // p
            5, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("t-crl4",
            5, 1,   5, 6,   5, 8, // p
            5, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "xi");
    // 5 n.a.
    test_both<P, double>("t-crl6",
            5, 1,   5, 6,   4, 4, // p
            5, 7,   5, 6,   7, 5, // q
            method_touch, 5, 6, "ii");

    // Collinear/left, Q turns left
    test_both<P, double>("t-cll1",
            5, 1,   5, 6,   4, 4, // p
            3, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "ii");
    test_both<P, double>("t-cll2",
            5, 1,   5, 6,   1, 4, // p
            3, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "xi");
    test_both<P, double>("t-cll3",
            5, 1,   5, 6,   3, 6, // p
            3, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("t-cll4",
            5, 1,   5, 6,   5, 9, // p
            3, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "cc");
    // 5 n.a.
    test_both<P, double>("t-cll6",
            5, 1,   5, 6,   6, 4, // p
            3, 5,   5, 6,   5, 8, // q
            method_touch, 5, 6, "iu");

    // Left to right
    test_both<P, double>("lr1",
            5, 1,   5, 6,   3, 3, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ii");
    test_both<P, double>("lr2",
            5, 1,   5, 6,   1, 5, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "xi");
    test_both<P, double>("lr3",
            5, 1,   5, 6,   4, 8, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("lr4",
            5, 1,   5, 6,   9, 5, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("lr5",
            5, 1,   5, 6,   7, 3, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "iu");
    // otherwise case more thoroughly
    test_both<P, double>("lr3a",
            5, 1,   5, 6,   1, 6, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("lr3b",
            5, 1,   5, 6,   5, 10, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("lr3c",
            5, 1,   5, 6,   8, 9, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("lr3d",
            5, 1,   5, 6,   9, 7, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("lr3e",
            5, 1,   5, 6,   9, 6, // p
            1, 5,   5, 6,   9, 5, // q
            method_touch, 5, 6, "ui");

    // Right to left
    test_both<P, double>("rl1",
            5, 1,   5, 6,   3, 3, // p
            9, 5,   5, 6,   1, 5, // q
            method_touch, 5, 6, "ui");
    test_both<P, double>("rl2",
            5, 1,   5, 6,   1, 5, // p
            9, 5,   5, 6,   1, 5, // q
            method_touch, 5, 6, "cc");
    test_both<P, double>("rl3",
            5, 1,   5, 6,   4, 8, // p
            9, 5,   5, 6,   1, 5, // q
            method_touch, 5, 6, "iu");
    test_both<P, double>("rl4",
            5, 1,   5, 6,   9, 5, // p
            9, 5,   5, 6,   1, 5, // q
            method_touch, 5, 6, "xu");
    test_both<P, double>("rl5",
            5, 1,   5, 6,   7, 3, // p
            9, 5,   5, 6,   1, 5, // q
            method_touch, 5, 6, "uu");

    // Equal (p1/q1 are equal)
    test_both<P, double>("ebl1",
            5, 1,   5, 6,   3, 4, // p
            5, 1,   5, 6,   3, 8, // q
            method_equal, 5, 6, "ui");
    test_both<P, double>("ebl2",
            5, 1,   5, 6,   3, 8, // p
            5, 1,   5, 6,   3, 4, // q
            method_equal, 5, 6, "iu");
    test_both<P, double>("ebl3",
            5, 1,   5, 6,   3, 8, // p
            5, 1,   5, 6,   3, 8, // q
            method_equal, 5, 6, "cc");

    test_both<P, double>("ebl3-c1",
            5, 1,   5, 6,   10, 1, // p
            5, 1,   5, 6,   3, 8, // q
            method_equal, 5, 6, "iu");

    test_both<P, double>("ebr1",
            5, 1,   5, 6,   7, 4, // p
            5, 1,   5, 6,   7, 8, // q
            method_equal, 5, 6, "iu");
    test_both<P, double>("ebr2",
            5, 1,   5, 6,   7, 8, // p
            5, 1,   5, 6,   7, 4, // q
            method_equal, 5, 6, "ui");
    test_both<P, double>("ebr3",
            5, 1,   5, 6,   7, 8, // p
            5, 1,   5, 6,   7, 8, // q
            method_equal, 5, 6, "cc");

    test_both<P, double>("ebr3-c1",
            5, 1,   5, 6,   0, 1, // p
            5, 1,   5, 6,   7, 8, // q
            method_equal, 5, 6, "ui");

    test_both<P, double>("elr1",
            5, 1,   5, 6,   7, 8, // p
            5, 1,   5, 6,   3, 8, // q
            method_equal, 5, 6, "iu");
    test_both<P, double>("elr2",
            5, 1,   5, 6,   3, 8, // p
            5, 1,   5, 6,   7, 8, // q
            method_equal, 5, 6, "ui");
    test_both<P, double>("ec1",
            5, 1,   5, 6,   5, 8, // p
            5, 1,   5, 6,   5, 8, // q
            method_equal, 5, 6, "cc");
    test_both<P, double>("ec2",
            5, 1,   5, 6,   5, 8, // p
            5, 1,   5, 6,   5, 7, // q
            method_equal, 5, 6, "cc");

    test_both<P, double>("snl-1",
            0, 3,   2, 3,   4, 3, // p
            4, 3,   2, 3,   0, 3, // q
            method_touch, 2, 3, "xx");

    // BSG 2012-05-26 to be decided what's the problem here and what it tests... 
    // Anyway, test results are not filled out.
    //test_both<P, double>("issue_buffer_mill",
    //        5.1983614873206241 , 6.7259025813913107 , 5.0499999999999998 , 6.4291796067500622 , 5.1983614873206241 , 6.7259025813913107, // p
    //        5.0499999999999998 , 6.4291796067500622 , 5.0499999999999998 , 6.4291796067500622 , 5.1983614873206241 , 6.7259025813913107, // q
    //        method_collinear, 2, 0, "tt");

}


/***
#include <boost/geometry/geometries/adapted/c_array.hpp>
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)

template <typename G>
void test2(G const& geometry)
{
    typedef typename bg::point_type<G>::type P;
    typedef typename bg::tag<G>::type T;
    typedef typename bg::tag<P>::type PT;
    std::cout << typeid(G).name() << std::endl;
    std::cout << typeid(T).name() << std::endl;
    std::cout << typeid(P).name() << std::endl;
    std::cout << typeid(PT).name() << std::endl;


    std::cout << bg::length(geometry) << std::endl;

    typedef bg::model::point<float, 3, bg::cs::cartesian> P2;
    bg::model::linestring<P2> out;
    bg::strategy::transform::scale_transformer<float[3], P2> scaler(5);
    bg::transform(geometry, out, scaler);
    std::cout << bg::dsv(out) << std::endl;
}

void test_f3()
{
    float vertices[][3] = {
        {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1},
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}
        };
    test2(vertices);
}
***/

int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    return 0;
}
