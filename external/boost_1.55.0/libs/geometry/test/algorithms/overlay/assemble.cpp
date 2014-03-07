// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <string>


#include <geometry_test_common.hpp>

#include <boost/foreach.hpp>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/union.hpp>
#include <boost/geometry/algorithms/difference.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>



#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

template <typename Geometry>
inline void test_assemble(std::string const& id, Geometry const& p, Geometry const& q, char operation = 'i')
{

    std::vector<Geometry> u, i, d1, d2;
    bg::detail::union_::union_insert<Geometry>(p, q, std::back_inserter(u));
    bg::detail::intersection::intersection_insert<Geometry>(p, q, std::back_inserter(i));
    bg::detail::difference::difference_insert<Geometry>(p, q, std::back_inserter(d1));
    bg::detail::difference::difference_insert<Geometry>(q, p, std::back_inserter(d2));

    if (operation == 'i')
    {
        typedef typename bg::default_area_result<Geometry>::type type;
        type area_p = bg::area(p);
        type area_q = bg::area(q);

        type area_i = 0, area_u = 0, area_d1 = 0, area_d2 = 0;

        BOOST_FOREACH(Geometry const& g, u)
        {
            area_u += bg::area(g);
        }
        BOOST_FOREACH(Geometry const& g, i)
        {
            area_i += bg::area(g);
        }
        BOOST_FOREACH(Geometry const& g, d1)
        {
            area_d1 += bg::area(g);
        }
        BOOST_FOREACH(Geometry const& g, d2)
        {
            area_d2 += bg::area(g);
        }


        type diff = (area_p + area_q) - area_u - area_i;
        type diff_d1 = (area_u - area_q) - area_d1;
        type diff_d2 = (area_u - area_p) - area_d2;

        BOOST_CHECK_CLOSE(diff, 0.0, 0.001);

        // Gives small deviations on gcc:
        // difference{0.001%} between diff_d1{1.1102230246251565e-016} and 0.0{0} exceeds 0.001%
        //BOOST_CHECK_CLOSE(diff_d1, 0.0, 0.001);
        //BOOST_CHECK_CLOSE(diff_d2, 0.0, 0.001);

        bool ok = abs(diff) < 0.001
            || abs(diff_d1) < 0.001
            || abs(diff_d2) < 0.001;

        BOOST_CHECK_MESSAGE(ok,
            id << " diff:  "
                << diff << " d1: "
                << diff_d1 << " d2: "
                << diff_d2);
    }

#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename << "assemble_" << id << "_" << operation << ".svg";
        std::ofstream svg(filename.str().c_str());

        bg::svg_mapper<typename bg::point_type<Geometry>::type> mapper(svg, 500, 500);
        mapper.add(p);
        mapper.add(q);
        mapper.map(p, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:3");
        mapper.map(q, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:3");
        std::string linestyle = "opacity:0.7;fill:none;stroke-opacity:1;stroke-miterlimit:4;";

        std::vector<Geometry> const& v = operation == 'i' ? i
            : operation == 'u' ? u
            : operation == 'd' ? d1
            : d2
            ;

        BOOST_FOREACH(Geometry const& geometry, v)
        {
            mapper.map(geometry,
                linestyle + "stroke-width:3;stroke-linejoin:round;stroke-linecap:square;stroke-dasharray:12,12;stroke:rgb(255,0,0);");
        }
    }
#endif
}

template <typename Polygon>
inline bool int_ok(Polygon const& poly)
{

    typename bg::point_type<Polygon>::type const& pi =
        bg::interior_rings(poly)[0].front();

    return bg::within(pi, bg::exterior_ring(poly));
}


template <typename T>
void generate()
{

    static std::string exteriors[4] = {
            "(0 0,0 10,10 10,10 0,0 0)",
            "(1 1,1 9,8 9,8 1,1 1)",
            "(2 0.5, 0.5 2,0.5 8,2 9.5,6 9.5,8.5 8,8.5 2,7 0.5,2 0.5)",
            "(3 3,3 7,6 7,6 3,3 3)"
    };
    static std::string interiors[4] = {
            "(2 2,2 8,7 8,7 2,2 2)",
            "(8.5 1,8.5 2,9.5 2,9.5 1,8.5 1)",
            "(4 4,4 5,5 5,5 4,4 4)",
            "(6 4,6 5,9 5,9 4,6 4)"
    };
    for (int pe = 0; pe < 4; pe++)
    {
        for (int qe = 0; qe < 4; qe++)
        {
            for (int pi = 0; pi < 4; pi++)
            {
                for (int qi = 0; qi < 4; qi++)
                {
                    std::string ps = "POLYGON(" + exteriors[pe] + "," + interiors[pi] + ")";
                    std::string qs = "POLYGON(" + exteriors[qe] + "," + interiors[qi] + ")";

                    typedef bg::model::d2::point_xy<T> point_type;
                    bg::model::polygon<point_type> p, q;
                    bg::read_wkt(ps, p);
                    bg::read_wkt(qs, q);
                    bg::correct(p);
                    bg::correct(q);
                    if (! bg::intersects(p)
                        && ! bg::intersects(q)
                        && int_ok(p)
                        && int_ok(q)
                        )
                    {
                        std::ostringstream out;
                        out << pe << qe << pi << qi;
                        test_assemble(out.str(), p, q);

#if defined(TEST_WITH_SVG)
                        test_assemble(out.str(), p, q, 'u');
                        test_assemble(out.str(), p, q, 'd');
                        test_assemble(out.str(), p, q, 'r');
#endif
                    }
                }
            }
        }
    }
}


#if ! defined(GEOMETRY_TEST_MULTI)
int test_main(int, char* [])
{
    generate<double>();
    return 0;
}
#endif
