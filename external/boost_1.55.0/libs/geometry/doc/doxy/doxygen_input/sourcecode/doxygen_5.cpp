OBSOLETE

// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Doxygen Examples, for Geometry Concepts

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/geometries.hpp>



struct legacy_point1
{
    double x, y;
};

// adapt legacy_point1
namespace boost { namespace geometry { namespace traits
{
    template <> struct tag<legacy_point1> { typedef point_tag type; };
    template <> struct coordinate_type<legacy_point1> { typedef double type; };
    template <> struct coordinate_system<legacy_point1> { typedef cs::cartesian type; };
    template <> struct dimension<legacy_point1>: boost::mpl::int_<2> {};
    template <> struct access<legacy_point1, 0>
    {
        static double get(legacy_point1 const& p) { return p.x; }
        static void set(legacy_point1& p, double const& value) { p.x = value; }
    };
    template <> struct access<legacy_point1, 1>
    {
        static double get(legacy_point1 const& p) { return p.y; }
        static void set(legacy_point1& p, double const& value) { p.y = value; }
    };
}}} // namespace boost::geometry::traits
// end adaptation

namespace example_legacy_point1
{
    // The first way to check a concept at compile time: checking if the input is parameter
    // or return type is OK.
    template <typename P>
    BOOST_CONCEPT_REQUIRES(((boost::geometry::concept::Point<P>)), (void))
    test1(P& p)
    {
    }

    // The second way to check a concept at compile time: checking if the provided type,
    // inside the function, if OK
    template <typename P>
    void test2(P& p)
    {
        BOOST_CONCEPT_ASSERT((boost::geometry::concept::Point<P>));
    }


    void example()
    {
        legacy_point1 p;
        test1(p);
        test2(p);
    }
}

// leave comment below for (strange behaviour of) doxygen
class legacy_point2
{
public :
    double x() const;
    double y() const;
};

// adapt legacy_point2
BOOST_GEOMETRY_REGISTER_POINT_2D_CONST(legacy_point2, double, boost::geometry::cs::cartesian, x(), y() )
// end adaptation


double legacy_point2::x() const { return 0; }
double legacy_point2::y() const { return 0; }

namespace example_legacy_point2
{
    // test it using boost concept requires

    template <typename P>
    BOOST_CONCEPT_REQUIRES(((boost::geometry::concept::ConstPoint<P>)), (double))
    test3(P& p)
    {
        return boost::geometry::get<0>(p);
    }

    void example()
    {
        legacy_point2 p;
        test3(p);
    }
}


template <typename P>
struct custom_linestring1 : std::deque<P>
{
    int id;
};

// adapt custom_linestring1
namespace boost { namespace geometry { namespace traits
{
    template <typename P>
    struct tag< custom_linestring1<P> > { typedef linestring_tag type; };
}}} // namespace boost::geometry::traits
// end adaptation

namespace example_custom_linestring1
{
    void example()
    {
        typedef custom_linestring1<legacy_point1> L;
        BOOST_CONCEPT_ASSERT((boost::geometry::concept::Linestring<L>));

    }
}

int main(void)
{
    example_legacy_point1::example();
    example_legacy_point2::example();
    example_custom_linestring1::example();

    return 0;
}
