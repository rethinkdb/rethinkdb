//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>

#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/neighbor_bfs.hpp>
#include <boost/property_map/property_map.hpp>

/*
  
  This examples shows how to use the breadth_first_search() GGCL
  algorithm, specifically the 3 argument variant of bfs that assumes
  the graph has a color property (property) stored internally.

  Two pre-defined visitors are used to record the distance of each
  vertex from the source vertex, and also to record the parent of each
  vertex. Any number of visitors can be layered and passed to a GGCL
  algorithm.

  The call to vertices(G) returns an STL-compatible container which
  contains all of the vertices in the graph.  In this example we use
  the vertices container in the STL for_each() function.

  Sample Output:

  0 --> 2 
  1 --> 1 3 4 
  2 --> 1 3 4 
  3 --> 1 4 
  4 --> 0 1 
  distances: 1 2 1 2 0 
  parent[0] = 4
  parent[1] = 2
  parent[2] = 0
  parent[3] = 2
  parent[4] = 0

*/

template <class ParentDecorator>
struct print_parent {
  print_parent(const ParentDecorator& p_) : p(p_) { }
  template <class Vertex>
  void operator()(const Vertex& v) const {
    std::cout << "parent[" << v << "] = " <<  p[v]  << std::endl;
  }
  ParentDecorator p;
};


template <class NewGraph, class Tag>
struct graph_copier 
  : public boost::base_visitor<graph_copier<NewGraph, Tag> >
{
  typedef Tag event_filter;

  graph_copier(NewGraph& graph) : new_g(graph) { }

  template <class Edge, class Graph>
  void operator()(Edge e, Graph& g) {
    boost::add_edge(boost::source(e, g), boost::target(e, g), new_g);
  }
private:
  NewGraph& new_g;
};

template <class NewGraph, class Tag>
inline graph_copier<NewGraph, Tag>
copy_graph(NewGraph& g, Tag) {
  return graph_copier<NewGraph, Tag>(g);
}

int main(int , char* []) 
{
  typedef boost::adjacency_list< 
    boost::mapS, boost::vecS, boost::bidirectionalS,
    boost::property<boost::vertex_color_t, boost::default_color_type,
        boost::property<boost::vertex_degree_t, int,
          boost::property<boost::vertex_in_degree_t, int,
    boost::property<boost::vertex_out_degree_t, int> > > >
  > Graph;
  
  Graph G(5);
  boost::add_edge(0, 2, G);
  boost::add_edge(1, 1, G);
  boost::add_edge(1, 3, G);
  boost::add_edge(1, 4, G);
  boost::add_edge(2, 1, G);
  boost::add_edge(2, 3, G);
  boost::add_edge(2, 4, G);
  boost::add_edge(3, 1, G);
  boost::add_edge(3, 4, G);
  boost::add_edge(4, 0, G);
  boost::add_edge(4, 1, G);

  typedef Graph::vertex_descriptor Vertex;

  // Array to store predecessor (parent) of each vertex. This will be
  // used as a Decorator (actually, its iterator will be).
  std::vector<Vertex> p(boost::num_vertices(G));
  // VC++ version of std::vector has no ::pointer, so
  // I use ::value_type* instead.
  typedef std::vector<Vertex>::value_type* Piter;

  // Array to store distances from the source to each vertex .  We use
  // a built-in array here just for variety. This will also be used as
  // a Decorator.  
  boost::graph_traits<Graph>::vertices_size_type d[5];
  std::fill_n(d, 5, 0);

  // The source vertex
  Vertex s = *(boost::vertices(G).first);
  p[s] = s;
  boost::neighbor_breadth_first_search
    (G, s, 
     boost::visitor(boost::make_neighbor_bfs_visitor
     (std::make_pair(boost::record_distances(d, boost::on_tree_edge()),
                     boost::record_predecessors(&p[0], 
                                                 boost::on_tree_edge())))));

  boost::print_graph(G);

  if (boost::num_vertices(G) < 11) {
    std::cout << "distances: ";
#ifdef BOOST_OLD_STREAM_ITERATORS
    std::copy(d, d + 5, std::ostream_iterator<int, char>(std::cout, " "));
#else
    std::copy(d, d + 5, std::ostream_iterator<int>(std::cout, " "));
#endif
    std::cout << std::endl;

    std::for_each(boost::vertices(G).first, boost::vertices(G).second, 
                  print_parent<Piter>(&p[0]));
  }

  return 0;
}
