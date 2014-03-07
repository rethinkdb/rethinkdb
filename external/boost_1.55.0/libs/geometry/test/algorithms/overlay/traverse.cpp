// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE
//#define BOOST_GEOMETRY_OVERLAY_NO_THROW
//#define TEST_WITH_SVG
//#define HAVE_TTMATH

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/type_traits/is_same.hpp>

#ifdef HAVE_TTMATH
#  include <boost/geometry/contrib/ttmath_stub.hpp>
#endif

#include <geometry_test_common.hpp>


// #define BOOST_GEOMETRY_DEBUG_ENRICH
//#define BOOST_GEOMETRY_DEBUG_RELATIVE_ORDER

// #define BOOST_GEOMETRY_REPORT_OVERLAY_ERROR
// #define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER


#define BOOST_GEOMETRY_TEST_OVERLAY_NOT_EXCHANGED

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
#  define BOOST_GEOMETRY_DEBUG_IDENTIFIER
#endif

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/calculate_distance_policy.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>


#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/correct.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>


#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

#include <boost/geometry/strategies/strategies.hpp>

#include <algorithms/overlay/overlay_cases.hpp>

static inline std::string operation(int d)
{
    return d == 1 ? "union" : "intersection";
}

namespace detail
{

template
<
    typename G1, typename G2,
    bg::detail::overlay::operation_type Direction,
    bool Reverse1, bool Reverse2
>
struct test_traverse
{

    static void apply(std::string const& id,
            std::size_t expected_count, double expected_area,
            G1 const& g1, G2 const& g2,
            double precision)
    {
        // DEBUG ONE or FEW CASE(S) ONLY
        //if (! boost::contains(id, "36") || Direction != 1) return;
        //if (! boost::contains(id, "iet_") || boost::contains(id, "st")) return;
        //if (! boost::contains(id, "66") || Direction != 1) return;
        //if (! boost::contains(id, "92") && ! boost::contains(id, "96") ) return;
        //if (! (boost::contains(id, "58_st") || boost::contains(id, "59_st") || boost::contains(id, "60_st")  || boost::contains(id, "83"))  ) return;
        //if (! (boost::contains(id, "81") || boost::contains(id, "82") || boost::contains(id, "84") || boost::contains(id, "85")  || boost::contains(id, "68"))  ) return;
        //if (! (boost::contains(id, "81") || boost::contains(id, "86") || boost::contains(id, "88"))  ) return;
        //if (! boost::contains(id, "58_") || Direction != 1) return;
        //if (! boost::contains(id, "55") || Direction != 1) return;
        //if (! boost::contains(id, "55_iet_iet") || Direction != 1) return;
        //if (! boost::contains(id, "55_st_iet") || Direction != 1) return;
        //if (! boost::contains(id, "55_iet_st") || Direction != 1) return;
        //if (! boost::contains(id, "54_st_st") || Direction != 1) return;
        //if (! boost::contains(id, "54_iet_st") || Direction != 1) return;
        //if (! (boost::contains(id, "54_") || boost::contains(id, "55_")) || Direction != 1) return;
        //if (Direction != 1) return;
        // END DEBUG ONE ...


        /*** FOR REVERSING ONLY
        {
            // If one or both are invalid (e.g. ccw),
            // they can be corrected by uncommenting this section
            G1 cg1 = g1;
            G2 cg2 = g2;
            bg::correct(cg1);
            bg::correct(cg2);
            std::cout << std::setprecision(12)
                << bg::wkt(cg1) << std::endl
                << bg::wkt(cg2) << std::endl;
        }
        ***/

#if defined(BOOST_GEOMETRY_DEBUG_OVERLAY) || defined(BOOST_GEOMETRY_DEBUG_ENRICH)
        bool const ccw =
            bg::point_order<G1>::value == bg::counterclockwise
            || bg::point_order<G2>::value == bg::counterclockwise;

        std::cout << std::endl
            << "TRAVERSE"
            << " " << id
            << (ccw ? "_ccw" : "")
            << " " << string_from_type<typename bg::coordinate_type<G1>::type>::name()
            << "("  << operation(Direction) << ")" << std::endl;

        //std::cout << bg::area(g1) << " " << bg::area(g2) << std::endl;
#endif

        typedef typename bg::strategy::side::services::default_strategy
        <
            typename bg::cs_tag<G1>::type
        >::type side_strategy_type;


        typedef bg::detail::overlay::traversal_turn_info
            <
                typename bg::point_type<G2>::type
            > turn_info;
        std::vector<turn_info> turns;

        bg::detail::get_turns::no_interrupt_policy policy;
        bg::get_turns<Reverse1, Reverse2, bg::detail::overlay::calculate_distance_policy>(g1, g2, turns, policy);
        bg::enrich_intersection_points<Reverse1, Reverse2>(turns,
                    Direction == 1 ? bg::detail::overlay::operation_union
                    : bg::detail::overlay::operation_intersection,
            g1, g2, side_strategy_type());

        typedef bg::model::ring<typename bg::point_type<G2>::type> ring_type;
        typedef std::vector<ring_type> out_vector;
        out_vector v;


        bg::detail::overlay::traverse
            <
                Reverse1, Reverse2,
                G1, G2
            >::apply(g1, g2, Direction, turns, v);

        // Check number of resulting rings
        BOOST_CHECK_MESSAGE(expected_count == boost::size(v),
                "traverse: " << id
                << " #shapes expected: " << expected_count
                << " detected: " << boost::size(v)
                << " type: " << string_from_type
                    <typename bg::coordinate_type<G1>::type>::name()
                );

        // Check total area of resulting rings
        typename bg::default_area_result<G1>::type total_area = 0;
        BOOST_FOREACH(ring_type const& ring, v)
        {
            total_area += bg::area(ring);
            //std::cout << bg::wkt(ring) << std::endl;
        }

        BOOST_CHECK_CLOSE(expected_area, total_area, precision);

#if defined(TEST_WITH_SVG)
        {
            std::ostringstream filename;
            filename << "traverse_" << operation(Direction)
                << "_" << id
                << "_" << string_from_type<typename bg::coordinate_type<G1>::type>::name()
                << ".svg";

            std::ofstream svg(filename.str().c_str());

            bg::svg_mapper<typename bg::point_type<G2>::type> mapper(svg, 500, 500);
            mapper.add(g1);
            mapper.add(g2);

            // Input shapes in green/blue
            mapper.map(g1, "fill-opacity:0.5;fill:rgb(153,204,0);"
                    "stroke:rgb(153,204,0);stroke-width:3");
            mapper.map(g2, "fill-opacity:0.3;fill:rgb(51,51,153);"
                    "stroke:rgb(51,51,153);stroke-width:3");

            // Traversal rings in magenta outline/red fill -> over blue/green this gives brown
            BOOST_FOREACH(ring_type const& ring, v)
            {
                mapper.map(ring, "fill-opacity:0.2;stroke-opacity:0.4;fill:rgb(255,0,0);"
                        "stroke:rgb(255,0,255);stroke-width:8");
            }

            // turn points in orange, + enrichment/traversal info
            typedef typename bg::coordinate_type<G1>::type coordinate_type;

            // Simple map to avoid two texts at same place (note that can still overlap!)
            std::map<std::pair<int, int>, int> offsets;
            int index = 0;
            int const margin = 5;

            BOOST_FOREACH(turn_info const& turn, turns)
            {
                int lineheight = 10;
                mapper.map(turn.point, "fill:rgb(255,128,0);"
                        "stroke:rgb(0,0,0);stroke-width:1", 3);

                {
                    coordinate_type half = 0.5;
                    coordinate_type ten = 10;
                    // Map characteristics
                    // Create a rounded off point
                    std::pair<int, int> p
                        = std::make_pair(
                            boost::numeric_cast<int>(half
                                + ten * bg::get<0>(turn.point)),
                            boost::numeric_cast<int>(half
                                + ten * bg::get<1>(turn.point))
                            );
                std::string style =  "fill:rgb(0,0,0);font-family:Arial;font-size:10px";

                if (turn.discarded)
                {
                    style =  "fill:rgb(92,92,92);font-family:Arial;font-size:6px";
                    lineheight = 6;
                }

                //if (! turn.is_discarded() && ! turn.blocked() && ! turn.both(bg::detail::overlay::operation_union))
                //if (! turn.discarded)
                {
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
                        out << "vx: " << turn.operations[0].enriched.travels_to_vertex_index
                         << " -> ip: "  << turn.operations[0].enriched.travels_to_ip_index;
                    }
                    out << " / ";
                    if (turn.operations[1].enriched.next_ip_index != -1)
                    {
                        out << "ip: " << turn.operations[1].enriched.next_ip_index;
                    }
                    else
                    {
                        out << "vx: " << turn.operations[1].enriched.travels_to_vertex_index
                         << " -> ip: " << turn.operations[1].enriched.travels_to_ip_index;
                    }
                    
                    out << std::endl;

                    /*out

                        << std::setprecision(3)
                        << "dist: " << boost::numeric_cast<double>(turn.operations[0].enriched.distance)
                        << " / "  << boost::numeric_cast<double>(turn.operations[1].enriched.distance)
                        << std::endl
                        << "vis: " << bg::visited_char(turn.operations[0].visited)
                        << " / "  << bg::visited_char(turn.operations[1].visited);
                    */

                    /*
                        out << index
                            << ": " << bg::operation_char(turn.operations[0].operation)
                            << " " << bg::operation_char(turn.operations[1].operation)
                            << " (" << bg::method_char(turn.method) << ")"
                            << (turn.ignore() ? " (ignore) " : " ")
                            << std::endl

                            << "ip: " << turn.operations[0].enriched.travels_to_ip_index
                            << "/"  << turn.operations[1].enriched.travels_to_ip_index;

                        if (turn.operations[0].enriched.next_ip_index != -1
                            || turn.operations[1].enriched.next_ip_index != -1)
                        {
                            out << " [" << turn.operations[0].enriched.next_ip_index
                                << "/"  << turn.operations[1].enriched.next_ip_index
                                << "]"
                                ;
                        }
                        out << std::endl;


                        out
                            << "vx:" << turn.operations[0].enriched.travels_to_vertex_index
                            << "/"  << turn.operations[1].enriched.travels_to_vertex_index
                            << std::endl

                            << std::setprecision(3)
                            << "dist: " << turn.operations[0].enriched.distance
                            << " / "  << turn.operations[1].enriched.distance
                            << std::endl
                            */



                        offsets[p] += lineheight;
                        int offset = offsets[p];
                        offsets[p] += lineheight * 3;
                        mapper.text(turn.point, out.str(), style, margin, offset, lineheight);
                    }
                    index++;
                }
            }
        }
#endif
    }
};
}

template
<
    typename G1, typename G2,
    bg::detail::overlay::operation_type Direction,
    bool Reverse1 = false,
    bool Reverse2 = false
>
struct test_traverse
{
    typedef detail::test_traverse
        <
            G1, G2, Direction, Reverse1, Reverse2
        > detail_test_traverse;

    inline static void apply(std::string const& id, std::size_t expected_count, double expected_area,
                std::string const& wkt1, std::string const& wkt2,
                double precision = 0.001)
    {
        if (wkt1.empty() || wkt2.empty())
        {
            return;
        }

        G1 g1;
        bg::read_wkt(wkt1, g1);

        G2 g2;
        bg::read_wkt(wkt2, g2);

        bg::correct(g1);
        bg::correct(g2);

        //std::cout << bg::wkt(g1) << std::endl;
        //std::cout << bg::wkt(g2) << std::endl;

        // Try the overlay-function in both ways
        std::string caseid = id;
        //goto case_reversed;

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        std::cout << std::endl << std::endl << "# " << caseid << std::endl;
#endif
        detail_test_traverse::apply(caseid, expected_count, expected_area, g1, g2, precision);

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        return;
#endif

    //case_reversed:
#if ! defined(BOOST_GEOMETRY_TEST_OVERLAY_NOT_EXCHANGED)
        caseid = id + "_rev";
#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        std::cout << std::endl << std::endl << "# " << caseid << std::endl;
#endif

        detail_test_traverse::apply(caseid, expected_count, expected_area, g2, g1, precision);
#endif
    }
};

#if ! defined(BOOST_GEOMETRY_TEST_MULTI)
template <typename T>
void test_all(bool test_self_tangencies = true, bool test_mixed = false)
{
    using namespace bg::detail::overlay;

    typedef bg::model::point<T, 2, bg::cs::cartesian> P;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::box<P> box;

    // 1-6
    test_traverse<polygon, polygon, operation_intersection>::apply("1", 1, 5.4736, case_1[0], case_1[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("2", 1, 12.0545, case_2[0], case_2[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("3", 1, 5, case_3[0], case_3[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("4", 1, 10.2212, case_4[0], case_4[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("5", 2, 12.8155, case_5[0], case_5[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("6", 1, 4.5, case_6[0], case_6[1]);

    // 7-12
    test_traverse<polygon, polygon, operation_intersection>::apply("7", 0, 0, case_7[0], case_7[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("8", 0, 0, case_8[0], case_8[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("9", 0, 0, case_9[0], case_9[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("10", 0, 0, case_10[0], case_10[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("11", 1, 1, case_11[0], case_11[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("12", 2, 0.63333, case_12[0], case_12[1]);

    // 13-18
    test_traverse<polygon, polygon, operation_intersection>::apply("13", 0, 0, case_13[0], case_13[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("14", 0, 0, case_14[0], case_14[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("15", 0, 0, case_15[0], case_15[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("16", 0, 0, case_16[0], case_16[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("17", 1, 2, case_17[0], case_17[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("18", 1, 2, case_18[0], case_18[1]);

    // 19-24
    test_traverse<polygon, polygon, operation_intersection>::apply("19", 0, 0, case_19[0], case_19[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("20", 1, 5.5, case_20[0], case_20[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("21", 0, 0, case_21[0], case_21[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("22", 0, 0, case_22[0], case_22[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("23", 1, 1.4, case_23[0], case_23[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("24", 1, 1.0, case_24[0], case_24[1]);

    // 25-30
    test_traverse<polygon, polygon, operation_intersection>::apply("25", 0, 0, case_25[0], case_25[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("26", 0, 0, case_26[0], case_26[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("27", 1, 0.9545454, case_27[0], case_27[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("28", 1, 0.9545454, case_28[0], case_28[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("29", 1, 1.4, case_29[0], case_29[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("30", 1, 0.5, case_30[0], case_30[1]);

    // 31-36
    test_traverse<polygon, polygon, operation_intersection>::apply("31", 0, 0, case_31[0], case_31[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("32", 0, 0, case_32[0], case_32[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("33", 0, 0, case_33[0], case_33[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("34", 1, 0.5, case_34[0], case_34[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("35", 1, 1.0, case_35[0], case_35[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("36", 1, 1.625, case_36[0], case_36[1]);

    // 37-42
    test_traverse<polygon, polygon, operation_intersection>::apply("37", 2, 0.666666, case_37[0], case_37[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("38", 2, 0.971429, case_38[0], case_38[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("39", 1, 24, case_39[0], case_39[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("40", 0, 0, case_40[0], case_40[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("41", 1, 5, case_41[0], case_41[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("42", 1, 5, case_42[0], case_42[1]);

    // 43-48 - invalid polygons
    //test_traverse<polygon, polygon, operation_intersection>::apply("43", 2, 0.75, case_43[0], case_43[1]);
    //test_traverse<polygon, polygon, operation_intersection>::apply("44", 1, 44, case_44[0], case_44[1]);
    //test_traverse<polygon, polygon, operation_intersection>::apply("45", 1, 45, case_45[0], case_45[1]);
    //test_traverse<polygon, polygon, operation_intersection>::apply("46", 1, 46, case_46[0], case_46[1]);
    //test_traverse<polygon, polygon, operation_intersection>::apply("47", 1, 47, case_47[0], case_47[1]);

    // 49-54

    test_traverse<polygon, polygon, operation_intersection>::apply("50", 0, 0, case_50[0], case_50[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("51", 0, 0, case_51[0], case_51[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("52", 1, 10.5, case_52[0], case_52[1]);
    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("53_st", 0, 0, case_53[0], case_53[1]);
    }
    test_traverse<polygon, polygon, operation_intersection>::apply("53_iet", 0, 0, case_53[0], case_53[2]);

    test_traverse<polygon, polygon, operation_intersection>::apply("54_iet_iet", 1, 2, case_54[1], case_54[3]);
    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("54_st_iet", 1, 2, case_54[0], case_54[3]);
        test_traverse<polygon, polygon, operation_intersection>::apply("54_iet_st", 1, 2, case_54[1], case_54[2]);
        test_traverse<polygon, polygon, operation_intersection>::apply("54_st_st", 1, 2, case_54[0], case_54[2]);
    }

    if (test_self_tangencies)
    {
        // 55-60
        test_traverse<polygon, polygon, operation_intersection>::apply("55_st_st", 1, 2, case_55[0], case_55[2]);
    }

    test_traverse<polygon, polygon, operation_intersection>::apply("55_st_iet", 1, 2, case_55[0], case_55[3]);
    test_traverse<polygon, polygon, operation_intersection>::apply("55_iet_st", 1, 2, case_55[1], case_55[2]);
    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("56", 2, 4.5, case_56[0], case_56[1]);
    }
    test_traverse<polygon, polygon, operation_intersection>::apply("55_iet_iet", 1, 2, case_55[1], case_55[3]);
    test_traverse<polygon, polygon, operation_intersection>::apply("57", 2, 5.9705882, case_57[0], case_57[1]);

    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("58_st",
            2, 0.333333, case_58[0], case_58[1]);
        test_traverse<polygon, polygon, operation_intersection>::apply("59_st",
            2, 1.5416667, case_59[0], case_59[1]);
        test_traverse<polygon, polygon, operation_intersection>::apply("60_st",
            3, 2, case_60[0], case_60[1]);
    }
    test_traverse<polygon, polygon, operation_intersection>::apply("58_iet",
        2, 0.333333, case_58[0], case_58[2]);
    test_traverse<polygon, polygon, operation_intersection>::apply("59_iet",
        2, 1.5416667, case_59[0], case_59[2]);
    test_traverse<polygon, polygon, operation_intersection>::apply("60_iet",
        3, 2, case_60[0], case_60[2]);
    test_traverse<polygon, polygon, operation_intersection>::apply("61_st",
        0, 0, case_61[0], case_61[1]);

    test_traverse<polygon, polygon, operation_intersection>::apply("70",
        2, 4, case_70[0], case_70[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("71",
        2, 2, case_71[0], case_71[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("72",
        3, 2.85, case_72[0], case_72[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("79",
        2, 20, case_79[0], case_79[1]);

    // other


    // pies (went wrong when not all cases where implemented, especially some collinear (opposite) cases
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_16_4_12",
        1, 491866.5, pie_16_4_12[0], pie_16_4_12[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_23_21_12_500",
        2, 2363199.3313, pie_23_21_12_500[0], pie_23_21_12_500[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_23_23_3_2000",
        2, 1867779.9349, pie_23_23_3_2000[0], pie_23_23_3_2000[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_23_16_16",
        2, 2128893.9555, pie_23_16_16[0], pie_23_16_16[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_16_2_15_0",
        0, 0, pie_16_2_15_0[0], pie_16_2_15_0[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_4_13_15",
        1, 490887.06678, pie_4_13_15[0], pie_4_13_15[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("pie_20_20_7_100",
        2, 2183372.2718, pie_20_20_7_100[0], pie_20_20_7_100[1]);



    // 1-6
    test_traverse<polygon, polygon, operation_union>::apply("1", 1, 11.5264, case_1[0], case_1[1]);
    test_traverse<polygon, polygon, operation_union>::apply("2", 1, 17.9455, case_2[0], case_2[1]);
    test_traverse<polygon, polygon, operation_union>::apply("3", 1, 9, case_3[0], case_3[1]);
    test_traverse<polygon, polygon, operation_union>::apply("4", 3, 17.7788, case_4[0], case_4[1]);
    test_traverse<polygon, polygon, operation_union>::apply("5", 2, 18.4345, case_5[0], case_5[1]);
    test_traverse<polygon, polygon, operation_union>::apply("6", 1, 9, case_6[0], case_6[1]);

    // 7-12
    test_traverse<polygon, polygon, operation_union>::apply("7", 1, 9, case_7[0], case_7[1]);
    test_traverse<polygon, polygon, operation_union>::apply("8", 1, 12, case_8[0], case_8[1]);
    test_traverse<polygon, polygon, operation_union>::apply("9", 0, 0 /*UU 2, 11*/, case_9[0], case_9[1]);
    test_traverse<polygon, polygon, operation_union>::apply("10", 1, 9, case_10[0], case_10[1]);
    test_traverse<polygon, polygon, operation_union>::apply("11", 1, 8, case_11[0], case_11[1]);
    test_traverse<polygon, polygon, operation_union>::apply("12", 2, 8.36667, case_12[0], case_12[1]);

    // 13-18
    test_traverse<polygon, polygon, operation_union>::apply("13", 1, 4, case_13[0], case_13[1]);
    test_traverse<polygon, polygon, operation_union>::apply("14", 1, 12, case_14[0], case_14[1]);
    test_traverse<polygon, polygon, operation_union>::apply("15", 1, 12, case_15[0], case_15[1]);
    test_traverse<polygon, polygon, operation_union>::apply("16", 1, 9, case_16[0], case_16[1]);
    test_traverse<polygon, polygon, operation_union>::apply("17", 1, 8, case_17[0], case_17[1]);
    test_traverse<polygon, polygon, operation_union>::apply("18", 1, 8, case_18[0], case_18[1]);

    // 19-24
    test_traverse<polygon, polygon, operation_union>::apply("19", 1, 10, case_19[0], case_19[1]);
    test_traverse<polygon, polygon, operation_union>::apply("20", 1, 5.5, case_20[0], case_20[1]);
    test_traverse<polygon, polygon, operation_union>::apply("21", 0, 0, case_21[0], case_21[1]);
    test_traverse<polygon, polygon, operation_union>::apply("22", 0, 0 /*UU 2, 9.5*/, case_22[0], case_22[1]);
    test_traverse<polygon, polygon, operation_union>::apply("23", 1, 6.1, case_23[0], case_23[1]);
    test_traverse<polygon, polygon, operation_union>::apply("24", 1, 5.5, case_24[0], case_24[1]);

    // 25-30
    test_traverse<polygon, polygon, operation_union>::apply("25", 0, 0 /*UU 2, 7*/, case_25[0], case_25[1]);
    test_traverse<polygon, polygon, operation_union>::apply("26", 0, 0 /*UU  2, 7.5 */, case_26[0], case_26[1]);
    test_traverse<polygon, polygon, operation_union>::apply("27", 1, 8.04545, case_27[0], case_27[1]);
    test_traverse<polygon, polygon, operation_union>::apply("28", 1, 10.04545, case_28[0], case_28[1]);
    test_traverse<polygon, polygon, operation_union>::apply("29", 1, 8.1, case_29[0], case_29[1]);
    test_traverse<polygon, polygon, operation_union>::apply("30", 1, 6.5, case_30[0], case_30[1]);

    // 31-36
    test_traverse<polygon, polygon, operation_union>::apply("31", 0, 0 /*UU 2, 4.5 */, case_31[0], case_31[1]);
    test_traverse<polygon, polygon, operation_union>::apply("32", 0, 0 /*UU 2, 4.5 */, case_32[0], case_32[1]);
    test_traverse<polygon, polygon, operation_union>::apply("33", 0, 0 /*UU 2, 4.5 */, case_33[0], case_33[1]);
    test_traverse<polygon, polygon, operation_union>::apply("34", 1, 6.0, case_34[0], case_34[1]);
    test_traverse<polygon, polygon, operation_union>::apply("35", 1, 10.5, case_35[0], case_35[1]);
    test_traverse<polygon, polygon, operation_union>::apply("36", 1 /*UU 2*/, 14.375, case_36[0], case_36[1]);

    // 37-42
    test_traverse<polygon, polygon, operation_union>::apply("37", 1, 7.33333, case_37[0], case_37[1]);
    test_traverse<polygon, polygon, operation_union>::apply("38", 1, 9.52857, case_38[0], case_38[1]);
    test_traverse<polygon, polygon, operation_union>::apply("39", 1, 40.0, case_39[0], case_39[1]);
    test_traverse<polygon, polygon, operation_union>::apply("40", 0, 0 /*UU 2, 11 */, case_40[0], case_40[1]);
    test_traverse<polygon, polygon, operation_union>::apply("41", 1, 5, case_41[0], case_41[1]);
    test_traverse<polygon, polygon, operation_union>::apply("42", 1, 5, case_42[0], case_42[1]);

    // 43-48
    //test_traverse<polygon, polygon, operation_union>::apply("43", 3, 8.1875, case_43[0], case_43[1]);
    //test_traverse<polygon, polygon, operation_union>::apply("44", 1, 44, case_44[0], case_44[1]);
    //test_traverse<polygon, polygon, operation_union>::apply("45", 1, 45, case_45[0], case_45[1]);
    //test_traverse<polygon, polygon, operation_union>::apply("46", 1, 46, case_46[0], case_46[1]);
    //test_traverse<polygon, polygon, operation_union>::apply("47", 1, 47, case_47[0], case_47[1]);

    // 49-54

    test_traverse<polygon, polygon, operation_union>::apply("50", 1, 25, case_50[0], case_50[1]);
    test_traverse<polygon, polygon, operation_union>::apply("51", 0, 0, case_51[0], case_51[1]);
    test_traverse<polygon, polygon, operation_union>::apply("52", 1, 15.5, case_52[0], case_52[1]);
    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_union>::apply("53_st", 2, 16, case_53[0], case_53[1]);
    }
    test_traverse<polygon, polygon, operation_union>::apply("53_iet",
            2, 16, case_53[0], case_53[2]);
    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_union>::apply("54_st_st", 2, 20, case_54[0], case_54[2]);
        test_traverse<polygon, polygon, operation_union>::apply("54_st_iet", 2, 20, case_54[0], case_54[3]);
        test_traverse<polygon, polygon, operation_union>::apply("54_iet_st", 2, 20, case_54[1], case_54[2]);
    }
    test_traverse<polygon, polygon, operation_union>::apply("54_iet_iet", 2, 20, case_54[1], case_54[3]);

    if (test_mixed)
    {
        test_traverse<polygon, polygon, operation_union>::apply("55_st_iet", 2, 18, case_55[0], case_55[3]);
        test_traverse<polygon, polygon, operation_union>::apply("55_iet_st", 2, 18, case_55[1], case_55[2]);
        // moved to mixed
        test_traverse<polygon, polygon, operation_union>::apply("55_iet_iet", 3, 18, case_55[1], case_55[3]);
    }

    // 55-60
    if (test_self_tangencies)
    {
        // 55 with both input polygons having self tangencies (st_st) generates 1 correct shape
        test_traverse<polygon, polygon, operation_union>::apply("55_st_st", 1, 18, case_55[0], case_55[2]);
        // 55 with one of them self-tangency, other int/ext ring tangency generate 2 correct shapes

        test_traverse<polygon, polygon, operation_union>::apply("56", 2, 14, case_56[0], case_56[1]);
    }
    test_traverse<polygon, polygon, operation_union>::apply("57", 1, 14.029412, case_57[0], case_57[1]);

    if (test_self_tangencies)
    {
        test_traverse<polygon, polygon, operation_union>::apply("58_st",
            4, 12.16666, case_58[0], case_58[1]);
        test_traverse<polygon, polygon, operation_union>::apply("59_st",
            2, 17.208333, case_59[0], case_59[1]);
        test_traverse<polygon, polygon, operation_union>::apply("60_st",
            3, 19, case_60[0], case_60[1]);
    }
    test_traverse<polygon, polygon, operation_union>::apply("58_iet",
         4, 12.16666, case_58[0], case_58[2]);
    test_traverse<polygon, polygon, operation_union>::apply("59_iet",
        1, -3.791666, // 2, 17.208333), outer ring (ii/ix) is done by ASSEMBLE
        case_59[0], case_59[2]);
    test_traverse<polygon, polygon, operation_union>::apply("60_iet",
        3, 19, case_60[0], case_60[2]);
    test_traverse<polygon, polygon, operation_union>::apply("61_st",
        1, 4, case_61[0], case_61[1]);

    test_traverse<polygon, polygon, operation_union>::apply("70",
        1, 9, case_70[0], case_70[1]);
    test_traverse<polygon, polygon, operation_union>::apply("71",
        2, 9, case_71[0], case_71[1]);
    test_traverse<polygon, polygon, operation_union>::apply("72",
        1, 10.65, case_72[0], case_72[1]);



    // other
    test_traverse<polygon, polygon, operation_intersection>::apply("collinear_overlaps",
        1, 24,
        collinear_overlaps[0], collinear_overlaps[1]);
    test_traverse<polygon, polygon, operation_union>::apply("collinear_overlaps",
        1, 50,
        collinear_overlaps[0], collinear_overlaps[1]);

    test_traverse<polygon, polygon, operation_intersection>::apply("many_situations", 1, 184, case_many_situations[0], case_many_situations[1]);
    test_traverse<polygon, polygon, operation_union>::apply("many_situations",
        1, 207, case_many_situations[0], case_many_situations[1]);


    // From "intersection piets", robustness test.
    // This all went wrong in the past
    // (when not all cases (get_turns) where implemented,
    //   especially important are some collinear (opposite) cases)
    test_traverse<polygon, polygon, operation_union>::apply("pie_16_4_12",
        1, 3669665.5, pie_16_4_12[0], pie_16_4_12[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_23_21_12_500",
        1, 6295516.7185, pie_23_21_12_500[0], pie_23_21_12_500[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_23_23_3_2000",
        1, 7118735.0530, pie_23_23_3_2000[0], pie_23_23_3_2000[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_23_16_16",
        1, 5710474.5406, pie_23_16_16[0], pie_23_16_16[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_16_2_15_0",
        1, 3833641.5, pie_16_2_15_0[0], pie_16_2_15_0[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_4_13_15",
        1, 2208122.43322, pie_4_13_15[0], pie_4_13_15[1]);
    test_traverse<polygon, polygon, operation_union>::apply("pie_20_20_7_100",
        1, 5577158.72823, pie_20_20_7_100[0], pie_20_20_7_100[1]);

    /*
    if (test_not_valid)
    {
    test_traverse<polygon, polygon, operation_union>::apply("pie_5_12_12_0_7s",
        1, 3271710.48516, pie_5_12_12_0_7s[0], pie_5_12_12_0_7s[1]);
    }
    */

    static const bool is_float
        = boost::is_same<T, float>::value;
    static const bool is_double
        = boost::is_same<T, double>::value
        || boost::is_same<T, long double>::value;

    static const double float_might_deviate_more = is_float ? 0.1 : 0.001; // In some cases up to 1 promille permitted

    // GCC: does not everywhere handle float correctly (in our algorithms)
    bool const is_float_on_non_msvc =
#if defined(_MSC_VER)
        false;
#else
        is_float;
#endif



    // From "Random Ellipse Stars", robustness test.
    // This all went wrong in the past
    // when using Determinant/ra/rb and comparing with 0/1
    // Solved now by avoiding determinant / using sides
    // ("hv" means "high volume")
    {
        double deviation = is_float ? 0.01 : 0.001;
        test_traverse<polygon, polygon, operation_union>::apply("hv1", 1, 1624.508688461573, hv_1[0], hv_1[1], deviation);
        test_traverse<polygon, polygon, operation_intersection>::apply("hv1", 1, 1622.7200125123809, hv_1[0], hv_1[1], deviation);

        test_traverse<polygon, polygon, operation_union>::apply("hv2", 1, 1622.9193392726836, hv_2[0], hv_2[1], deviation);
        test_traverse<polygon, polygon, operation_intersection>::apply("hv2", 1, 1622.1733591429329, hv_2[0], hv_2[1], deviation);

        test_traverse<polygon, polygon, operation_union>::apply("hv3", 1, 1624.22079205664, hv_3[0], hv_3[1], deviation);
        test_traverse<polygon, polygon, operation_intersection>::apply("hv3", 1, 1623.8265057282042, hv_3[0], hv_3[1], deviation);


        if (! is_float)
        {
            test_traverse<polygon, polygon, operation_union>::apply("hv4", 1, 1626.5146964146334, hv_4[0], hv_4[1], deviation);
            test_traverse<polygon, polygon, operation_intersection>::apply("hv4", 1, 1626.2580370864305, hv_4[0], hv_4[1], deviation);
            test_traverse<polygon, polygon, operation_union>::apply("hv5", 1, 1624.2158307261871, hv_5[0], hv_5[1], deviation);
            test_traverse<polygon, polygon, operation_intersection>::apply("hv5", 1, 1623.4506071521519, hv_5[0], hv_5[1], deviation);

            // Case 2009-12-07
            test_traverse<polygon, polygon, operation_intersection>::apply("hv6", 1, 1604.6318757402121, hv_6[0], hv_6[1], deviation);
            test_traverse<polygon, polygon, operation_union>::apply("hv6", 1, 1790.091872401327, hv_6[0], hv_6[1], deviation);

            // Case 2009-12-08, needing sorting on side in enrich_intersection_points
            test_traverse<polygon, polygon, operation_union>::apply("hv7", 1, 1624.5779453641017, hv_7[0], hv_7[1], deviation);
            test_traverse<polygon, polygon, operation_intersection>::apply("hv7", 1, 1623.6936420295772, hv_7[0], hv_7[1], deviation);
        }
    }

    // From "Random Ellipse Stars", robustness test.
    // This all went wrong in the past when distances (see below) were zero (dz)
    // "Distance zero", dz, means: the distance between two intersection points
    // on a same segment is 0, therefore it can't be sorted normally, therefore
    // the chance is 50% that the segments are not sorted correctly and the wrong
    // decision is taken.
    // Solved now (by sorting on sides in those cases)
    if (! is_float_on_non_msvc)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("dz_1",
                3, 16.887537949472005, dz_1[0], dz_1[1]);
        test_traverse<polygon, polygon, operation_union>::apply("dz_1",
                3, 1444.2621305732864, dz_1[0], dz_1[1]);

        test_traverse<polygon, polygon, operation_intersection>::apply("dz_2",
                2, 68.678921274288541, dz_2[0], dz_2[1]);
        test_traverse<polygon, polygon, operation_union>::apply("dz_2",
                2, 1505.4202304878663, dz_2[0], dz_2[1]);

        test_traverse<polygon, polygon, operation_intersection>::apply("dz_3",
                6, 192.49316937645651, dz_3[0], dz_3[1]);
        test_traverse<polygon, polygon, operation_union>::apply("dz_3",
                6, 1446.496005965641, dz_3[0], dz_3[1]);

        test_traverse<polygon, polygon, operation_intersection>::apply("dz_4",
                1, 473.59423868207693, dz_4[0], dz_4[1]);
        test_traverse<polygon, polygon, operation_union>::apply("dz_4",
                1, 1871.6125138873476, dz_4[0], dz_4[1]);
    }

    // Real-life problems

    // SNL (Subsidiestelsel Natuur & Landschap - verAANnen)

    if (! is_float_on_non_msvc)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("snl-1",
            2, 286.996062095888,
            snl_1[0], snl_1[1],
            float_might_deviate_more);

        test_traverse<polygon, polygon, operation_union>::apply("snl-1",
            2, 51997.5408506132,
            snl_1[0], snl_1[1],
            float_might_deviate_more);
    }

    {
        test_traverse<polygon, polygon, operation_intersection>::apply("isov",
                1, 88.1920, isovist[0], isovist[1],
                float_might_deviate_more);
        test_traverse<polygon, polygon, operation_union>::apply("isov",
                1, 313.3604, isovist[0], isovist[1],
                float_might_deviate_more);
    }

    // GEOS tests
    if (! is_float)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("geos_1_test_overlay",
                1, 3461.02330171138, geos_1_test_overlay[0], geos_1_test_overlay[1]);
        test_traverse<polygon, polygon, operation_union>::apply("geos_1_test_overlay",
                1, 3461.31592235516, geos_1_test_overlay[0], geos_1_test_overlay[1]);

        if (! is_double)
        {
            test_traverse<polygon, polygon, operation_intersection>::apply("geos_2",
                    2, 2.157e-6, // by bg/ttmath; sql server reports: 2.20530228034477E-06
                    geos_2[0], geos_2[1]);
        }
        test_traverse<polygon, polygon, operation_union>::apply("geos_2",
                1, 350.550662845485,
                geos_2[0], geos_2[1]);
    }

    if (! is_float && ! is_double)
    {
        test_traverse<polygon, polygon, operation_intersection>::apply("geos_3",
                1, 2.484885e-7,
                geos_3[0], geos_3[1]);
    }

    if (! is_float_on_non_msvc)
    {
        // Sometimes output is reported as 29229056
        test_traverse<polygon, polygon, operation_union>::apply("geos_3",
                1, 29391548.5,
                geos_3[0], geos_3[1],
                float_might_deviate_more);

        // Sometimes output is reported as 0.078125
        test_traverse<polygon, polygon, operation_intersection>::apply("geos_4",
                1, 0.0836884926070727,
                geos_4[0], geos_4[1],
                float_might_deviate_more);
    }

    test_traverse<polygon, polygon, operation_union>::apply("geos_4",
            1, 2304.41633605957,
            geos_4[0], geos_4[1]);
	
    if (! is_float)
    {

#if defined(_MSC_VER)
        double const expected = if_typed_tt<T>(3.63794e-17, 0.0);
        int expected_count = if_typed_tt<T>(1, 0);
#else
        double const expected = if_typed<T, long double>(2.77555756156289135106e-17, 0.0);
        int expected_count = if_typed<T, long double>(1, 0);
#endif

        // Calculate intersection/union of two triangles. Robustness case.
        // ttmath can form a very small intersection triangle 
        // (which is even not accomplished by SQL Server/PostGIS)
        std::string const caseid = "ggl_list_20110820_christophe";
        test_traverse<polygon, polygon, operation_intersection>::apply(caseid, 
            expected_count, expected,
            ggl_list_20110820_christophe[0], ggl_list_20110820_christophe[1]);
        test_traverse<polygon, polygon, operation_union>::apply(caseid, 
            1, 67.3550722317627, 
            ggl_list_20110820_christophe[0], ggl_list_20110820_christophe[1]);
    }

    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_f", 
        1, 4.60853, 
        buffer_rt_f[0], buffer_rt_f[1]);
    test_traverse<polygon, polygon, operation_intersection>::apply("buffer_rt_f", 
        1, 0.0002943725152286, 
        buffer_rt_f[0], buffer_rt_f[1], 0.01);

    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_g", 
        1, 16.571, 
        buffer_rt_g[0], buffer_rt_g[1]);

    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_g_boxes1", 
        1, 20, 
        buffer_rt_g_boxes[0], buffer_rt_g_boxes[1]);
    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_g_boxes2", 
        1, 24, 
        buffer_rt_g_boxes[0], buffer_rt_g_boxes[2]);
    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_g_boxes3", 
        1, 28, 
        buffer_rt_g_boxes[0], buffer_rt_g_boxes[3]);

    test_traverse<polygon, polygon, operation_union>::apply("buffer_rt_g_boxes43", 
        1, 30, 
        buffer_rt_g_boxes[4], buffer_rt_g_boxes[3]);


    if (boost::is_same<T, double>::value)
    {
        test_traverse<polygon, polygon, operation_union>::apply("buffer_mp2", 
                2, 36.7535642, buffer_mp2[0], buffer_mp2[1], 0.01);
    }
    test_traverse<polygon, polygon, operation_union>::apply("collinear_opposite_rr",
            1, 6.41, collinear_opposite_right[0], collinear_opposite_right[1]);
    test_traverse<polygon, polygon, operation_union>::apply("collinear_opposite_ll",
            1, 11.75, collinear_opposite_left[0], collinear_opposite_left[1]);
    test_traverse<polygon, polygon, operation_union>::apply("collinear_opposite_ss",
            1, 6, collinear_opposite_straight[0], collinear_opposite_straight[1]);
    test_traverse<polygon, polygon, operation_union>::apply("collinear_opposite_lr",
            1, 8.66, collinear_opposite_left[0], collinear_opposite_right[1]);
    test_traverse<polygon, polygon, operation_union>::apply("collinear_opposite_rl",
            1, 9, collinear_opposite_right[0], collinear_opposite_left[1]);

    test_traverse<polygon, polygon, operation_intersection>::apply("ticket_7462", 1, 0.220582, ticket_7462[0], ticket_7462[1]);

#ifdef BOOST_GEOMETRY_OVERLAY_NO_THROW
    {
        // NOTE: currently throws (normally)
        std::string caseid = "ggl_list_20120229_volker";
        test_traverse<polygon, polygon, operation_union>::apply(caseid, 
            1, 99, 
            ggl_list_20120229_volker[0], ggl_list_20120229_volker[1]);
        test_traverse<polygon, polygon, operation_intersection>::apply(caseid, 
            1, 99, 
            ggl_list_20120229_volker[0], ggl_list_20120229_volker[1]);
        caseid = "ggl_list_20120229_volker_2";
        test_traverse<polygon, polygon, operation_union>::apply(caseid, 
            1, 99, 
            ggl_list_20120229_volker[2], ggl_list_20120229_volker[1]);
        test_traverse<polygon, polygon, operation_intersection>::apply(caseid, 
            1, 99, 
            ggl_list_20120229_volker[2], ggl_list_20120229_volker[1]);
    }
#endif
}

template <typename T>
void test_open()
{
    using namespace bg::detail::overlay;

    typedef bg::model::polygon
        <
            bg::model::point<T, 2, bg::cs::cartesian>,
            true, false
        > polygon;

    test_traverse<polygon, polygon, operation_intersection>::apply("open_1", 1, 5.4736,
        open_case_1[0], open_case_1[1]);
    test_traverse<polygon, polygon, operation_union>::apply("open_1", 1, 11.5264,
        open_case_1[0], open_case_1[1]);
}


template <typename T>
void test_ccw()
{
    using namespace bg::detail::overlay;

    typedef bg::model::polygon
        <
            bg::model::point<T, 2, bg::cs::cartesian>,
            false, true
        > polygon;

    test_traverse<polygon, polygon, operation_intersection, true, true>::apply("ccw_1", 1, 5.4736,
        ccw_case_1[0], ccw_case_1[1]);
    test_traverse<polygon, polygon, operation_union, true, true>::apply("ccw_1", 1, 11.5264,
        ccw_case_1[0], ccw_case_1[1]);
}



int test_main(int, char* [])
{
#if defined(BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE)
    test_all<double>();
#else
    test_all<float>();
    test_all<double>();
    test_open<double>();
    test_ccw<double>();

#if ! defined(_MSC_VER)
    test_all<long double>();
#endif

#ifdef HAVE_TTMATH
    test_all<ttmath_big>();
#endif
#endif

    return 0;
 }

#endif
