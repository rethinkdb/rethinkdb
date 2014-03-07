// (C) Copyright 2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef TEST_GRAPH_HPP
#define TEST_GRAPH_HPP

/** @file test_graph.hpp
 * This file houses the generic graph testing framework, which is essentially
 * run using the test_graph function. This function is called, passing a
 * graph object that be constructed and exercised according to the concepts
 * that the graph models. This test is extensively metaprogrammed in order to
 * differentiate testable features of graph instances.
 */

#include "typestr.hpp"

#include <utility>
#include <vector>
#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_mutability_traits.hpp>
#include <boost/graph/labeled_graph.hpp>

#define BOOST_META_ASSERT(x) BOOST_ASSERT(x::value)

typedef std::pair<std::size_t, std::size_t> Pair;

static const std::size_t N = 6;
static const std::size_t M = 7;

// A helper function that globally defines the graph being constructed. Note
// that this graph shown here: http://en.wikipedia.org/wiki/Graph_theory.
std::pair<Pair*, Pair*> edge_pairs() {
    static Pair pairs[] = {
        Pair(5, 3), Pair(3, 4), Pair(3, 2), Pair(4, 0), Pair(4, 1),
        Pair(2, 1), Pair(1, 0)
    };
    Pair* f = &pairs[0];
    Pair* l = f + M;
    return std::make_pair(f, l);
}

// Return true if the vertex v is the target of the edge e.
template <typename Graph, typename Edge, typename Vertex>
bool has_target(Graph const& g, Edge e, Vertex v)
{ return boost::target(e, g) == v; }

// Return true if the vertex v is the source of the edge e.
template <typename Graph, typename Edge, typename Vertex>
bool has_source(Graph const& g, Edge e, Vertex v)
{ return boost::source(e, g) == v; }

/** @name Property Bundles
 * Support testing with bundled properties. Note that the vertex bundle and
 * edge bundle MAY NOT be the same if you want to use the property map type
 * generator to define property maps.
 */
//@{
// This is really just a place holder to make sure that bundled graph
// properties actually work. There are no semantics to this type.
struct GraphBundle {
  int value;
};

struct VertexBundle {
    VertexBundle() : value() { }
    VertexBundle(int n) : value(n) { }

    bool operator==(VertexBundle const& x) const { return value == x.value; }
    bool operator<(VertexBundle const& x) const { return value < x.value; }

    int value;
};

struct EdgeBundle {
    EdgeBundle() : value() { }
    EdgeBundle(int n) : value(n) { }

    bool operator==(EdgeBundle const& x) const { return value == x.value; }
    bool operator<(EdgeBundle const& x) const { return value < x.value; }

    int value;
};
//@}

#include "test_construction.hpp"
#include "test_destruction.hpp"
#include "test_iteration.hpp"
#include "test_direction.hpp"
#include "test_properties.hpp"

template <typename Graph>
void test_graph(Graph& g) {
    using namespace boost;
    BOOST_CONCEPT_ASSERT((GraphConcept<Graph>));

    std::cout << typestr(g) << "\n";

    // Define a bunch of tags for the graph.
    typename graph_has_add_vertex<Graph>::type can_add_vertex;
    typename graph_has_remove_vertex<Graph>::type can_remove_vertex;
    typename is_labeled_graph<Graph>::type is_labeled;
    typename is_directed_unidirectional_graph<Graph>::type is_directed;
    typename is_directed_bidirectional_graph<Graph>::type is_bidirectional;
    typename is_undirected_graph<Graph>::type is_undirected;

    // Test constrution and vertex list.
    build_graph(g, can_add_vertex, is_labeled);
    build_property_graph(g, can_add_vertex, is_labeled);

    test_vertex_list_graph(g);

    // Collect the vertices for an easy method of "naming" them.
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename graph_traits<Graph>::vertex_iterator VertexIterator;
    std::vector<Vertex> verts;
    std::pair<VertexIterator, VertexIterator> rng = vertices(g);
    for( ; rng.first != rng.second; ++rng.first) {
        verts.push_back(*rng.first);
    }

    // Test connection and edge list
    connect_graph(g, verts, is_labeled);
    // connect_property_graph(g, verts, is_labeld);
    test_edge_list_graph(g);

    // Test properties
    test_properties(g, verts);

    // Test directionality.
    test_outdirected_graph(g, verts, is_directed);
    test_indirected_graph(g, verts, is_bidirectional);
    test_undirected_graph(g, verts, is_undirected);

    // Test disconnection
    disconnect_graph(g, verts, is_labeled);
    destroy_graph(g, verts, can_remove_vertex, is_labeled);
}


#endif
