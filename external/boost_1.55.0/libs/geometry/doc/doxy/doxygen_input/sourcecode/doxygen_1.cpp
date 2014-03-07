OBSOLETE

// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Doxygen Examples, referred to from the sources

#include <boost/tuple/tuple.hpp>

#if defined(_MSC_VER)
// We deliberately mix float/double's here so turn off warning
#pragma warning( disable : 4244 )
#endif // defined(_MSC_VER)

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

// All functions below are referred to in the documentation of Boost.Geometry
// Don't rename them.
void example_area_polygon()
{
    //[area_polygon
    //` Calculate the area of a polygon
    boost::geometry::polygon<boost::geometry::point_xy<double> > poly; /*< Declare >*/
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly); /*< Fill, in this case with WKT >*/
    double area = boost::geometry::area(poly); /*< Calculate area >*/
    //]

    //[area_polygon_spherical
    //` Calculate the area of a *spherical* polygon
    namespace bg = boost::geometry;
    bg::polygon<bg::point<float, 2, bg::cs::spherical<bg::degree> > > sph_poly;
    bg::read_wkt("POLYGON((0 0,0 45,45 0,0 0))", sph_poly);
    double area = bg::area(sph_poly);
    //]
}

void example_as_wkt_point()
{
    typedef boost::geometry::point_xy<double> P;
    P p(5.12, 6.34);
    // Points can be streamed like this:
    std::cout << boost::geometry::dsv<P>(p) << std::endl;

    // or like this:
    std::cout << boost::geometry::dsv(p) << std::endl;

    // or (with extension) like this:
    std::cout << boost::geometry::wkt(p) << std::endl;
}

void example_as_wkt_vector()
{
    std::vector<boost::geometry::point_xy<int> > v;
    boost::geometry::read_wkt<boost::geometry::point_xy<int> >("linestring(1 1,2 2,3 3,4 4)", std::back_inserter(v));

    std::cout << boost::geometry::dsv(std::make_pair(v.begin(), v.end())) << std::endl;
}


void example_centroid_polygon()
{
    boost::geometry::polygon<boost::geometry::point_xy<double> > poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    // Center of polygon might have different type than points of polygon
    boost::geometry::point_xy<float> center;
    boost::geometry::centroid(poly, center);
    std::cout << "Centroid: " << boost::geometry::dsv(center) << std::endl;
}


void example_distance_point_point()
{
    boost::geometry::point_xy<double> p1(1, 1);
    boost::geometry::point_xy<double> p2(2, 3);
    std::cout << "Distance p1-p2 is "
        << boost::geometry::distance(p1, p2)
        << " units" << std::endl;

    /*
    Extension, other coordinate system:
    // Read 2 Dutch cities from WKT texts (in decimal degrees)
    boost::geometry::point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> >  a, r;
    boost::geometry::read_wkt("POINT(4.89222 52.3731)", a);
    boost::geometry::read_wkt("POINT(4.47917 51.9308)", r);

    std::cout << "Distance Amsterdam-Rotterdam is "
        << boost::geometry::distance(a, r) / 1000.0
        << " kilometers " << std::endl;
    */
}

void example_distance_point_point_strategy()
{
    /*
    Extension, other coordinate system:
    typedef boost::geometry::point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> > LL;
    LL a, r;
    boost::geometry::read_wkt("POINT(4.89222 52.3731)", a);
    boost::geometry::read_wkt("POINT(4.47917 51.9308)", r);

    std::cout << "Distance Amsterdam-Rotterdam is "
        << boost::geometry::distance(a, r,
                boost::geometry::strategy::distance::vincenty<LL>() )
                 / 1000.0
        << " kilometers " << std::endl;
    */
}

void example_from_wkt_point()
{
    boost::geometry::point_xy<int> point;
    boost::geometry::read_wkt("Point(1 2)", point);
    std::cout << point.x() << "," << point.y() << std::endl;
}

void example_from_wkt_output_iterator()
{
    std::vector<boost::geometry::point_xy<int> > v;
    boost::geometry::read_wkt<boost::geometry::point_xy<int> >("linestring(1 1,2 2,3 3,4 4)", std::back_inserter(v));
    std::cout << "vector has " << v.size() << " coordinates" << std::endl;
}

void example_from_wkt_linestring()
{
    boost::geometry::linestring<boost::geometry::point_xy<double> > line;
    boost::geometry::read_wkt("linestring(1 1,2 2,3 3,4 4)", line);
    std::cout << "linestring has " << line.size() << " coordinates" << std::endl;
}

void example_from_wkt_polygon()
{
    boost::geometry::polygon<boost::geometry::point_xy<double> > poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 1,1 1,1 0,0 0))", poly);
    std::cout << "Polygon has " << poly.outer().size() << " coordinates in outer ring" << std::endl;
}

void example_point_ll_convert()
{
    /*
    Extension, other coordinate system:
    boost::geometry::point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> > deg(boost::geometry::latitude<>(33.0), boost::geometry::longitude<>(-118.0));
    boost::geometry::point_ll<double, boost::geometry::cs::geographic<boost::geometry::radian> > rad;
    boost::geometry::transform(deg, rad);

    std::cout << "point in radians: " << rad << std::endl;
    */
}

void example_clip_linestring1()
{
    typedef boost::geometry::point_xy<double> P;
    boost::geometry::linestring<P> line;
    boost::geometry::read_wkt("linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)", line);
    boost::geometry::box<P> cb(P(1.5, 1.5), P(4.5, 2.5));
    std::cout << "Clipped linestring(s) " << std::endl;

    std::vector<boost::geometry::linestring<P> > intersection;
    boost::geometry::intersection_inserter<boost::geometry::linestring<P> >(cb, line, std::back_inserter(intersection));
}

void example_clip_linestring2()
{
    typedef boost::geometry::point_xy<double> P;
    std::vector<P> vector_in;
    boost::geometry::read_wkt<P>("linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)",
                    std::back_inserter(vector_in));

    boost::geometry::box<P> cb(P(1.5, 1.5), P(4.5, 2.5));
    typedef std::vector<std::vector<P> > VV;
    VV vector_out;
    boost::geometry::intersection_inserter<std::vector<P>  >(cb, vector_in, std::back_inserter(vector_out));

    std::cout << "Clipped vector(s) " << std::endl;
    for (VV::const_iterator it = vector_out.begin(); it != vector_out.end(); it++)
    {
        std::copy(it->begin(), it->end(), std::ostream_iterator<P>(std::cout, " "));
        std::cout << std::endl;
    }
}





void example_intersection_polygon1()
{
    typedef boost::geometry::point_xy<double> P;
    typedef std::vector<boost::geometry::polygon<P> > PV;

    boost::geometry::box<P> cb(P(1.5, 1.5), P(4.5, 2.5));
    boost::geometry::polygon<P> poly;
    boost::geometry::read_wkt("POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            ",(4 2,4.2 1.4,4.8 1.9,4.4 2.2,4 2))", poly);

    PV v;
    boost::geometry::intersection_inserter<boost::geometry::polygon<P> >(cb, poly, std::back_inserter(v));

    std::cout << "Clipped polygon(s) " << std::endl;
    for (PV::const_iterator it = v.begin(); it != v.end(); it++)
    {
        std::cout << boost::geometry::dsv(*it) << std::endl;
    }
}

void example_simplify_linestring1()
{
    //[simplify
    //` Simplify a linestring
    boost::geometry::linestring<boost::geometry::point_xy<double> > line, simplified;
    boost::geometry::read_wkt("linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)", line);
    boost::geometry::simplify(line, simplified, 0.5); /*< Simplify it, using distance of 0.5 units >*/
    std::cout
        << "  original line: " << boost::geometry::dsv(line) << std::endl
        << "simplified line: " << boost::geometry::dsv(simplified) << std::endl;
    //]
}

void example_simplify_linestring2()
{
    //[simplify_inserter
    //` Simplify a linestring using an output iterator
    typedef boost::geometry::point_xy<double> P;
    typedef boost::geometry::linestring<P> L;
    L line;

    boost::geometry::read_wkt("linestring(1.1 1.1, 2.5 2.1, 3.1 3.1, 4.9 1.1, 3.1 1.9)", line);

    typedef boost::geometry::strategy::distance::projected_point<P, P> DS;
    typedef boost::geometry::strategy::simplify::douglas_peucker<P, DS> simplification;
    boost::geometry::simplify_inserter(line, std::ostream_iterator<P>(std::cout, "\n"), 0.5, simplification());
    //]
}



void example_within()
{
    boost::geometry::polygon<boost::geometry::point_xy<double> > poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    boost::geometry::point_xy<float> point(3, 3);
    std::cout << "Point is "
        << (boost::geometry::within(point, poly) ? "IN" : "NOT in")
        << " polygon"
        << std::endl;
}

/*
void example_within_strategy()
{
    // TO BE UPDATED/FINISHED
    typedef boost::geometry::point_xy<double> P;
    typedef boost::geometry::polygon<P> POLY;
    P p;
    std::cout << within(p, poly, strategy::within::cross_count<P>) << std::endl;
}
*/

void example_length_linestring()
{
    using namespace boost::geometry;
    linestring<point_xy<double> > line;
    read_wkt("linestring(0 0,1 1,4 8,3 2)", line);
    std::cout << "linestring length is "
        << length(line)
        << " units" << std::endl;

    /*
    Extension, other coordinate system:
    // Linestring in latlong, filled with
    // explicit degree-minute-second values
    typedef point_ll<float, boost::geometry::cs::geographic<boost::geometry::degree> > LL;
    linestring<LL> line_ll;
    line_ll.push_back(LL(
        latitude<float>(dms<north, float>(52, 22, 23)),
        longitude<float>(dms<east, float>(4, 53, 32))));
    line_ll.push_back(LL(
        latitude<float>(dms<north, float>(51, 55, 51)),
        longitude<float>(dms<east, float>(4, 28, 45))));
    line_ll.push_back(LL(
        latitude<float>(dms<north, float>(52, 4, 48)),
        longitude<float>(dms<east, float>(4, 18, 0))));
    std::cout << "linestring length is "
        << length(line_ll) / 1000
        << " kilometers " << std::endl;
        */
}

void example_length_linestring_iterators1()
{
    boost::geometry::linestring<boost::geometry::point_xy<double> > line;
    boost::geometry::read_wkt("linestring(0 0,1 1,4 8,3 2)", line);
    std::cout << "linestring length is "
        << boost::geometry::length(line)
        << " units" << std::endl;
}

void example_length_linestring_iterators2()
{
    std::vector<boost::geometry::point_xy<double> > line;
    boost::geometry::read_wkt<boost::geometry::point_xy<double> >("linestring(0 0,1 1,4 8,3 2)", std::back_inserter(line));
    std::cout << "linestring length is "
        << boost::geometry::length(line)
        << " units" << std::endl;
}

void example_length_linestring_iterators3()
{
    /*
    Extension, other coordinate system:
    using namespace boost::geometry;
    typedef point_ll<float, boost::geometry::cs::geographic<boost::geometry::degree> > LL;
    std::deque<LL> line;
    boost::geometry::read_wkt<LL>("linestring(0 51,1 51,2 52)", std::back_inserter(line));
    std::cout << "linestring length is "
        << 0.001 * boost::geometry::length(line, boost::geometry::strategy::distance::vincenty<LL>())
        << " kilometers" << std::endl;
    */
}


void example_length_linestring_strategy()
{
    /*
    Extension, other coordinate system:
    using namespace boost::geometry;
    typedef point_ll<float, boost::geometry::cs::geographic<boost::geometry::degree> > LL;
    linestring<LL> line_ll;
    line_ll.push_back(LL(latitude<float>(dms<north, float>(52, 22, 23)), longitude<float>(dms<east, float>(4, 53, 32))));
    line_ll.push_back(LL(latitude<float>(dms<north, float>(51, 55, 51)), longitude<float>(dms<east, float>(4, 28, 45))));
    line_ll.push_back(LL(latitude<float>(dms<north, float>(52, 4, 48)), longitude<float>(dms<east, float>(4, 18, 0))));
    std::cout << "linestring length is "
        << length(line_ll, strategy::distance::vincenty<LL, LL>() )/(1000)
        << " kilometers " << std::endl;
    */
}


void example_envelope_linestring()
{
    boost::geometry::linestring<boost::geometry::point_xy<double> > line;
    boost::geometry::read_wkt("linestring(0 0,1 1,4 8,3 2)", line);
    boost::geometry::box<boost::geometry::point_xy<double> > box;
    boost::geometry::envelope(line, box);

    std::cout << "envelope is " << boost::geometry::dsv(box) << std::endl;
}

void example_envelope_polygon()
{
    /*
    Extension, other coordinate system:
    using namespace boost::geometry;
    typedef point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> >  LL;

    // Wrangel island, 180 meridian crossing island above Siberia.
    polygon<LL> wrangel;
    wrangel.outer().push_back(LL(latitude<>(dms<north>(70, 47, 7)), longitude<>(dms<west>(178, 47, 9))));
    wrangel.outer().push_back(LL(latitude<>(dms<north>(71, 14, 0)), longitude<>(dms<east>(177, 28, 33))));
    wrangel.outer().push_back(LL(latitude<>(dms<north>(71, 34, 24)), longitude<>(dms<east>(179, 44, 37))));
    // Close it
    wrangel.outer().push_back(wrangel.outer().front());

    boost::geometry::box<LL> box;
    boost::geometry::envelope(wrangel, box);

    dms<cd_lat> minlat(box.min_corner().lat());
    dms<cd_lon> minlon(box.min_corner().lon());

    dms<cd_lat> maxlat(box.max_corner().lat());
    dms<cd_lon> maxlon(box.max_corner().lon());

    std::cout << wrangel << std::endl;
    std::cout << "min: " << minlat.get_dms() << " , " << minlon.get_dms() << std::endl;
    std::cout << "max: " << maxlat.get_dms() << " , " << maxlon.get_dms() << std::endl;
    */
}


void example_dms()
{
    /*
    Extension, other coordinate system:
    // Construction with degree/minute/seconds
    boost::geometry::dms<boost::geometry::east> d1(4, 53, 32.5);

    // Explicit conversion to double.
    std::cout << d1.as_value() << std::endl;

    // Conversion to string, with optional strings
    std::cout << d1.get_dms(" deg ", " min ", " sec") << std::endl;

    // Combination with latitude/longitude and cardinal directions
    {
        using namespace boost::geometry;
        point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> > canberra(
            latitude<>(dms<south>(35, 18, 27)),
            longitude<>(dms<east>(149, 7, 27.9)));
        std::cout << canberra << std::endl;
    }
    */
}

void example_point_ll_construct()
{
    /*
    Extension, other coordinate system:
    using namespace boost::geometry;
    typedef point_ll<double, boost::geometry::cs::geographic<boost::geometry::degree> > ll;

    // Constructions in both orders possible
    ll juneau(
        latitude<>(dms<north>(58, 21, 5)),
        longitude<>(dms<west>(134, 30, 42)));
    ll wladiwostok(
        longitude<>(dms<east>(131, 54)),
        latitude<>(dms<north>(43, 8))
        );
    */
}

int main(void)
{
    example_area_polygon();

    example_centroid_polygon();

    example_distance_point_point();
    example_distance_point_point_strategy();

    example_from_wkt_point();
    example_from_wkt_output_iterator();
    example_from_wkt_linestring();
    example_from_wkt_polygon();

    example_as_wkt_point();

    example_clip_linestring1();
    example_clip_linestring2();
    example_intersection_polygon1();

    example_simplify_linestring1();
    example_simplify_linestring2();

    example_length_linestring();
    example_length_linestring_iterators1();
    example_length_linestring_iterators2();
    example_length_linestring_iterators3();
    example_length_linestring_strategy();

    example_envelope_linestring();
    example_envelope_polygon();

    example_within();

    example_point_ll_convert();
    example_point_ll_construct();
    example_dms();

    return 0;
}
