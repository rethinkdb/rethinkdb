// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <iostream>
#include <iomanip>

#include <boost/foreach.hpp>

#include <geometry_test_common.hpp>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>

#include <algorithms/overlay/overlay_cases.hpp>

template <typename Geometry>
struct rev : boost::mpl::if_c<bg::point_order<Geometry>::value == bg::counterclockwise, boost::true_type, boost::false_type>::type
{};

template <typename Geometry1, typename Geometry2>
inline typename bg::coordinate_type<Geometry1>::type intersect(Geometry1 const& g1, Geometry2 const& g2, std::string const& name,
               bg::detail::overlay::operation_type op)
{
    typedef typename bg::strategy::side::services::default_strategy
    <
        typename bg::cs_tag<Geometry1>::type
    >::type side_strategy_type;


    typedef bg::detail::overlay::traversal_turn_info
        <
            typename bg::point_type<Geometry1>::type
        > turn_info;
    std::vector<turn_info> turns;

    bg::detail::get_turns::no_interrupt_policy policy;
    bg::get_turns
        <
            rev<Geometry1>::value,
            rev<Geometry2>::value,
            bg::detail::overlay::calculate_distance_policy
        >(g1, g2, turns, policy);

    bg::enrich_intersection_points<rev<Geometry1>::value, rev<Geometry2>::value >(turns, bg::detail::overlay::operation_intersection,
        g1, g2, side_strategy_type());

    typedef bg::model::ring<typename bg::point_type<Geometry1>::type> ring_type;
    typedef std::deque<ring_type> out_vector;
    out_vector v;

    bg::detail::overlay::traverse
        <
            rev<Geometry1>::value, rev<Geometry2>::value,
            Geometry1, Geometry2
        >::apply(g1, g2, op, turns, v);

    typename bg::coordinate_type<Geometry1>::type result = 0.0;
    BOOST_FOREACH(ring_type& ring, v)
    {
        result += bg::area(ring);
    }

#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename
            << name << "_"
            << (op == bg::detail::overlay::operation_intersection ? "i" : "u")
            << "_" << (rev<Geometry1>::value ? "ccw" : "cw")
            << "_" << (rev<Geometry2>::value ? "ccw" : "cw")
            << ".svg";

        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<typename bg::point_type<Geometry1>::type> mapper(svg, 500, 500);
        mapper.add(g1);
        mapper.add(g2);

        // Input shapes in green/blue
        mapper.map(g1, "fill-opacity:0.5;fill:rgb(153,204,0);"
                "stroke:rgb(153,204,0);stroke-width:3");
        mapper.map(g2, "fill-opacity:0.3;fill:rgb(51,51,153);"
                "stroke:rgb(51,51,153);stroke-width:3");

        // Traversal rings in magenta/light yellow fill
        BOOST_FOREACH(ring_type const& ring, v)
        {
            mapper.map(ring, "fill-opacity:0.3;stroke-opacity:0.4;fill:rgb(255,255,0);"
                    "stroke:rgb(255,0,255);stroke-width:8");
        }


        // turn points in orange, + enrichment/traversal info

        // Simple map to avoid two texts at same place (note that can still overlap!)
        std::map<std::pair<int, int>, int> offsets;
        int index = 0;
        int const lineheight = 10;
        int const margin = 5;

        BOOST_FOREACH(turn_info const& turn, turns)
        {
            mapper.map(turn.point, "fill:rgb(255,128,0);"
                    "stroke:rgb(0,0,0);stroke-width:1", 3);

            {
                // Map characteristics
                // Create a rounded off point
                std::pair<int, int> p
                    = std::make_pair(
                        boost::numeric_cast<int>(0.5 + 10 * bg::get<0>(turn.point)),
                        boost::numeric_cast<int>(0.5 + 10 * bg::get<1>(turn.point))
                        );
                std::string style =  "fill:rgb(0,0,0);font-family:Arial;font-size:10px";

                std::ostringstream out;
                out << index
                    << ": " << bg::method_char(turn.method)
                    << std::endl
                    << "op: " << bg::operation_char(turn.operations[0].operation)
                    << " / " << bg::operation_char(turn.operations[1].operation)
                    << (turn.is_discarded() ? " (discarded) " : turn.blocked() ? " (blocked)" : "")
                    << std::endl;

                if (turn.operations[0].enriched.next_ip_index != -1)
                {
                    out << "ip: " << turn.operations[0].enriched.next_ip_index;
                }
                else
                {
                    out << "vx: " << turn.operations[0].enriched.travels_to_vertex_index;
                }
                out << " ";
                if (turn.operations[1].enriched.next_ip_index != -1)
                {
                    out << "ip: " << turn.operations[1].enriched.next_ip_index;
                }
                else
                {
                    out << "vx: " << turn.operations[1].enriched.travels_to_vertex_index;
                }

                out << std::endl;

                out

                    << std::setprecision(3)
                    << "dist: " << turn.operations[0].enriched.distance
                    << " / "  << turn.operations[1].enriched.distance
                    << std::endl;

                offsets[p] += lineheight;
                int offset = offsets[p];
                offsets[p] += lineheight * 5;
                mapper.text(turn.point, out.str(), style, margin, offset, lineheight);
            }
            index++;
        }
    }
#endif

    return result;
}

template <typename Geometry1, typename Geometry2>
inline typename bg::coordinate_type<Geometry1>::type intersect(std::string const& wkt1, std::string const& wkt2, std::string const& name,
            bg::detail::overlay::operation_type op)
{
    Geometry1 geometry1;
    Geometry2 geometry2;
    bg::read_wkt(wkt1, geometry1);
    bg::read_wkt(wkt2, geometry2);

    // Reverse if necessary: adapt to cw/ccw
    bg::correct(geometry1);
    bg::correct(geometry2);

    return intersect(geometry1, geometry2, name, op);
}

template <typename T>
inline void test_polygon(std::string const& wkt1, std::string const& wkt2, std::string const& name)
{
    typedef bg::model::d2::point_xy<T> point;
    typedef bg::model::polygon<point> clock;
    typedef bg::model::polygon<point, false> counter;

    namespace ov = bg::detail::overlay;
    T area1 = intersect<clock, clock>(wkt1, wkt2, name, ov::operation_intersection);
    T area2 = intersect<counter, counter>(wkt1, wkt2, name, ov::operation_intersection);
    T area3 = intersect<clock, counter>(wkt1, wkt2, name, ov::operation_intersection);
    T area4 = intersect<counter, clock>(wkt1, wkt2, name, ov::operation_intersection);
    BOOST_CHECK_CLOSE(area1, area2, 0.001);
    BOOST_CHECK_CLOSE(area3, area4, 0.001);
    BOOST_CHECK_CLOSE(area1, area3, 0.001);
    BOOST_CHECK_CLOSE(area2, area4, 0.001);

    area1 = intersect<clock, clock>(wkt1, wkt2, name, ov::operation_union);
    area2 = intersect<counter, counter>(wkt1, wkt2, name, ov::operation_union);
    area3 = intersect<clock, counter>(wkt1, wkt2, name, ov::operation_union);
    area4 = intersect<counter, clock>(wkt1, wkt2, name, ov::operation_union);
    BOOST_CHECK_CLOSE(area1, area2, 0.001);
    BOOST_CHECK_CLOSE(area3, area4, 0.001);
    BOOST_CHECK_CLOSE(area1, area3, 0.001);
    BOOST_CHECK_CLOSE(area2, area4, 0.001);
}

template <typename T>
inline void test_box_polygon(std::string const& wkt1, std::string const& wkt2, std::string const& name)
{
    typedef bg::model::d2::point_xy<T> point;
    typedef bg::model::box<point> box;
    typedef bg::model::polygon<point> clock;
    typedef bg::model::polygon<point, false> counter;

    namespace ov = bg::detail::overlay;
    T area1 = intersect<box, clock>(wkt1, wkt2, name + "_bp", ov::operation_intersection);
    T area2 = intersect<box, counter>(wkt1, wkt2, name + "_bp", ov::operation_intersection);
    T area3 = intersect<clock, box>(wkt2, wkt1, name + "_pb", ov::operation_intersection);
    T area4 = intersect<counter, box>(wkt2, wkt1, name + "_pb", ov::operation_intersection);
    BOOST_CHECK_CLOSE(area1, area2, 0.001);
    BOOST_CHECK_CLOSE(area3, area4, 0.001);
    BOOST_CHECK_CLOSE(area1, area3, 0.001);
    BOOST_CHECK_CLOSE(area2, area4, 0.001);

    area1 = intersect<box, clock>(wkt1, wkt2, name + "_bp", ov::operation_union);
    area2 = intersect<box, counter>(wkt1, wkt2, name + "_bp", ov::operation_union);
    area3 = intersect<clock, box>(wkt2, wkt1, name + "_pb", ov::operation_union);
    area4 = intersect<counter, box>(wkt2, wkt1, name + "_pb", ov::operation_union);
    BOOST_CHECK_CLOSE(area1, area2, 0.001);
    BOOST_CHECK_CLOSE(area3, area4, 0.001);
    BOOST_CHECK_CLOSE(area1, area3, 0.001);
    BOOST_CHECK_CLOSE(area2, area4, 0.001);
}

int test_main(int, char* [])
{
    //bool const ig = true;
    test_polygon<double>(case_1[0], case_1[1], "c1");
    test_polygon<double>(case_2[0], case_2[1], "c2");
    test_polygon<double>(case_3[0], case_3[1], "c3");
    test_polygon<double>(case_4[0], case_4[1], "c4");
    test_polygon<double>(case_5[0], case_5[1], "c5");
    test_polygon<double>(case_6[0], case_6[1], "c6");
    test_polygon<double>(case_7[0], case_7[1], "c7");
    test_polygon<double>(case_8[0], case_8[1], "c8");
    test_polygon<double>(case_9[0], case_9[1], "c9" /*, ig */);


    test_polygon<double>(case_10[0], case_10[1], "c10");
    test_polygon<double>(case_11[0], case_11[1], "c11");
    test_polygon<double>(case_12[0], case_12[1], "c12");
    test_polygon<double>(case_13[0], case_13[1], "c13");
    test_polygon<double>(case_14[0], case_14[1], "c14");
    test_polygon<double>(case_15[0], case_15[1], "c15");
    test_polygon<double>(case_16[0], case_16[1], "c16");
    test_polygon<double>(case_17[0], case_17[1], "c17");
    test_polygon<double>(case_18[0], case_18[1], "c18");
    test_polygon<double>(case_19[0], case_19[1], "c19");
    test_polygon<double>(case_20[0], case_20[1], "c20");
    test_polygon<double>(case_21[0], case_21[1], "c21");
    test_polygon<double>(case_22[0], case_22[1], "c22" /*, ig */);
    test_polygon<double>(case_23[0], case_23[1], "c23");
    test_polygon<double>(case_24[0], case_24[1], "c24");
    test_polygon<double>(case_25[0], case_25[1], "c25" /*, ig */);
    test_polygon<double>(case_26[0], case_26[1], "c26" /*, ig */);
    test_polygon<double>(case_27[0], case_27[1], "c27");
    test_polygon<double>(case_28[0], case_28[1], "c28");
    test_polygon<double>(case_29[0], case_29[1], "c29");
    test_polygon<double>(case_30[0], case_30[1], "c30");
    test_polygon<double>(case_31[0], case_31[1], "c31" /*, ig */);
    test_polygon<double>(case_32[0], case_32[1], "c32" /*, ig */);
    test_polygon<double>(case_33[0], case_33[1], "c33" /*, ig */);
    test_polygon<double>(case_34[0], case_34[1], "c34");
    test_polygon<double>(case_35[0], case_35[1], "c35");
    test_polygon<double>(case_36[0], case_36[1], "c36" /*, ig */);
    test_polygon<double>(case_37[0], case_37[1], "c37" /*, ig */);
    test_polygon<double>(case_38[0], case_38[1], "c38" /*, ig */);
    test_polygon<double>(case_39[0], case_39[1], "c39");
    test_polygon<double>(case_40[0], case_40[1], "c40" /*, ig */);
    test_polygon<double>(case_41[0], case_41[1], "c41");
    test_polygon<double>(case_42[0], case_42[1], "c42");
    //test_polygon<double>(case_43[0], case_43[1], "c43", inv);
    test_polygon<double>(case_44[0], case_44[1], "c44");
    test_polygon<double>(case_45[0], case_45[1], "c45");
    //test_polygon<double>(case_46[0], case_46[1], "c46", inv);
    //test_polygon<double>(case_47[0], case_47[1], "c47" /*, ig */);
    //test_polygon<double>(case_48[0], case_48[1], "c48");
    test_polygon<double>(case_49[0], case_49[1], "c49");
    test_polygon<double>(case_50[0], case_50[1], "c50");
    test_polygon<double>(case_51[0], case_51[1], "c51");
    test_polygon<double>(case_52[0], case_52[1], "c52" /*, ig */);
    test_polygon<double>(case_53[0], case_53[1], "c53");
    // Invalid ones / overlaying intersection points / self tangencies
    //test_polygon<double>(case_54[0], case_54[1], "c54");
    //test_polygon<double>(case_55[0], case_55[1], "c55");
    //test_polygon<double>(case_56[0], case_56[1], "c56");
    //test_polygon<double>(case_57[0], case_57[1], "c57" /*, ig */);
    //test_polygon<double>(case_58[0], case_58[1], "c58");
    //test_polygon<double>(case_59[0], case_59[1], "c59");

    test_polygon<double>(pie_16_4_12[0], pie_16_4_12[1], "pie_16_4_12");
    test_polygon<double>(pie_23_21_12_500[0], pie_23_21_12_500[1], "pie_23_21_12_500");
    test_polygon<double>(pie_23_23_3_2000[0], pie_23_23_3_2000[1], "pie_23_23_3_2000");
    test_polygon<double>(pie_23_16_16[0], pie_23_16_16[1], "pie_23_16_16");
    test_polygon<double>(pie_16_2_15_0[0], pie_16_2_15_0[1], "pie_16_2_15_0");
    test_polygon<double>(pie_4_13_15[0], pie_4_13_15[1], "pie_4_13_15");
    test_polygon<double>(pie_20_20_7_100[0], pie_20_20_7_100[1], "pie_20_20_7_100");

    test_polygon<double>(hv_1[0], hv_1[1], "hv1");
    test_polygon<double>(hv_2[0], hv_2[1], "hv2");
    test_polygon<double>(hv_3[0], hv_3[1], "hv3");
    test_polygon<double>(hv_4[0], hv_4[1], "hv4");
    test_polygon<double>(hv_5[0], hv_5[1], "hv5");
    test_polygon<double>(hv_6[0], hv_6[1], "hv6");
    test_polygon<double>(hv_7[0], hv_7[1], "hv7");
    test_polygon<double>(dz_1[0], dz_1[1], "dz_1");
    test_polygon<double>(dz_2[0], dz_2[1], "dz_2");
    test_polygon<double>(dz_3[0], dz_3[1], "dz_3");

    test_box_polygon<double>("POLYGON((1 1,4 4))", case_1[0], "bp1");

    {
        static std::string example_box = "POLYGON((1.5 1.5, 4.5 2.5))";
        static std::string example_ring =
            "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3))";
        test_box_polygon<double>(example_box, example_ring, "bp2");
    }

    return 0;
}
