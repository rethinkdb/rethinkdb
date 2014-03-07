//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <iostream>                         // for std::cout
#include <utility>                          // for std::pair
#include <algorithm>                        // for std::for_each
#include <boost/utility.hpp>                // for boost::tie
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

using namespace boost;

template <class Graph> struct exercise_vertex {
  exercise_vertex(Graph& g_, const char name_[]) : g(g_),name(name_) { }
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  void operator()(const Vertex& v) const
  {
    using namespace boost;
    typename property_map<Graph, vertex_index_t>::type
      vertex_id = get(vertex_index, g);
    std::cout << "vertex: " << name[get(vertex_id, v)] << std::endl;

    // Write out the outgoing edges
    std::cout << "\tout-edges: ";
    typename graph_traits<Graph>::out_edge_iterator out_i, out_end;
    typename graph_traits<Graph>::edge_descriptor e;
    for (boost::tie(out_i, out_end) = out_edges(v, g);
         out_i != out_end; ++out_i)
    {
      e = *out_i;
      Vertex src = source(e, g), targ = target(e, g);
      std::cout << "(" << name[get(vertex_id, src)]
                << "," << name[get(vertex_id, targ)] << ") ";
    }
    std::cout << std::endl;

    // Write out the incoming edges
    std::cout << "\tin-edges: ";
    typename graph_traits<Graph>::in_edge_iterator in_i, in_end;
    for (boost::tie(in_i, in_end) = in_edges(v, g); in_i != in_end; ++in_i)
    {
      e = *in_i;
      Vertex src = source(e, g), targ = target(e, g);
      std::cout << "(" << name[get(vertex_id, src)]
                << "," << name[get(vertex_id, targ)] << ") ";
    }
    std::cout << std::endl;

    // Write out all adjacent vertices
    std::cout << "\tadjacent vertices: ";
    typename graph_traits<Graph>::adjacency_iterator ai, ai_end;
    for (boost::tie(ai,ai_end) = adjacent_vertices(v, g);  ai != ai_end; ++ai)
      std::cout << name[get(vertex_id, *ai)] <<  " ";
    std::cout << std::endl;
  }
  Graph& g;
  const char *name;
};


int main(int,char*[])
{
  // create a typedef for the Graph type
  typedef adjacency_list<vecS, vecS, bidirectionalS,
     no_property, property<edge_weight_t, float> > Graph;

  // Make convenient labels for the vertices
  enum { A, B, C, D, E, N };
  const int num_vertices = N;
  const char name[] = "ABCDE";

  // writing out the edges in the graph
  typedef std::pair<int,int> Edge;
  Edge edge_array[] =
  { Edge(A,B), Edge(A,D), Edge(C,A), Edge(D,C),
    Edge(C,E), Edge(B,D), Edge(D,E), };
  const int num_edges = sizeof(edge_array)/sizeof(edge_array[0]);

  // average transmission delay (in milliseconds) for each connection
  float transmission_delay[] = { 1.2, 4.5, 2.6, 0.4, 5.2, 1.8, 3.3, 9.1 };

  // declare a graph object, adding the edges and edge properties
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle the iterator constructor
  Graph g(num_vertices);
  property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, g);
  for (std::size_t j = 0; j < num_edges; ++j) {
    graph_traits<Graph>::edge_descriptor e; bool inserted;
    boost::tie(e, inserted) = add_edge(edge_array[j].first, edge_array[j].second, g);
    weightmap[e] = transmission_delay[j];
  }
#else
  Graph g(edge_array, edge_array + num_edges,
          transmission_delay, num_vertices);
#endif

  boost::property_map<Graph, vertex_index_t>::type
    vertex_id = get(vertex_index, g);
  boost::property_map<Graph, edge_weight_t>::type
    trans_delay = get(edge_weight, g);

  std::cout << "vertices(g) = ";
  typedef graph_traits<Graph>::vertex_iterator vertex_iter;
  std::pair<vertex_iter, vertex_iter> vp;
  for (vp = vertices(g); vp.first != vp.second; ++vp.first)
    std::cout << name[get(vertex_id, *vp.first)] <<  " ";
  std::cout << std::endl;

  std::cout << "edges(g) = ";
  graph_traits<Graph>::edge_iterator ei, ei_end;
  for (boost::tie(ei,ei_end) = edges(g); ei != ei_end; ++ei)
    std::cout << "(" << name[get(vertex_id, source(*ei, g))]
              << "," << name[get(vertex_id, target(*ei, g))] << ") ";
  std::cout << std::endl;

  std::for_each(vertices(g).first, vertices(g).second,
                exercise_vertex<Graph>(g, name));

  std::map<std::string,std::string> graph_attr, vertex_attr, edge_attr;
  graph_attr["size"] = "3,3";
  graph_attr["rankdir"] = "LR";
  graph_attr["ratio"] = "fill";
  vertex_attr["shape"] = "circle";

  boost::write_graphviz(std::cout, g,
                        make_label_writer(name),
                        make_label_writer(trans_delay),
                        make_graph_attributes_writer(graph_attr, vertex_attr,
                                                     edge_attr));

  return 0;
}


