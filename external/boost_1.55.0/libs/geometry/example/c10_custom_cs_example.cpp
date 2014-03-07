// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Example: Custom coordinate system example 

#include <iostream>

#include <boost/geometry/geometry.hpp>

#ifdef OPTIONALLY_ELLIPSOIDAL  // see below
#include <boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp>
#endif

// 1: declare a coordinate system. For example for Mars
//    Like for the Earth, we let the use choose between degrees or radians
//    (Unfortunately, in real life Mars has two coordinate systems:
//     http://planetarynames.wr.usgs.gov/Page/MARS/system)
template<typename DegreeOrRadian>
struct martian
{
    typedef DegreeOrRadian units;
};

// 2: give it also a family
struct martian_tag;

// 3: register to which coordinate system family it belongs to
//    this must be done in namespace boost::geometry::traits
namespace boost { namespace geometry { namespace traits
{

template <typename DegreeOrRadian>
struct cs_tag<martian<DegreeOrRadian> >
{
    typedef martian_tag type;
};

}}} // namespaces


// NOTE: if the next steps would not be here, 
// compiling a distance function call with martian coordinates 
// would result in a MPL assertion

// 4: so register a distance strategy as its default strategy
namespace boost { namespace geometry { namespace strategy { namespace distance { namespace services
{

template <typename Point1, typename Point2>
struct default_strategy<point_tag, Point1, Point2, martian_tag, martian_tag>
{
    typedef haversine<double> type;
};

}}}}} // namespaces

// 5: not worked out. To implement a specific distance strategy for Mars,
//    e.g. with the Mars radius given by default, 
//    you will have to implement (/register) several other metafunctions:
//      tag, return_type, similar_type, comparable_type, 
//    and structs:
//      get_similar, get_comparable, result_from_distance
//   See e.g. .../boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp 

int main()
{
    typedef boost::geometry::model::point
        <
            double, 2, martian<boost::geometry::degree> 
        > mars_point;

    // Declare two points 
    // (Source: http://nssdc.gsfc.nasa.gov/planetary/mars_mileage_guide.html)
    // (Other sources: Wiki and Google give slightly different coordinates, resulting
    //  in other distance, 20 km off)
    mars_point viking1(-48.23, 22.54); // Viking 1 landing site in Chryse Planitia
    mars_point pathfinder(-33.55, 19.33); // Pathfinder landing site in Ares Vallis

    double d = boost::geometry::distance(viking1, pathfinder); // Distance in radians on unit-sphere

    // Using the Mars mean radius
    // (Source: http://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html)
    std::cout << "Distance between Viking1 and Pathfinder landing sites: " 
        << d * 3389.5 << " km" << std::endl;

    // We would get 832.616 here, same order as the 835 (rounded on 5 km) listed
    // on the mentioned site 

#ifdef OPTIONALLY_ELLIPSOIDAL
    // Optionally the distance can be calculated more accurate by an Ellipsoidal approach,
    // giving 834.444 km
    d = boost::geometry::distance(viking1, pathfinder,
        boost::geometry::strategy::distance::andoyer<mars_point>
            (boost::geometry::detail::ellipsoid<double>(3396.2, 3376.2)));
    std::cout << "Ellipsoidal distance: " << d << " km" << std::endl;
#endif

    return 0;
}
