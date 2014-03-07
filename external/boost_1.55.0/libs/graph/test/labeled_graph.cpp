// (C) Copyright 2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <set>

#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/labeled_graph.hpp>

#include "typestr.hpp"

using std::cout;
using std::string;
using namespace boost;

void test_norm();
void test_temp();
void test_bacon();

int main() {
    test_norm();
    test_temp();
    test_bacon();
}

//////////////////////////////////////
// Utility Functions and Types
//////////////////////////////////////


struct Actor {
    Actor() { }
    Actor(string const& s) : name(s) { }
    string name;
};

struct Movie {
    Movie() { }
    Movie(string const& s) : name(s) { }
    string name;
};


template <typename Graph>
void init_graph(Graph& g) {
    for(int i = 0; i < 6; ++i) {
        add_vertex(i, g);
    }
}

template <typename Graph>
void label_graph(Graph& g)
{
    typedef typename graph_traits<Graph>::vertex_iterator Iter;
    Iter f, l;
    int x = 0;
    for(boost::tie(f, l) = vertices(g); f != l; ++f, ++x) {
        label_vertex(*f, x, g);
    }
}

template <typename Graph>
void build_graph(Graph& g) {
    // This is the graph shown on the wikipedia page for Graph Theory.
    add_edge_by_label(5, 3, g);
    add_edge_by_label(3, 4, g);
    add_edge_by_label(3, 2, g);
    add_edge_by_label(4, 1, g);
    add_edge_by_label(4, 0, g);
    add_edge_by_label(2, 1, g);
    add_edge_by_label(1, 0, g);
    BOOST_ASSERT(num_vertices(g) == 6);
    BOOST_ASSERT(num_edges(g) == 7);
}

//////////////////////////////////////
// Temporal Labelings
//////////////////////////////////////

void test_norm() {
    {
        typedef labeled_graph<undirected_graph<>, unsigned> Graph;
        Graph g;
        init_graph(g);
        build_graph(g);
    }

    {
        typedef labeled_graph<directed_graph<>, unsigned> Graph;
        Graph g;
        init_graph(g);
        build_graph(g);
    }
}

//////////////////////////////////////
// Temporal Labelings
//////////////////////////////////////


void test_temp() {
    typedef undirected_graph<> Graph;
    typedef labeled_graph<Graph*, int> LabGraph;
    Graph g(6);
    LabGraph lg(&g);
    label_graph(lg);
    build_graph(lg);
}

//////////////////////////////////////
// Labeled w/ Properties
//////////////////////////////////////

void test_bacon() {
        string bacon("Kevin Bacon");
        string slater("Christian Slater");
        Movie murder("Murder in the First");
    {

        typedef labeled_graph<undirected_graph<Actor, Movie>, string> Graph;
        Graph g;
        add_vertex(bacon, g);
        add_vertex(slater, g);
        add_edge_by_label(bacon, slater, murder, g);
    }

    {
        string bacon = "Kevin Bacon";
        string slater = "Christian Slater";

        typedef labeled_graph<directed_graph<Actor, Movie>, string> Graph;
        Graph g;
        add_vertex(bacon, g);
        add_vertex(slater, g);
        add_edge_by_label(bacon, slater, murder, g);
    }
}
