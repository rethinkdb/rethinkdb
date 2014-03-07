// Boost.Geometry Index
// Additional tests

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <fstream>

#define BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/detail/rtree/utilities/statistics.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>

template <typename T, size_t I = 0, size_t S = boost::tuples::length<T>::value>
struct print_tuple
{
    template <typename Os>
    static inline Os & apply(Os & os, T const& t)
    {
        os << boost::get<I>(t) << ", ";
        return print_tuple<T, I+1>::apply(os, t);
    }
};

template <typename T, size_t S>
struct print_tuple<T, S, S>
{
    template <typename Os>
    static inline Os & apply(Os & os, T const&)
    {
        return os;
    }
};

int main()
{
    namespace bg = boost::geometry;
    namespace bgi = bg::index;

    typedef boost::tuple<std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, std::size_t> S;

    typedef bg::model::point<double, 2, bg::cs::cartesian> P;
    typedef bg::model::box<P> B;
    typedef B V;
    //typedef bgi::rtree<V, bgi::linear<16> > RT;
    //typedef bgi::rtree<V, bgi::quadratic<8, 3> > RT;
    //typedef bgi::rtree<V, bgi::rstar<8, 3> > RT;
    typedef bgi::rtree<V, bgi::dynamic_linear > RT;

    //RT tree;
    RT tree(bgi::dynamic_linear(16));
    std::vector<V> vect;

    boost::timer t;

    //insert values
    {
        for ( double x = 0 ; x < 1000 ; x += 1 )
            for ( double y = 0 ; y < 1000 ; y += 1 )
                vect.push_back(B(P(x, y), P(x+0.5, y+0.5)));
        RT tmp(vect, tree.parameters());
        tree = boost::move(tmp);
    }
    B q(P(5, 5), P(6, 6));
    S s;

    std::cout << "vector and tree created in: " << t.elapsed() << std::endl;

    print_tuple<S>::apply(std::cout, bgi::detail::rtree::utilities::statistics(tree)) << std::endl;
    std::cout << boost::get<0>(s) << std::endl;
    BOOST_FOREACH(V const& v, tree | bgi::adaptors::queried(bgi::intersects(q)))
        std::cout << bg::wkt<V>(v) << std::endl;

    // save
    {
        std::ofstream ofs("serialized_vector.bin", std::ios::binary | std::ios::trunc);
        boost::archive::binary_oarchive oa(ofs);
        t.restart();
        oa << vect;
        std::cout << "vector saved to bin in: " << t.elapsed() << std::endl;
    }
    {
        std::ofstream ofs("serialized_tree.bin", std::ios::binary | std::ios::trunc);
        boost::archive::binary_oarchive oa(ofs);
        t.restart();
        oa << tree;
        std::cout << "tree saved to bin in: " << t.elapsed() << std::endl;
    }
    {
        std::ofstream ofs("serialized_tree.xml", std::ios::trunc);
        boost::archive::xml_oarchive oa(ofs);
        t.restart();
        oa << boost::serialization::make_nvp("rtree", tree);
        std::cout << "tree saved to xml in: " << t.elapsed() << std::endl;
    }

    t.restart();
    vect.clear();
    std::cout << "vector cleared in: " << t.elapsed() << std::endl;

    t.restart();
    tree.clear();
    std::cout << "tree cleared in: " << t.elapsed() << std::endl;

    // load

    {
        std::ifstream ifs("serialized_vector.bin", std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        t.restart();
        ia >> vect;
        std::cout << "vector loaded from bin in: " << t.elapsed() << std::endl;
        t.restart();
        RT tmp(vect, tree.parameters());
        tree = boost::move(tmp);
        std::cout << "tree rebuilt from vector in: " << t.elapsed() << std::endl;
    }
    
    t.restart();
    tree.clear();
    std::cout << "tree cleared in: " << t.elapsed() << std::endl;

    {
        std::ifstream ifs("serialized_tree.bin", std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        t.restart();
        ia >> tree;
        std::cout << "tree loaded from bin in: " << t.elapsed() << std::endl;
    }

    std::cout << "loaded from bin" << std::endl;
    print_tuple<S>::apply(std::cout, bgi::detail::rtree::utilities::statistics(tree)) << std::endl;
    BOOST_FOREACH(V const& v, tree | bgi::adaptors::queried(bgi::intersects(q)))
        std::cout << bg::wkt<V>(v) << std::endl;

    t.restart();
    tree.clear();
    std::cout << "tree cleared in: " << t.elapsed() << std::endl;

    {
        std::ifstream ifs("serialized_tree.xml");
        boost::archive::xml_iarchive ia(ifs);
        t.restart();
        ia >> boost::serialization::make_nvp("rtree", tree);
        std::cout << "tree loaded from xml in: " << t.elapsed() << std::endl;
    }

    std::cout << "loaded from xml" << std::endl;
    print_tuple<S>::apply(std::cout, bgi::detail::rtree::utilities::statistics(tree)) << std::endl;
    BOOST_FOREACH(V const& v, tree | bgi::adaptors::queried(bgi::intersects(q)))
        std::cout << bg::wkt<V>(v) << std::endl;

    return 0;
}
