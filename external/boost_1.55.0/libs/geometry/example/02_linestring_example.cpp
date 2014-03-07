// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Linestring Example

#include <algorithm> // for reverse, unique
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

// Optional includes and defines to handle c-arrays as points, std::vectors as linestrings
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)

BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)


template<typename P>
inline void translate_function(P& p)
{
        p.x(p.x() + 100.0);
}

template<typename P>
struct scale_functor
{
    inline void operator()(P& p)
    {
        p.x(p.x() * 1000.0);
        p.y(p.y() * 1000.0);
    }
};


template<typename Point>
struct round_coordinates
{
    typedef typename boost::geometry::coordinate_type<Point>::type coordinate_type;
    coordinate_type m_factor;

    inline round_coordinates(coordinate_type const& factor)
        : m_factor(factor)
    {}

    template <int Dimension>
    inline void round(Point& p)
    {
        coordinate_type c = boost::geometry::get<Dimension>(p) / m_factor;
        int rounded = c;
        boost::geometry::set<Dimension>(p, coordinate_type(rounded) * m_factor);
    }

    inline void operator()(Point& p)
    {
        round<0>(p);
        round<1>(p);
    }
};


int main(void)
{
    using namespace boost::geometry;

    // Define a linestring, which is a vector of points, and add some points
    // (we add them deliberately in different ways)
    typedef model::d2::point_xy<double> point_2d;
    typedef model::linestring<point_2d> linestring_2d;
    linestring_2d ls;

    // points can be created using "make" and added to a linestring using the std:: "push_back"
    ls.push_back(make<point_2d>(1.1, 1.1));

    // points can also be assigned using "assign_values" and added to a linestring using "append"
    point_2d lp;
    assign_values(lp, 2.5, 2.1);
    append(ls, lp);

    // Lines can be streamed using DSV (delimiter separated values)
    std::cout << dsv(ls) << std::endl;

    // The bounding box of linestrings can be calculated
    typedef model::box<point_2d> box_2d;
    box_2d b;
    envelope(ls, b);
    std::cout << dsv(b) << std::endl;

    // The length of the line can be calulated
    std::cout << "length: " << length(ls) << std::endl;

    // All things from std::vector can be called, because a linestring is a vector
    std::cout << "number of points 1: " << ls.size() << std::endl;

    // All things from boost ranges can be called because a linestring is considered as a range
    std::cout << "number of points 2: " << boost::size(ls) << std::endl;

    // Generic function from geometry/OGC delivers the same value
    std::cout << "number of points 3: " << num_points(ls) << std::endl;

    // The distance from a point to a linestring can be calculated
    point_2d p(1.9, 1.2);
    std::cout << "distance of " << dsv(p)
        << " to line: " << distance(p, ls) << std::endl;

    // A linestring is a vector. However, some algorithms consider "segments",
    // which are the line pieces between two points of a linestring.
    double d = distance(p, model::segment<point_2d >(ls.front(), ls.back()));
    std::cout << "distance: " << d << std::endl;

    // Add some three points more, let's do it using a classic array.
    // (See documentation for picture of this linestring)
    const double c[][2] = { {3.1, 3.1}, {4.9, 1.1}, {3.1, 1.9} };
    append(ls, c);
    std::cout << "appended: " << dsv(ls) << std::endl;

    // Output as iterator-pair on a vector
    {
        std::vector<point_2d> v;
        std::copy(ls.begin(), ls.end(), std::back_inserter(v));

        std::cout
            << "as vector: "
            << dsv(v)
            << std::endl;
    }

    // All algorithms from std can be used: a linestring is a vector
    std::reverse(ls.begin(), ls.end());
    std::cout << "reversed: " << dsv(ls) << std::endl;
    std::reverse(boost::begin(ls), boost::end(ls));

    // The other way, using a vector instead of a linestring, is also possible
    std::vector<point_2d> pv(ls.begin(), ls.end());
    std::cout << "length: " << length(pv) << std::endl;

    // If there are double points in the line, you can use unique to remove them
    // So we add the last point, print, make a unique copy and print
    {
        // (sidenote, we have to make copies, because
        // ls.push_back(ls.back()) often succeeds but
        // IS dangerous and erroneous!
        point_2d last = ls.back(), first = ls.front();
        ls.push_back(last);
        ls.insert(ls.begin(), first);
    }
    std::cout << "extra duplicate points: " << dsv(ls) << std::endl;

    {
        linestring_2d ls_copy;
        std::unique_copy(ls.begin(), ls.end(), std::back_inserter(ls_copy),
            boost::geometry::equal_to<point_2d>());
        ls = ls_copy;
        std::cout << "uniquecopy: " << dsv(ls) << std::endl;
    }

    // Lines can be simplified. This removes points, but preserves the shape
    linestring_2d ls_simplified;
    simplify(ls, ls_simplified, 0.5);
    std::cout << "simplified: " << dsv(ls_simplified) << std::endl;


    // for_each:
    // 1) Lines can be visited with std::for_each
    // 2) for_each_point is also defined for all geometries
    // 3) for_each_segment is defined for all geometries to all segments
    // 4) loop is defined for geometries to visit segments
    //    with state apart, and to be able to break out (not shown here)
    {
        linestring_2d lscopy = ls;
        std::for_each(lscopy.begin(), lscopy.end(), translate_function<point_2d>);
        for_each_point(lscopy, scale_functor<point_2d>());
        for_each_point(lscopy, translate_function<point_2d>);
        std::cout << "modified line: " << dsv(lscopy) << std::endl;
    }

    // Lines can be clipped using a clipping box. Clipped lines are added to the output iterator
    box_2d cb(point_2d(1.5, 1.5), point_2d(4.5, 2.5));

    std::vector<linestring_2d> clipped;
    intersection(cb, ls, clipped);

    // Also possible: clip-output to a vector of vectors
    std::vector<std::vector<point_2d> > vector_out;
    intersection(cb, ls, vector_out);

    std::cout << "clipped output as vector:" << std::endl;
    for (std::vector<std::vector<point_2d> >::const_iterator it
            = vector_out.begin(); it != vector_out.end(); ++it)
    {
        std::cout << dsv(*it) << std::endl;
    }

    // Calculate the convex hull of the linestring
    model::polygon<point_2d> hull;
    convex_hull(ls, hull);
    std::cout << "Convex hull:" << dsv(hull) << std::endl;

    // All the above assumed 2D Cartesian linestrings. 3D is possible as well
    // Let's define a 3D point ourselves, this time using 'float'
    typedef model::point<float, 3, cs::cartesian> point_3d;
    model::linestring<point_3d> line3;
    line3.push_back(make<point_3d>(1,2,3));
    line3.push_back(make<point_3d>(4,5,6));
    line3.push_back(make<point_3d>(7,8,9));

    // Not all algorithms work on 3d lines. For example convex hull does NOT.
    // But, for example, length, distance, simplify, envelope and stream do.
    std::cout << "3D: length: " << length(line3) << " line: " << dsv(line3) << std::endl;

    // With DSV you can also use other delimiters, e.g. JSON style
    std::cout << "JSON: "
        << dsv(ls, ", ", "[", "]", ", ", "[ ", " ]")
        << std::endl;

    return 0;
}
