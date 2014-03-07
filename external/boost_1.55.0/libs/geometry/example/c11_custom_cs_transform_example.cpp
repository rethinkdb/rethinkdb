// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Example: Custom coordinate system example, using transform

#include <iostream>

#include <boost/geometry/geometry.hpp>

// See also c10_custom_cs_example

// 1: declare, for example two cartesian coordinate systems
struct cart {};
struct cart_shifted5 {};

// 2: register to which coordinate system family they belong
namespace boost { namespace geometry { namespace traits
{

template<> struct cs_tag<cart> { typedef cartesian_tag type; };
template<> struct cs_tag<cart_shifted5> { typedef cartesian_tag type; };

}}} // namespaces


// 3: sample implementation of a shift 
//    to convert coordinate system "cart" to "cart_shirted5"
struct shift
{
    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2& p2) const
    {
        namespace bg = boost::geometry;
        bg::set<0>(p2, bg::get<0>(p1) + 5);
        bg::set<1>(p2, bg::get<1>(p1));
        return true;
    }
};


// 4: register the default strategy to transform any cart point to any cart_shifted5 point
namespace boost { namespace geometry  { namespace strategy { namespace transform { namespace services 
{

template <typename P1, typename P2>
struct default_strategy<cartesian_tag, cartesian_tag, cart, cart_shifted5, 2, 2, P1, P2>
{
    typedef shift type;
};

}}}}} // namespaces


// 5: implement a distance strategy between the two different ones
struct shift_and_calc_distance
{
    template <typename P1, typename P2>
    inline double apply(P1 const& p1, P2 const& p2) const
    {
        P2 p1_shifted;
        boost::geometry::transform(p1, p1_shifted);
        return boost::geometry::distance(p1_shifted, p2);
    }
};

// 6: Define point types using this explicitly
typedef boost::geometry::model::point<double, 2, cart> point1;
typedef boost::geometry::model::point<double, 2, cart_shifted5> point2;

// 7: register the distance strategy
namespace boost { namespace geometry { namespace strategy { namespace distance { namespace services 
{
    template <>
    struct tag<shift_and_calc_distance>
    {
        typedef strategy_tag_distance_point_point type;
    };
    
    template <typename P1, typename P2>
    struct return_type<shift_and_calc_distance, P1, P2>
    {
        typedef double type;
    };

    template <>
    struct default_strategy<point_tag, point1, point2, cartesian_tag, cartesian_tag>
    {
        typedef shift_and_calc_distance type;
    };


}}}}}



int main()
{
    point1 p1_a(0, 0), p1_b(5, 5);
    point2 p2_a(2, 2), p2_b(6, 6); 

    // Distances run for points on the same coordinate system.
    // This is possible by default because they are cartesian coordinate systems.
    double d1 = boost::geometry::distance(p1_a, p1_b); 
    double d2 = boost::geometry::distance(p2_a, p2_b); 

    std::cout << d1 << " " << d2 << std::endl;

    // Transform from a to b:
    boost::geometry::model::point<double, 2, cart_shifted5> p1_shifted;
    boost::geometry::transform(p1_a, p1_shifted); 


    // Of course this can be calculated now, same CS
    double d3 = boost::geometry::distance(p1_shifted, p2_a); 


    // Calculate distance between them. Note that inside distance the 
    // transformation is called.
    double d4 = boost::geometry::distance(p1_a, p2_a); 

    // The result should be the same.
    std::cout << d3 << " " << d4 << std::endl;

    return 0;
}
