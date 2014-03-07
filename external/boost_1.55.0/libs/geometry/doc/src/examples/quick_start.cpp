// Boost.Geometry (aka GGL, Generic Geometry Library)
// Quickbook Examples, for main page

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#if defined(_MSC_VER)
// We deliberately mix float/double's here so turn off warning
//#pragma warning( disable : 4244 )
#endif // defined(_MSC_VER)

//[quickstart_include

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

using namespace boost::geometry;
//]

#include <boost/geometry/geometries/register/point.hpp>


//[quickstart_register_c_array
#include <boost/geometry/geometries/adapted/c_array.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
//]

//[quickstart_register_boost_tuple
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
//]

// Small QRect simulations following http://doc.trolltech.com/4.4/qrect.html
// Todo: once work the traits out further, would be nice if there is a real example of this.
// However for the example it makes no difference, it will work any way.
struct QPoint
{
    int x, y;
    // In Qt these are methods but for example below it makes no difference
};

struct QRect
{
    int x, y, width, height;
    QRect(int _x, int _y, int w, int h)
        : x(_x), y(_y), width(w), height(h)
    {}
    // In Qt these are methods but that will work as well, requires changing traits below
};


// Would be get/set with x(),y(),setX(),setY()
BOOST_GEOMETRY_REGISTER_POINT_2D(QPoint, int, cs::cartesian, x, y)


// Register the QT rectangle. The macro(s) does not offer (yet) enough flexibility to do this in one line,
// but the traits classes do their job perfectly.
namespace boost { namespace geometry { namespace traits
{

template <> struct tag<QRect> { typedef box_tag type; };
template <> struct point_type<QRect> { typedef QPoint type; };

template <size_t C, size_t D>
struct indexed_access<QRect, C, D>
{
    static inline int get(const QRect& qr)
    {
        // Would be: x(), y(), width(), height()
        return C == min_corner && D == 0 ? qr.x
                : C == min_corner && D == 1 ? qr.y
                : C == max_corner && D == 0 ? qr.x + qr.width
                : C == max_corner && D == 1 ? qr.y + qr.height
                : 0;
    }

    static inline void set(QRect& qr, const int& value)
    {
        // Would be: setX, setY, setWidth, setHeight
        if (C == min_corner && D == 0) qr.x = value;
        else if (C == min_corner && D == 1) qr.y = value;
        else if (C == max_corner && D == 0) qr.width = value - qr.x;
        else if (C == max_corner && D == 1) qr.height = value - qr.y;
    }
};


}}}


int main(void)
{
    //[quickstart_distance
    model::d2::point_xy<int> p1(1, 1), p2(2, 2);
    std::cout << "Distance p1-p2 is: " << distance(p1, p2) << std::endl;
    //]

    //[quickstart_distance_c_array
    int a[2] = {1,1};
    int b[2] = {2,3};
    double d = distance(a, b);
    std::cout << "Distance a-b is: " << d << std::endl;
    //]
    
    //[quickstart_point_in_polygon
    double points[][2] = {{2.0, 1.3}, {4.1, 3.0}, {5.3, 2.6}, {2.9, 0.7}, {2.0, 1.3}};
    model::polygon<model::d2::point_xy<double> > poly;
    append(poly, points);
    boost::tuple<double, double> p = boost::make_tuple(3.7, 2.0);
    std::cout << "Point p is in polygon? " << std::boolalpha << within(p, poly) << std::endl;
    //]
    
    //[quickstart_area
    std::cout << "Area: " << area(poly) << std::endl;
    //]

    //[quickstart_distance_mixed
    double d2 = distance(a, p);
    std::cout << "Distance a-p is: " << d2 << std::endl;
    //]
    
    //[quick_start_spherical
    typedef boost::geometry::model::point
        <
            double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
        > spherical_point;
    
    spherical_point amsterdam(4.90, 52.37);
    spherical_point paris(2.35, 48.86);
    
    double const earth_radius = 3959; // miles
    std::cout << "Distance in miles: " << distance(amsterdam, paris) * earth_radius << std::endl;
    //]

    /***
    Now extension
    point_ll_deg  amsterdam, paris;
    parse(amsterdam, "52 22 23 N", "4 53 32 E");
    parse(paris, "48 52 0 N", "2 19 59 E");
    std::cout << "Distance A'dam-Paris: " << distance(amsterdam, paris) / 1000.0 << " kilometers " << std::endl;
    ***/

    //[quickstart_qt
    QRect r1(100, 200, 15, 15);
    QRect r2(110, 210, 20, 20);
    if (overlaps(r1, r2))
    {
        assign_values(r2, 200, 300, 220, 320);
    }
    //]
    
    return 0;
}

