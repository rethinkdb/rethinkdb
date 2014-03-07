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

#if defined(_MSC_VER)
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#endif

//#define GEOMETRY_DEBUG_INTERSECTION


#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>

#include <boost/numeric_adaptor/numeric_adaptor.hpp>
#include <boost/numeric_adaptor/gmp_value_type.hpp>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/calculate_distance_policy.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>



#include <boost/geometry/io/wkt/wkt.hpp>

#define TEST_WITH_SVG


#if defined(TEST_WITH_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif



template <typename OutputType, typename G1, typename G2>
void test_traverse(std::string const& caseid, G1 const& g1, G2 const& g2)
{
    typedef bg::detail::intersection::intersection_point
        <typename bg::point_type<G2>::type> ip;
    typedef typename boost::range_const_iterator<std::vector<ip> >::type iterator;
    typedef std::vector<ip> ip_vector;
    ip_vector ips;

    typedef typename bg::strategy::side::services::default_strategy
            <
                typename bg::cs_tag<G1>::type
            >::type strategy_type;


    typedef bg::detail::overlay::traversal_turn_info
        <
            typename bg::point_type<G2>::type
        > turn_info;
    typedef typename boost::range_iterator<const std::vector<turn_info> >::type iterator;
    std::vector<turn_info> ips;

    bg::get_turns<false, false, bg::detail::overlay::calculate_distance_policy>(g1, g2, ips);
    bg::enrich_intersection_points(ips, g1, g2, strategy_type());

    typedef bg::model::ring<typename bg::point_type<G2>::type> ring_type;
    typedef std::vector<ring_type> out_vector;
    out_vector v;



    bg::traverse
        <
            strategy_type,
            ring_type
        >
        (
            g1, g2, -1, ips, std::back_inserter(v)
        );



#if defined(TEST_WITH_SVG)
    {
        std::ostringstream filename;
        filename << "intersection_" << caseid << ".svg";

        std::ofstream svg(filename.str().c_str());

        // Trick to have this always LongDouble
        //typedef bg::model::d2::point_xy<long double> P;
        typedef typename bg::point_type<G1>::type P;
        //typename bg::replace_point_type<G1, P>::type rg1;
        //typename bg::replace_point_type<G2, P>::type rg2;

        bg::svg_mapper<P> mapper(svg, 1000, 800);

        mapper.add(g1);
        mapper.add(g2);

        // Input shapes in green/blue
        mapper.map(g1, "opacity:0.8;fill:rgb(0,255,0);"
                "stroke:rgb(0,0,0);stroke-width:1");
        mapper.map(g2, "opacity:0.8;fill:rgb(0,0,255);"
                "stroke:rgb(0,0,0);stroke-width:1");

        // Traversal rings in red
        for (typename out_vector::const_iterator it = boost::begin(v);
            it != boost::end(v);
            ++it)
        {
            mapper.map(*it, "fill-opacity:0.1;stroke-opacity:0.9;"
                "fill:rgb(255,0,0);stroke:rgb(255,0,0);stroke-width:5");

            std::cout << bg::wkt(*it) << std::endl;
            std::cout << bg::area(*it) << std::endl;
        }

        // IP's in orange
        for (iterator it = boost::begin(ips); it != boost::end(ips); ++it)
        {
            mapper.map(it->point, "fill:rgb(255,128,0);"
                    "stroke:rgb(0,0,100);stroke-width:1");
        }
    }
#endif
}

template <typename OutputType, typename G1, typename G2>
void test_one(std::string const& caseid, std::string const& wkt1, std::string const& wkt2)
{
    G1 g1;
    bg::read_wkt(wkt1, g1);

    G2 g2;
    bg::read_wkt(wkt2, g2);

    bg::correct(g1);
    bg::correct(g2);
    std::cout << "area1 " << bg::area(g1) << " "  << " area2: "  << bg::area(g2) << std::endl;

    test_traverse<OutputType>(caseid, g1, g2);
}


#if ! defined(GEOMETRY_TEST_MULTI)

template <typename P>
void test_traverse_gmp(std::string const& caseid)
{
    typedef bg::model::polygon<P> polygon;
    std::cout << typeid(typename bg::coordinate_type<P>::type).name() << std::endl;
    std::cout << std::setprecision(30) << std::numeric_limits<float>::epsilon() << std::endl;
    std::cout << std::setprecision(30) << std::numeric_limits<double>::epsilon() << std::endl;
    std::cout << std::setprecision(30) << std::numeric_limits<long double>::epsilon() << std::endl;

    static std::string brandon[3] =
        {
            //37.43402099609375 1.470055103302002,
        "POLYGON((37.29449462890625 1.7902572154998779,37.000419616699219 1.664225697517395,37.140213012695313 1.3446992635726929,50.974888957147442 -30.277285722290763,57.297810222148939 -37.546793343968417,41.590042114257813 -7.2021245956420898,40.6978759765625 -5.4500408172607422,40.758884429931641 -5.418975830078125,42.577911376953125 -4.4901103973388672,42.577877044677734 -4.4900407791137695,42.699958801269531 -4.4278755187988281,46.523914387974358 -8.5152102535033496,47.585065917176543 -6.1314922196594779,45.389434814453125 -4.5143837928771973,46.296027072709599 -2.4984308554828116,37.29449462890625 1.7902572154998779))",
        "POLYGON((42.399410247802734 1.4956772327423096,42.721500396728516 2.2342472076416016,42.721500396728516 3.6584999561309814,51.20102152843122 7.1738039562841562,51.370888500897557 7.4163459734570729,37.43402099609375 1.470055103302002,37.29449462890625 1.7902572154998779,37.000419616699219 1.664225697517395,37.140213012695313 1.3446992635726929,36.954700469970703 1.2597870826721191,26.472516656201325 -3.5380830513658776,27.069889344709196 -4.2926591211028242,30.501169204711914 -2.3718316555023193,32.708126068115234 -2.3611266613006592,32.708126068115234 -2.3611700534820557,32.708168029785156 -2.3611698150634766,32.718830108642578 -4.3281683921813965,29.135100397190627 -8.9262827849488211,29.619997024536133 -9.5368013381958008,30.339155197143555 -8.9838371276855469,30.670633316040039 -8.8180980682373047,30.896280288696289 -9.1206979751586914,30.207040612748258 -10.275926149505661,30.947774887084961 -11.208560943603516,31.669155120849609 -10.653837203979492,32.000633239746094 -10.488097190856934,32.226280212402344 -10.790698051452637,31.682494778186321 -12.133624901803865,32.274600982666016 -12.879127502441406,32.998821258544922 -12.323249816894531,33.339523315429688 -12.147735595703125,33.566280364990234 -12.450697898864746,33.164891643669634 -14.000060288415174,33.598796844482422 -14.546377182006836,34.328716278076172 -13.992490768432617,34.658355712890625 -13.81736946105957,34.886280059814453 -14.120697975158691,34.634240447128811 -15.85007183479255,34.931102752685547 -16.223842620849609,35.656356811523438 -15.66030216217041,35.963497161865234 -15.476018905639648,37.326129913330078 -17.190576553344727,38.823680877685547 -16.296066284179688,39.966808319091797 -17.625011444091797,40.800632476806641 -17.208097457885742,41.821544647216797 -19.211688995361328,41.988733475572282 -19.945838749437218,57.524304765518266 -37.807195733984784,41.590042114257813 -7.2021245956420898,40.6978759765625 -5.4500408172607422,40.758884429931641 -5.418975830078125,42.577911376953125 -4.4901103973388672,42.577877044677734 -4.4900407791137695,42.699958801269531 -4.4278755187988281,46.559533858616469 -8.435196445683264,47.604561877161387 -6.087697464505224,45.389434814453125 -4.5143837928771973,46.695858001708984 -1.6093428134918213,47.263670054709685 -1.784876824891044,47.830955505371094 -0.69758313894271851,48.43512638981781 -0.81299959072453376,49.071769542946825 0.61489892713413252,43.764598846435547 0.93951499462127686,43.644271850585938 0.96149998903274536,42.399410247802734 1.4956772327423096))",
        "POLYGON((43.644271850585938 0.96149998903274536,43.764598846435547 0.93951499462127686,49.071769542946825 0.61489892713413252,48.43512638981781 -0.81299959072453376,47.830955505371094 -0.69758313894271851,47.263670054709685 -1.784876824891044,46.695858001708984 -1.6093428134918213,45.389434814453125 -4.5143837928771973,47.604561877161387 -6.087697464505224,46.559533858616469 -8.435196445683264,42.699958801269531 -4.4278755187988281,42.577877044677734 -4.4900407791137695,42.577911376953125 -4.4901103973388672,40.758884429931641 -5.418975830078125,40.6978759765625 -5.4500408172607422,41.590042114257813 -7.2021245956420898,57.524304765518266 -37.807195733984784,41.988733475572282 -19.945838749437218,41.821544647216797 -19.211688995361328,40.800632476806641 -17.208097457885742,39.966808319091797 -17.625011444091797,38.823680877685547 -16.296066284179688,37.326129913330078 -17.190576553344727,35.963497161865234 -15.476018905639648,35.656356811523438 -15.66030216217041,34.931102752685547 -16.223842620849609,34.634240447128811 -15.85007183479255,34.886280059814453 -14.120697975158691,34.658355712890625 -13.81736946105957,34.328716278076172 -13.992490768432617,33.598796844482422 -14.546377182006836,33.164891643669634 -14.000060288415174,33.566280364990234 -12.450697898864746,33.339523315429688 -12.147735595703125,32.998821258544922 -12.323249816894531,32.274600982666016 -12.879127502441406,31.682494778186321 -12.133624901803865,32.226280212402344 -10.790698051452637,32.000633239746094 -10.488097190856934,31.669155120849609 -10.653837203979492,30.947774887084961 -11.208560943603516,30.207040612748258 -10.275926149505661,30.896280288696289 -9.1206979751586914,30.670633316040039 -8.8180980682373047,30.339155197143555 -8.9838371276855469,29.619997024536133 -9.5368013381958008,29.135100397190627 -8.9262827849488211,32.718830108642578 -4.3281683921813965,32.708168029785156 -2.3611698150634766,32.708126068115234 -2.3611700534820557,32.708126068115234 -2.3611266613006592,30.501169204711914 -2.3718316555023193,27.069889344709196 -4.2926591211028242,26.472516656201325 -3.5380830513658776,36.954700469970703 1.2597870826721191,37.140213012695313 1.3446992635726929,37.000419616699219 1.664225697517395,37.29449462890625 1.7902572154998779,37.43402099609375 1.470055103302002,51.370888500897557 7.4163459734570729,51.20102152843122 7.1738039562841562,42.721500396728516 3.6584999561309814,42.721500396728516 2.2342472076416016,42.399410247802734 1.4956772327423096,43.644271850585938 0.96149998903274536))"
        };


    // Test the FORWARD case
    test_one<polygon, polygon, polygon>(caseid, brandon[0], brandon[1]);
}
#endif



int main(int argc, char** argv)
{
    int mode = (argc > 1) ? atol(argv[1]) : 1;
    switch(mode)
    {
        case 1 :
            test_traverse_gmp<bg::model::d2::point_xy<float> >("float");
            break;
        case 2 :
            test_traverse_gmp<bg::model::d2::point_xy<double> >("double");
            break;
        case 3 :
            test_traverse_gmp<bg::model::d2::point_xy<long double> >("long double");
            break;
        case 4 :
            #if defined(HAVE_TTMATH)
                test_traverse_gmp<bg::model::d2::point_xy<ttmath_big> >("ttmath_big");
            #endif
            break;
    }
    return 0;
}
