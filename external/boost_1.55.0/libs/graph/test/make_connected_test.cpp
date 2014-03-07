//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/make_connected.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/test/minimal.hpp>


using namespace boost;


template <typename Graph>
void reset_edge_index(Graph& g)
{
  typename property_map<Graph, edge_index_t>::type index = get(edge_index, g);
  typename graph_traits<Graph>::edge_iterator ei, ei_end;
  typename graph_traits<Graph>::edges_size_type cnt = 0;
  for(boost::tie(ei,ei_end) = edges(g); ei != ei_end; ++ei)
    put(index, *ei, cnt++);
}



template <typename Graph>
void reset_vertex_index(Graph& g)
{
  typename property_map<Graph, vertex_index_t>::type index = get(vertex_index, g);
  typename graph_traits<Graph>::vertex_iterator vi, vi_end;
  typename graph_traits<Graph>::vertices_size_type cnt = 0;
  for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
    put(index, *vi, cnt++);
}


template <typename Graph>
void make_disconnected_cycles(Graph& g, int num_cycles, int cycle_size)
{
  // This graph will consist of num_cycles cycles, each of which
  // has cycle_size vertices and edges. The entire graph has
  // num_cycles * cycle_size vertices and edges, and requires
  // num_cycles - 1 edges to make it connected

  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;

  for(int i = 0; i < num_cycles; ++i)
    {
      vertex_t first_vertex = add_vertex(g);
      vertex_t prev_vertex;
      vertex_t curr_vertex = first_vertex;
      for(int j = 1; j < cycle_size; ++j)
        {
          prev_vertex = curr_vertex;
          curr_vertex = add_vertex(g);
          add_edge(prev_vertex, curr_vertex, g);
        }
      add_edge(curr_vertex, first_vertex, g);
    }
}





int test_main(int, char* [])
{
  typedef adjacency_list 
    <vecS, 
    vecS, 
    undirectedS,
    property<vertex_index_t, int>,
    property<edge_index_t, int>
    > 
    VVgraph_t;
  
  typedef adjacency_list 
    <vecS, 
    listS, 
    undirectedS,
    property<vertex_index_t, int>,
    property<edge_index_t, int>
    > 
    VLgraph_t;

  typedef adjacency_list
    <listS, 
    vecS, 
    undirectedS,
    property<vertex_index_t, int>,
    property<edge_index_t, int>
    > 
    LVgraph_t;

  typedef adjacency_list 
    <listS, 
    listS, 
    undirectedS,
    property<vertex_index_t, int>,
    property<edge_index_t, int>
    > 
    LLgraph_t;

  VVgraph_t gVV;
  std::size_t num_cycles = 10;
  std::size_t cycle_size = 10;
  make_disconnected_cycles(gVV, num_cycles, cycle_size);
  reset_edge_index(gVV);
  std::vector<int> gVV_components(num_vertices(gVV));
  boost::iterator_property_map<
    std::vector<int>::iterator,
    typename boost::property_map<VVgraph_t, boost::vertex_index_t>::const_type
  > gVV_components_pm(gVV_components.begin(), get(boost::vertex_index, gVV));
  BOOST_CHECK(connected_components(gVV, gVV_components_pm) == 
              static_cast<int>(num_cycles));
  make_connected(gVV);
  BOOST_CHECK(connected_components(gVV, gVV_components_pm) == 1);
  BOOST_CHECK(num_edges(gVV) == num_cycles * cycle_size + num_cycles - 1);

  LVgraph_t gLV;
  num_cycles = 20;
  cycle_size = 20;
  make_disconnected_cycles(gLV, num_cycles, cycle_size);
  reset_edge_index(gLV);
  std::vector<int> gLV_components(num_vertices(gLV));
  boost::iterator_property_map<
    std::vector<int>::iterator,
    typename boost::property_map<VVgraph_t, boost::vertex_index_t>::const_type
  > gLV_components_pm(gLV_components.begin(), get(boost::vertex_index, gLV));
  BOOST_CHECK(connected_components(gLV, gLV_components_pm) == 
              static_cast<int>(num_cycles));
  make_connected(gLV);
  BOOST_CHECK(connected_components(gLV, gLV_components_pm) == 1);
  BOOST_CHECK(num_edges(gLV) == num_cycles * cycle_size + num_cycles - 1);

  VLgraph_t gVL;
  num_cycles = 30;
  cycle_size = 30;
  make_disconnected_cycles(gVL, num_cycles, cycle_size);
  reset_edge_index(gVL);
  reset_vertex_index(gVL);
  BOOST_CHECK(connected_components(gVL, make_vector_property_map<int>(get(vertex_index,gVL))) 
              == static_cast<int>(num_cycles)
              );
  make_connected(gVL);
  BOOST_CHECK(connected_components(gVL, make_vector_property_map<int>(get(vertex_index,gVL))) 
              == 1
              );
  BOOST_CHECK(num_edges(gVL) == num_cycles * cycle_size + num_cycles - 1);

  LLgraph_t gLL;
  num_cycles = 40;
  cycle_size = 40;
  make_disconnected_cycles(gLL, num_cycles, cycle_size);
  reset_edge_index(gLL);
  reset_vertex_index(gLL);
  BOOST_CHECK(connected_components(gLL, make_vector_property_map<int>(get(vertex_index,gLL))) 
              == static_cast<int>(num_cycles));
  make_connected(gLL);
  BOOST_CHECK(connected_components(gLL, make_vector_property_map<int>(get(vertex_index,gLL))) 
              == 1
              );
  BOOST_CHECK(num_edges(gLL) == num_cycles * cycle_size + num_cycles - 1);

  // Now make sure that no edges are added to an already connected graph
  // when you call make_connected again

  graph_traits<VVgraph_t>::edges_size_type VV_num_edges(num_edges(gVV));
  make_connected(gVV);
  BOOST_CHECK(num_edges(gVV) == VV_num_edges);

  graph_traits<VLgraph_t>::edges_size_type VL_num_edges(num_edges(gVL));
  make_connected(gVL);
  BOOST_CHECK(num_edges(gVL) == VL_num_edges);

  graph_traits<LVgraph_t>::edges_size_type LV_num_edges(num_edges(gLV));
  make_connected(gLV);
  BOOST_CHECK(num_edges(gLV) == LV_num_edges);

  graph_traits<LLgraph_t>::edges_size_type LL_num_edges(num_edges(gLL));
  make_connected(gLL);
  BOOST_CHECK(num_edges(gLL) == LL_num_edges);

  return 0;
}
