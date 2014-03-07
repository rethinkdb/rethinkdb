// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>

#include <geometry_test_common.hpp>

#define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER


#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>

#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>

#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

#include <algorithms/overlay/overlay_cases.hpp>




// To test that "get_turns" can be called using additional information
template <typename P>
struct my_turn_op : public bg::detail::overlay::turn_operation
{
};

namespace detail
{

struct test_get_turns
{
    template<typename G1, typename G2>
    static void apply(std::string const& id,
            std::size_t expected_count,
            G1 const& g1, G2 const& g2, double precision)
    {
            typedef bg::detail::overlay::turn_info
            <
                typename bg::point_type<G2>::type
            > turn_info;
        std::vector<turn_info> turns;

        bg::detail::get_turns::no_interrupt_policy policy;
        bg::get_turns<false, false, bg::detail::overlay::assign_null_policy>(g1, g2, turns, policy);

        BOOST_CHECK_MESSAGE(
            expected_count == boost::size(turns),
                "get_turns: " << id
                << " #turns expected: " << expected_count
                << " detected: " << boost::size(turns)
                << " type: " << string_from_type
                    <typename bg::coordinate_type<G1>::type>::name()
                );

#if defined(TEST_WITH_SVG)
        {
            typedef typename bg::coordinate_type<G1>::type coordinate_type;
            std::map<std::pair<coordinate_type, coordinate_type>, int> offsets;
            std::ostringstream filename;
            filename << "get_turns_" << id
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

            int index = 0;
            BOOST_FOREACH(turn_info const& turn, turns)
            {
                mapper.map(turn.point, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");

                // Map characteristics
                std::pair<coordinate_type, coordinate_type> p
                    = std::make_pair(bg::get<0>(turn.point), bg::get<1>(turn.point));

                {
                    std::ostringstream out;
                    out << index
                        << ": " << bg::operation_char(turn.operations[0].operation)
                        << " " << bg::operation_char(turn.operations[1].operation)
                        << " (" << bg::method_char(turn.method) << ")"
                        << (turn.is_discarded() ? " (discarded) " : turn.blocked() ? " (blocked)" : "")
                        ;

                    offsets[p] += 10;
                    int offset = offsets[p];
                    mapper.text(turn.point, out.str(),
                        "fill:rgb(0,0,0);font-family:Arial;font-size:8px",
                        5, offset);
                }

                ++index;
            }
        }
#endif
    }
};

}

template<typename G1, typename G2>
struct test_get_turns
{
    inline static void apply(std::string const& id, std::size_t expected_count, 
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

        // Try the overlay-function in both ways
        std::string caseid = id;
        //goto case_reversed;

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        std::cout << std::endl << std::endl << "# " << caseid << std::endl;
#endif
        detail::test_get_turns::apply(caseid, expected_count, g1, g2, precision);

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        return;
#endif

    //case_reversed:
#if ! defined(BOOST_GEOMETRY_TEST_OVERLAY_NOT_EXCHANGED)
        caseid = id + "_rev";
#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
        std::cout << std::endl << std::endl << "# " << caseid << std::endl;
#endif

        detail::test_get_turns::apply(caseid, expected_count, g2, g1, precision);
#endif
    }
};

#if ! defined(GEOMETRY_TEST_MULTI)
template <typename T>
void test_all()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> P;
    typedef bg::model::polygon<P> polygon;
    typedef bg::model::linestring<P> linestring;
    typedef bg::model::box<P> box;

    // Expected count, average x, average y
    typedef boost::tuple<int> Tuple;

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
    std::cout << string_from_type<T>::name() << std::endl;
#endif


    // snl
    /*
    test_get_turns<polygon, polygon>::apply("snl_2",
        5,
        //snl-1
        //"POLYGON((182467 605842,182480 605954,182557 605958,182571 605958,182585 605958,182579 605843,182559 605838,182467 605842))",
        //"POLYGON((182499 605955,182511 605960,182536 605974,182536 605981,182536 606006,182563 606006,182610 605985,182613 605976,182620 605948,182628 605937,182631 605924,182639 605889,182634 605885,182603 605848,182579 605843,182585 605958,182571 605958,182557 605958,182499 605955))");
        //snl-2
        //"POLYGON((120812 525783,120845 525792,120821 525842,120789 525826,120818 525849,120831 525854,120875 525875,120887 525881,120887 525881,120920 525834,120920 525834,120811 525772,120789 525826,120812 525783))",
        //"POLYGON((120789 525826,120812 525783,120845 525792,120821 525842,120789 525826,120818 525849,120831 525854,120875 525875,120923 525836,120811 525772,120789 525826))"
        //snl-4
        "POLYGON((184913.4512400339881423860788345336914 606985.779408219968900084495544433594,184912.8999999999941792339086532592773 606987.145999999949708580970764160156,184904.4135310589917935431003570556641 606987.651360383024439215660095214844,184901.847619076987029984593391418457 607014.593436188995838165283203125,184916.3977574919990729540586471557617 607021.060164373018778860569000244141,184927.7147701499925460666418075561523 607008.126435620011761784553527832031,184926.0980706939881201833486557006836 606998.426238880958408117294311523438,184913.4512400339881423860788345336914 606985.779408219968900084495544433594),(184907.5560000000114087015390396118164 607013.300999999977648258209228515625,184905.7820000000065192580223083496094 607009.971999999950639903545379638672,184906.0039999999862629920244216918945 607005.978000000002793967723846435547,184908.4439999999885912984609603881836 606998.876999999978579580783843994141,184912.2149999999965075403451919555664 606994.217999999993480741977691650391,184919.3140000000130385160446166992188 606993.996000000042840838432312011719,184922.4200000000128056854009628295898 606995.770999999949708580970764160156,184925.7470000000030267983675003051758 606998.876999999978579580783843994141,184926.4130000000004656612873077392578 607002.871999999973922967910766601563,184925.7470000000030267983675003051758 607007.753000000026077032089233398438,184922.4200000000128056854009628295898 607012.190999999991618096828460693359,184917.0959999999904539436101913452148 607015.297999999951571226119995117188,184911.7710000000079162418842315673828 607015.297999999951571226119995117188,184907.5560000000114087015390396118164 607013.300999999977648258209228515625))",
        "POLYGON((184861.1180000010062940418720245361328 606901.158000000054016709327697753906,184893.7870000000111758708953857421875 606898.482999998959712684154510498047,184925.0430000009946525096893310546875 606913.399999998975545167922973632813,184927.1739999990095384418964385986328 606951.758999999961815774440765380859,184912.8999999990046489983797073364258 606987.146000002045184373855590820313,184877.8700000010139774531126022338867 606989.232000001007691025733947753906,184885.1030000000027939677238464355469 607023.773999999975785613059997558594,184899.0579999980109278112649917602539 607022.743000000948086380958557128906,184906.0080000009911600500345230102539 607044.947999999043531715869903564453,184966.4649999999965075403451919555664 607025.020000000018626451492309570313,184968.4420000019890721887350082397461 606961.300000000977888703346252441406,185024.7679999989923089742660522460938 606947.401999998954124748706817626953,185024.5439999999944120645523071289063 606941.354999999981373548507690429688,185027.0069999989937059581279754638672 606937.322999999043531715869903564453,185030.3660000000090803951025009155273 606934.186999998986721038818359375,185035.5159999990137293934822082519531 606933.962999999988824129104614257813,185040.4420000019890721887350082397461 606935.530999999027699232101440429688,185042.905000000988366082310676574707 606939.114999998011626303195953369141,185088.3640000000013969838619232177734 606931.385000001988373696804046630859,185089.1389999990060459822416305541992 607015.508999999961815774440765380859,185095.1999999989930074661970138549805 607011.300000000977888703346252441406,185118.8269999999902211129665374755859 606995.545000002020969986915588378906,185126.813000001013278961181640625 606991.9950000010430812835693359375,185177.7270000019925646483898162841797 606973.798999998951330780982971191406,185181.4820000010076910257339477539063 606966.67599999904632568359375,185193.5709999990067444741725921630859 606977.795000002020969986915588378906,185193.710999998991610482335090637207 606960.300000000977888703346252441406,185189.3520000019925646483898162841797 606779.020000000018626451492309570313,185167.5150000010035000741481781005859 606783.844000000972300767898559570313,185086.9600000010104849934577941894531 606801.241000000038184225559234619141,185011.7069999990053474903106689453125 606817.809000000008381903171539306641,185000 606819.304000001051463186740875244141,184994.0340000019932631403207778930664 606819.793999999994412064552307128906,184976.3979999980074353516101837158203 606819.572000000975094735622406005859,184956.6539999989909119904041290283203 606817.1310000009834766387939453125,184934.9129999990109354257583618164063 606813.136999998008832335472106933594,184893.0969999989902134984731674194336 606804.927000000956468284130096435547,184884.4450000000069849193096160888672 606831.555000000051222741603851318359,184866.9189999999944120645523071289063 606883.480999998981133103370666503906,184861.1180000010062940418720245361328 606901.158000000054016709327697753906),(184907.5560000019904691725969314575195 607013.30099999904632568359375,184905.7820000019855797290802001953125 607009.971999999019317328929901123047,184906.0040000010048970580101013183594 607005.978000000002793967723846435547,184908.4439999980095308274030685424805 606998.876999999978579580783843994141,184912.2149999999965075403451919555664 606994.217999998014420270919799804688,184919.3139999989944044500589370727539 606993.995999998995102941989898681641,184922.420000001991866156458854675293 606995.771000002045184373855590820313,184925.7470000009925570338964462280273 606998.876999999978579580783843994141,184926.4129999990109354257583618164063 607002.872000001021660864353179931641,184925.7470000009925570338964462280273 607007.752999998978339135646820068359,184922.420000001991866156458854675293 607012.190999999991618096828460693359,184917.0960000010090880095958709716797 607015.297999999951571226119995117188,184911.7710000019869767129421234130859 607015.297999999951571226119995117188,184907.5560000019904691725969314575195 607013.30099999904632568359375))"
        );


    return;
    */

    // 1-6
    test_get_turns<polygon, polygon>::apply("1", 6, case_1[0], case_1[1]);
    test_get_turns<polygon, polygon>::apply("2", 8, case_2[0], case_2[1]);
    test_get_turns<polygon, polygon>::apply("3", 4, case_3[0], case_3[1]);
    test_get_turns<polygon, polygon>::apply("4", 12, case_4[0], case_4[1]);
    test_get_turns<polygon, polygon>::apply("5", 17, case_5[0], case_5[1]);
    test_get_turns<polygon, polygon>::apply("6", 3, case_6[0], case_6[1]);

    // 7-12
    test_get_turns<polygon, polygon>::apply("7", 2, case_7[0], case_7[1]);
    test_get_turns<polygon, polygon>::apply("8", 2, case_8[0], case_8[1]);
    test_get_turns<polygon, polygon>::apply("9", 1, case_9[0], case_9[1]);
    test_get_turns<polygon, polygon>::apply("10", 3, case_10[0], case_10[1]);
    test_get_turns<polygon, polygon>::apply("11", 1, case_11[0], case_11[1]);
    test_get_turns<polygon, polygon>::apply("12", 8, case_12[0], case_12[1]);

    // 13-18
    test_get_turns<polygon, polygon>::apply("13", 2, case_13[0], case_13[1]);
    test_get_turns<polygon, polygon>::apply("14", 2, case_14[0], case_14[1]);
    test_get_turns<polygon, polygon>::apply("15", 2, case_15[0], case_15[1]);
    test_get_turns<polygon, polygon>::apply("16", 4, case_16[0], case_16[1]);
    test_get_turns<polygon, polygon>::apply("17", 2, case_17[0], case_17[1]);
    test_get_turns<polygon, polygon>::apply("18", 4, case_18[0], case_18[1]);

    // 19-24
    test_get_turns<polygon, polygon>::apply("19", 2, case_19[0], case_19[1]);
    test_get_turns<polygon, polygon>::apply("20", 3, case_20[0], case_20[1]);
    test_get_turns<polygon, polygon>::apply("21", 3, case_21[0], case_21[1]);
    test_get_turns<polygon, polygon>::apply("22", 1, case_22[0], case_22[1]);
    test_get_turns<polygon, polygon>::apply("23", 2, case_23[0], case_23[1]);
    test_get_turns<polygon, polygon>::apply("24", 1, case_24[0], case_24[1]);

    // 25-30
    test_get_turns<polygon, polygon>::apply("25", 1, case_25[0], case_25[1]);
    test_get_turns<polygon, polygon>::apply("26", 1, case_26[0], case_26[1]);
    test_get_turns<polygon, polygon>::apply("27", 2, case_27[0], case_27[1]);
    test_get_turns<polygon, polygon>::apply("28", 2, case_28[0], case_28[1]);
    test_get_turns<polygon, polygon>::apply("29", 2, case_29[0], case_29[1]);
    test_get_turns<polygon, polygon>::apply("30", 2, case_30[0], case_30[1]);

    // 31-36
    test_get_turns<polygon, polygon>::apply("31", 1, case_31[0], case_31[1]);
    test_get_turns<polygon, polygon>::apply("32", 1, case_32[0], case_32[1]);
    test_get_turns<polygon, polygon>::apply("33", 1, case_33[0], case_33[1]);
    test_get_turns<polygon, polygon>::apply("34", 2, case_34[0], case_34[1]);
    test_get_turns<polygon, polygon>::apply("35", 1, case_35[0], case_35[1]);
    test_get_turns<polygon, polygon>::apply("36", 3, case_36[0], case_36[1]);

    // 37-42
    test_get_turns<polygon, polygon>::apply("37", 3, case_37[0], case_37[1]);
    test_get_turns<polygon, polygon>::apply("38", 3, case_38[0], case_38[1]);
    test_get_turns<polygon, polygon>::apply("39", 4, case_39[0], case_39[1]);
    test_get_turns<polygon, polygon>::apply("40", 3, case_40[0], case_40[1]);
    test_get_turns<polygon, polygon>::apply("41", 5, case_41[0], case_41[1]);
    test_get_turns<polygon, polygon>::apply("42", 5, case_42[0], case_42[1]);

    // 43-48
    test_get_turns<polygon, polygon>::apply("43", 4, case_43[0], case_43[1]);
    test_get_turns<polygon, polygon>::apply("44", 4, case_44[0], case_44[1]);
    test_get_turns<polygon, polygon>::apply("45", 4, case_45[0], case_45[1]);
    test_get_turns<polygon, polygon>::apply("46", 4, case_46[0], case_46[1]);
    test_get_turns<polygon, polygon>::apply("47", 5, case_47[0], case_47[1]);

    // 49-54
    test_get_turns<polygon, polygon>::apply("50", 4, case_50[0], case_50[1]);
    test_get_turns<polygon, polygon>::apply("51", 3, case_51[0], case_51[1]);
    test_get_turns<polygon, polygon>::apply("52", 8, case_52[0], case_52[1]);
    // A touching point interior/ring exterior/ring can be represented in two ways:
    test_get_turns<polygon, polygon>::apply("53a", 4, case_53[0], case_53[1]);
    test_get_turns<polygon, polygon>::apply("53b", 4, case_53[0], case_53[2]);
    test_get_turns<polygon, polygon>::apply("54aa", 13, case_54[0], case_54[2]);
    test_get_turns<polygon, polygon>::apply("54ab", 13, case_54[0], case_54[3]);
    test_get_turns<polygon, polygon>::apply("54ba", 13, case_54[1], case_54[2]);
    test_get_turns<polygon, polygon>::apply("54bb", 13, case_54[1], case_54[3]);

    test_get_turns<polygon, polygon>::apply("55", 12, case_55[0], case_55[1]);
    test_get_turns<polygon, polygon>::apply("56", 9, case_56[0], case_56[1]);


    // other
    test_get_turns<polygon, polygon>::apply("many_situations", 31, case_many_situations[0], case_many_situations[1]);


    // ticket#17
    test_get_turns<polygon, box>::apply("ticket_17", 6, ticket_17[0], ticket_17[1]);

    // GGL-mailing list
    test_get_turns<polygon, polygon>::apply("ggl_list_20110306_javier",
            4,
            ggl_list_20110306_javier[0], ggl_list_20110306_javier[1]);

#ifdef _MSC_VER // gcc returns 14 for float
    // test_get_turns<polygon, polygon>::apply("ggl_list_20110716_enrico",
            // 13,
            // ggl_list_20110716_enrico[0], ggl_list_20110716_enrico[1]);

#endif

    // pies
    test_get_turns<polygon, polygon>::apply("pie_23_16_16", 3, pie_23_16_16[0], pie_23_16_16[1]);
    test_get_turns<polygon, polygon>::apply("pie_16_4_12", 2, pie_16_4_12[0], pie_16_4_12[1]);
    test_get_turns<polygon, polygon>::apply("pie_4_13_15", 3, pie_4_13_15[0], pie_4_13_15[1]);
    test_get_turns<polygon, polygon>::apply("pie_16_2_15_0", 2, pie_16_2_15_0[0], pie_16_2_15_0[1]);
    test_get_turns<polygon, polygon>::apply("pie_20_20_7_100", 3, pie_20_20_7_100[0], pie_20_20_7_100[1]);
    test_get_turns<polygon, polygon>::apply("pie_23_23_3_2000", 5, pie_23_23_3_2000[0], pie_23_23_3_2000[1]);


    // line-line
    test_get_turns<linestring, linestring>::apply("lineline1", 3, line_line1[0], line_line1[1]);

    // line-polygon
    test_get_turns<linestring, polygon>::apply("line_poly1", 4, line_line1[0], case_1[1]);
    test_get_turns<linestring, polygon>::apply("line_poly2", 4, line_line1[1], case_1[0]);
    test_get_turns<polygon, linestring>::apply("poly_line", 4, case_1[1], line_line1[0]);
}


template <typename T>
void test_ccw()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> P;
    typedef bg::model::polygon<P, false, true> polygon;
    typedef boost::tuple<int> Tuple;


    test_get_turns<polygon, polygon>::apply("ccw_1",
                6,
                ccw_case_1[0], ccw_case_1[1]);

    test_get_turns<polygon, polygon>::apply("ccw_9",
                1,
                case_9[0], case_9[1]);

}

template <typename T>
void test_open()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> P;
    typedef bg::model::polygon<P, true, false> polygon;
    typedef boost::tuple<int> Tuple;

    test_get_turns<polygon, polygon>::apply("open_1",
                6,
                open_case_1[0], open_case_1[1]);

    test_get_turns<polygon, polygon>::apply("open_9",
                1,
                open_case_9[0], open_case_9[1]);
}



int test_main(int, char* [])
{
    test_all<float>();
    test_all<double>();
    test_ccw<double>();
    test_open<double>();

#if ! defined(_MSC_VER)
    test_all<long double>();
#endif

#if defined(HAVE_TTMATH)
    test_all<ttmath_big>();
#endif
    return 0;
}

#endif
