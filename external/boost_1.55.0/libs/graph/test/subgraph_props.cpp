// (C) Copyright Andrew Sutton 2009
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>
#include "typestr.hpp"

using namespace boost;

struct TestProps {
    typedef property<vertex_name_t, std::size_t> VertexProp;
    typedef property<edge_name_t, std::size_t> EdgeName;
    typedef property<edge_index_t, std::size_t, EdgeName> EdgeProp;

    typedef adjacency_list<
        vecS, vecS, bidirectionalS, VertexProp, EdgeProp
    > Graph;

    typedef subgraph<Graph> Subgraph;
    typedef graph_traits<Subgraph>::vertex_descriptor Vertex;
    typedef graph_traits<Subgraph>::edge_descriptor Edge;
    typedef graph_traits<Subgraph>::vertex_iterator VertexIter;
    typedef std::pair<VertexIter, VertexIter> VertexRange;

    static void run() {
        // Create a graph with some vertices.
        Subgraph g(5);
        VertexRange r = vertices(g);

        // Create a child subgraph and add some vertices.
        Subgraph& sg = g.create_subgraph();
        Vertex v = add_vertex(*r.first, sg);

        typedef property_map<Subgraph, vertex_name_t>::type DefaultMap;
        DefaultMap map = get(vertex_name, g);
        BOOST_ASSERT(get(map, v) == 0);
        put(map, v, 5);
        BOOST_ASSERT(get(map, v) == 5);

        typedef global_property<vertex_name_t> GlobalProp;
        typedef property_map<Subgraph, GlobalProp>::type GlobalVertMap;
        GlobalVertMap groot = get(global(vertex_name), g);
        GlobalVertMap gsub = get(global(vertex_name), sg);
        BOOST_ASSERT(get(groot, v) == 5);
        BOOST_ASSERT(get(gsub, v) == 5);
        put(gsub, v, 10);
        BOOST_ASSERT(get(groot, v) == 10);
        BOOST_ASSERT(get(gsub, v) == 10);
        BOOST_ASSERT(get(map, v) == 10);

        typedef local_property<vertex_name_t> LocalProp;
        typedef property_map<Subgraph, LocalProp>::type LocalVertMap;
        LocalVertMap lroot = get(local(vertex_name), g); // Actually global!
        LocalVertMap lsub = get(local(vertex_name), sg);
        BOOST_ASSERT(get(lroot, v) == 10);  // Recall it's 10 from above!
        BOOST_ASSERT(get(lsub, v) == 0);
        put(lsub, v, 5);
        BOOST_ASSERT(get(lsub, v) == 5);
        BOOST_ASSERT(get(lroot, v) == 10);  // Don't change the root prop
        BOOST_ASSERT(get(map, v) == 10);    // Don't change the root prop

//         typedef detail::subgraph_local_pmap::bind_<LocalProp, Subgraph, void> PM;
//         std::cout << typestr<PM::TagType>() << "\n";
//         std::cout << typestr<PM::PMap>() << "\n";
    }
};

struct TestBundles {
    struct Node {
        Node() : value(-1) { }
        int value;
    };
    struct Arc {
        Arc() : value(-1) { }
        int value;
    };
    typedef property<edge_index_t, std::size_t, Arc> EdgeProp;

    typedef adjacency_list<
        vecS, vecS, bidirectionalS, Node, EdgeProp
    > Graph;

    typedef subgraph<Graph> Subgraph;
    typedef graph_traits<Subgraph>::vertex_descriptor Vertex;
    typedef graph_traits<Subgraph>::edge_descriptor Edge;
    typedef graph_traits<Subgraph>::vertex_iterator VertexIter;
    typedef std::pair<VertexIter, VertexIter> VertexRange;

    static void run() {
        // Create a graph with some vertices.
        Subgraph g(5);
        VertexRange r = vertices(g);

        // Create a child subgraph and add some vertices.
        Subgraph& sg = g.create_subgraph();
        Vertex v = add_vertex(*r.first, sg);

        sg[v].value = 1;
        BOOST_ASSERT(sg[v].value == 1);
        BOOST_ASSERT(sg[global(v)].value == 1);
        BOOST_ASSERT(sg[local(v)].value == -1);

        sg[local(v)].value = 5;
        BOOST_ASSERT(sg[local(v)].value == 5);
        BOOST_ASSERT(sg[global(v)].value == 1);
        BOOST_ASSERT(sg[v].value == 1);

        typedef property_map<
            Subgraph, local_property<int Node::*>
        >::type LocalVertMap;
        LocalVertMap lvm = get(local(&Node::value), sg);
        BOOST_ASSERT(get(lvm, v) == 5);

        typedef property_map<
            Subgraph, global_property<int Node::*>
        >::type GlobalVertMap;
        GlobalVertMap gvm = get(global(&Node::value), sg);
        BOOST_ASSERT(get(gvm, v) == 1);
    }
};

int main(int argc, char* argv[])
{
    TestProps::run();
    TestBundles::run();

    return 0;
}
