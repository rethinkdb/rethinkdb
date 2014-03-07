// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[tag
//` Shows how tag dispatching essentially works in Boost.Geometry

#include <iostream>

#include <boost/assign.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

template <typename Tag> struct dispatch {};

// Specialization for points
template <> struct dispatch<boost::geometry::point_tag>
{
    template <typename Point>
    static inline void apply(Point const& p)
    {
        // Use the Boost.Geometry free function "get"
        // working on all supported point types
        std::cout << "Hello POINT, you are located at: " 
            << boost::geometry::get<0>(p) << ", " 
            << boost::geometry::get<1>(p) 
            << std::endl;
    }
};

// Specialization for polygons
template <> struct dispatch<boost::geometry::polygon_tag>
{
    template <typename Polygon>
    static inline void apply(Polygon const& p)
    {
        // Use the Boost.Geometry manipulator "dsv" 
        // working on all supported geometries
        std::cout << "Hello POLYGON, you look like: " 
            << boost::geometry::dsv(p) 
            << std::endl;
    }
};

// Specialization for multipolygons
template <> struct dispatch<boost::geometry::multi_polygon_tag>
{
    template <typename MultiPolygon>
    static inline void apply(MultiPolygon const& m)
    {
        // Use the Boost.Range free function "size" because all
        // multigeometries comply to Boost.Range
        std::cout << "Hello MULTIPOLYGON, you contain: " 
            << boost::size(m) << " polygon(s)"
            << std::endl;
    }
};

template <typename Geometry>
inline void hello(Geometry const& geometry)
{
    // Call the metafunction "tag" to dispatch, and call method (here "apply")
    dispatch
        <
            typename boost::geometry::tag<Geometry>::type
        >::apply(geometry);
}

int main()
{
    // Define polygon type (here: based on a Boost.Tuple)
    typedef boost::geometry::model::polygon<boost::tuple<int, int> > polygon_type;

    // Declare and fill a polygon and a multipolygon
    polygon_type poly;
    boost::geometry::exterior_ring(poly) = boost::assign::tuple_list_of(0, 0)(0, 10)(10, 5)(0, 0);
        
    boost::geometry::model::multi_polygon<polygon_type> multi;
    multi.push_back(poly);

    // Call "hello" for point, polygon, multipolygon
    hello(boost::make_tuple(2, 3));
    hello(poly);
    hello(multi);

    return 0;
}

//]

//[tag_output
/*`
Output:
[pre
Hello POINT, you are located at: 2, 3
Hello POLYGON, you look like: (((0, 0), (0, 10), (10, 5), (0, 0)))
Hello MULTIPOLYGON, you contain: 1 polygon(s)
]
*/
//]
