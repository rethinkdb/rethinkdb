// Boost.Geometry (aka GGL, Generic Geometry Library)
// Tool reporting Implementation Support Status in QBK or plain text format

// Copyright (c) 2011-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/type_traits/is_base_of.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_GEOMETRY_IMPLEMENTATION_STATUS_BUILD true
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/convex_hull.hpp>
#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/for_each.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/num_geometries.hpp>
#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/overlaps.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>
#include <boost/geometry/algorithms/reverse.hpp>
#include <boost/geometry/algorithms/simplify.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/algorithms/unique.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/algorithms/append.hpp>
#include <boost/geometry/multi/algorithms/area.hpp>
#include <boost/geometry/multi/algorithms/centroid.hpp>
#include <boost/geometry/multi/algorithms/clear.hpp>
#include <boost/geometry/multi/algorithms/convert.hpp>
#include <boost/geometry/multi/algorithms/correct.hpp>
#include <boost/geometry/multi/algorithms/covered_by.hpp>
#include <boost/geometry/multi/algorithms/distance.hpp>
#include <boost/geometry/multi/algorithms/envelope.hpp>
#include <boost/geometry/multi/algorithms/equals.hpp>
#include <boost/geometry/multi/algorithms/for_each.hpp>
#include <boost/geometry/multi/algorithms/length.hpp>
#include <boost/geometry/multi/algorithms/num_geometries.hpp>
#include <boost/geometry/multi/algorithms/num_interior_rings.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>
#include <boost/geometry/multi/algorithms/perimeter.hpp>
#include <boost/geometry/multi/algorithms/reverse.hpp>
#include <boost/geometry/multi/algorithms/simplify.hpp>
#include <boost/geometry/multi/algorithms/transform.hpp>
#include <boost/geometry/multi/algorithms/unique.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include "text_outputter.hpp"
#include "qbk_outputter.hpp"

typedef boost::geometry::cs::cartesian cartesian;

typedef boost::geometry::model::point<double, 2, cartesian> point_type;
typedef boost::geometry::model::linestring<point_type>      linestring_type;
typedef boost::geometry::model::polygon<point_type>         polygon_type;
typedef boost::geometry::model::box<point_type>             box_type;
typedef boost::geometry::model::ring<point_type>            ring_type;
typedef boost::geometry::model::segment<point_type>         segment_type;

typedef boost::geometry::model::multi_point<point_type>           multi_point_type;
typedef boost::geometry::model::multi_linestring<linestring_type> multi_linestring_type;
typedef boost::geometry::model::multi_polygon<polygon_type>       multi_polygon_type;

typedef boost::mpl::vector<
    point_type,
    segment_type,
    box_type,
    linestring_type,
    ring_type,
    polygon_type,
    multi_point_type,
    multi_linestring_type,
    multi_polygon_type
> all_types;

#define DECLARE_UNARY_ALGORITHM(algorithm) \
    template <typename G> \
    struct algorithm: boost::geometry::dispatch::algorithm<G> \
    {};

#define DECLARE_BINARY_ALGORITHM(algorithm) \
    template <typename G1, typename G2> \
    struct algorithm: boost::geometry::dispatch::algorithm<G1, G2> \
    {};

DECLARE_BINARY_ALGORITHM(append)
DECLARE_UNARY_ALGORITHM(area)
DECLARE_BINARY_ALGORITHM(buffer)
DECLARE_UNARY_ALGORITHM(centroid)
DECLARE_UNARY_ALGORITHM(clear)
DECLARE_BINARY_ALGORITHM(convert)
DECLARE_UNARY_ALGORITHM(convex_hull)
DECLARE_UNARY_ALGORITHM(correct)
DECLARE_BINARY_ALGORITHM(covered_by)
DECLARE_BINARY_ALGORITHM(disjoint)
DECLARE_BINARY_ALGORITHM(distance)
DECLARE_UNARY_ALGORITHM(envelope)
DECLARE_BINARY_ALGORITHM(equals)
DECLARE_BINARY_ALGORITHM(expand)
DECLARE_UNARY_ALGORITHM(for_each_point)
DECLARE_UNARY_ALGORITHM(for_each_segment)
DECLARE_UNARY_ALGORITHM(length)
DECLARE_UNARY_ALGORITHM(num_geometries)
DECLARE_UNARY_ALGORITHM(num_interior_rings)
DECLARE_UNARY_ALGORITHM(num_points)
DECLARE_BINARY_ALGORITHM(overlaps)
DECLARE_UNARY_ALGORITHM(perimeter)
DECLARE_UNARY_ALGORITHM(reverse)
DECLARE_UNARY_ALGORITHM(simplify)
DECLARE_BINARY_ALGORITHM(transform)
DECLARE_UNARY_ALGORITHM(unique)
DECLARE_BINARY_ALGORITHM(within)
DECLARE_UNARY_ALGORITHM(wkt)


template <template <typename> class Dispatcher, typename Outputter, typename G>
struct do_unary_test
{
    Outputter& m_outputter;
    inline do_unary_test(Outputter& outputter)
        : m_outputter(outputter)
    {}

    void operator()()
    {
        if (boost::is_base_of<boost::geometry::nyi::not_implemented_tag, Dispatcher<G> >::type::value)
        {
            m_outputter.nyi();
        }
        else
        {
            m_outputter.ok();
        }
    }
};

template <template <typename, typename> class Dispatcher, typename Outputter, typename G2 = void>
struct do_binary_test
{
    Outputter& m_outputter;
    inline do_binary_test(Outputter& outputter)
        : m_outputter(outputter)
    {}

    template <typename G1>
    void operator()(G1)
    {
        if (boost::is_base_of<boost::geometry::nyi::not_implemented_tag, Dispatcher<G1, G2> >::type::value)
        {
            m_outputter.nyi();
        }
        else
        {
            m_outputter.ok();
        }
    }
};

template <template <typename> class Dispatcher, typename Outputter>
struct unary_test
{
    Outputter& m_outputter;
    inline unary_test(Outputter& outputter)
        : m_outputter(outputter)
    {}

    template <typename G>
    void operator()(G)
    {
         m_outputter.template begin_row<G>();
         do_unary_test<Dispatcher, Outputter, G> test(m_outputter);
         test();
         m_outputter.end_row();
    }
};

template <template <typename, typename> class Dispatcher, typename Types, typename Outputter>
struct binary_test
{
    Outputter& m_outputter;
    inline binary_test(Outputter& outputter)
        : m_outputter(outputter)
    {}

    template <typename G2>
    void operator()(G2)
    {
         m_outputter.template begin_row<G2>();
         boost::mpl::for_each<Types>(do_binary_test<Dispatcher, Outputter, G2>(m_outputter));
         m_outputter.end_row();
    }
};

template <template <typename> class Dispatcher, typename Types, typename Outputter>
void test_unary_algorithm(std::string const& name)
{
    Outputter outputter(name);
    outputter.header(name);

    outputter.table_header();
    boost::mpl::for_each<Types>(unary_test<Dispatcher, Outputter>(outputter));

    outputter.table_footer();
}

template <template <typename, typename> class Dispatcher, typename Types1, typename Types2, typename Outputter>
void test_binary_algorithm(std::string const& name)
{
    Outputter outputter(name);
    outputter.header(name);

    outputter.template table_header<Types2>();
    boost::mpl::for_each<Types1>(binary_test<Dispatcher, Types2, Outputter>(outputter));

    outputter.table_footer();
}


template <typename OutputFactory>
void support_status()
{
    test_binary_algorithm<append, all_types, boost::mpl::vector<point_type, std::vector<point_type> >, OutputFactory>("append");
    test_unary_algorithm<area, all_types, OutputFactory>("area");
    test_binary_algorithm<buffer, all_types, all_types, OutputFactory>("buffer");
    test_unary_algorithm<centroid, all_types, OutputFactory>("centroid");
    test_unary_algorithm<clear, all_types, OutputFactory>("clear");
    test_binary_algorithm<convert, all_types, all_types, OutputFactory>("convert");
    test_unary_algorithm<convex_hull, all_types, OutputFactory>("convex_hull");
    test_unary_algorithm<correct, all_types, OutputFactory>("correct");
    test_binary_algorithm<covered_by, all_types, all_types, OutputFactory>("covered_by");
    test_binary_algorithm<disjoint, all_types, all_types, OutputFactory>("disjoint");
    test_binary_algorithm<distance, all_types, all_types, OutputFactory>("distance");
    test_unary_algorithm<envelope, all_types, OutputFactory>("envelope");
    test_binary_algorithm<equals, all_types, all_types, OutputFactory>("equals");
    test_binary_algorithm<expand, all_types, all_types, OutputFactory>("expand");
    test_unary_algorithm<for_each_point, all_types, OutputFactory>("for_each_point");
    test_unary_algorithm<for_each_segment, all_types, OutputFactory>("for_each_segment");
    test_unary_algorithm<length, all_types, OutputFactory>("length");
    test_unary_algorithm<num_geometries, all_types, OutputFactory>("num_geometries");
    test_unary_algorithm<num_interior_rings, all_types, OutputFactory>("num_interior_rings");
    test_unary_algorithm<num_interior_rings, all_types, OutputFactory>("num_points");
    test_binary_algorithm<overlaps, all_types, all_types, OutputFactory>("overlaps");
    test_unary_algorithm<perimeter, all_types, OutputFactory>("perimeter");
    test_unary_algorithm<reverse, all_types, OutputFactory>("reverse");
    test_unary_algorithm<simplify, all_types, OutputFactory>("simplify");
    test_binary_algorithm<transform, all_types, all_types, OutputFactory>("transform");
    test_unary_algorithm<unique, all_types, OutputFactory>("unique");
    test_binary_algorithm<within, all_types, all_types, OutputFactory>("within");
    test_unary_algorithm<wkt, all_types, OutputFactory>("wkt");
}


int main(int argc, char** argv)
{
    if (argc > 1 && ! strcmp(argv[1], "qbk"))
    {
        support_status<qbk_outputter>();
    }
    else
    {
        support_status<text_outputter>();
    }

    return 0;
}
