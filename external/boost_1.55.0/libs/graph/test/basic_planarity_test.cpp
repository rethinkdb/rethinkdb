//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/test/minimal.hpp>


using namespace boost;


struct VertexIndexUpdater
{
  template <typename Graph>
  void reset(Graph& g)
  {
    typename property_map<Graph, vertex_index_t>::type index = get(vertex_index, g);
    typename graph_traits<Graph>::vertex_iterator vi, vi_end;
    typename graph_traits<Graph>::vertices_size_type cnt = 0;
    for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
      put(index, *vi, cnt++);
  }
};

struct NoVertexIndexUpdater
{
  template <typename Graph> void reset(Graph&) {}
};



template <typename Graph, typename VertexIndexUpdater>
void test_K_5(VertexIndexUpdater vertex_index)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;

  Graph g;
  vertex_t v1 = add_vertex(g);
  vertex_t v2 = add_vertex(g);
  vertex_t v3 = add_vertex(g);
  vertex_t v4 = add_vertex(g);
  vertex_t v5 = add_vertex(g);
  vertex_index.reset(g);

  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v2, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v3, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v3, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v3, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v3, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  
  //This edge should make the graph non-planar
  add_edge(v4, v5, g);
  BOOST_CHECK(!boyer_myrvold_planarity_test(g));

}





template <typename Graph, typename VertexIndexUpdater>
void test_K_3_3(VertexIndexUpdater vertex_index)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;

  Graph g;
  vertex_t v1 = add_vertex(g);
  vertex_t v2 = add_vertex(g);
  vertex_t v3 = add_vertex(g);
  vertex_t v4 = add_vertex(g);
  vertex_t v5 = add_vertex(g);
  vertex_t v6 = add_vertex(g);
  vertex_index.reset(g);

  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v1, v6, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v2, v6, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v3, v4, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  add_edge(v3, v5, g);
  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  
  //This edge should make the graph non-planar
  add_edge(v3, v6, g);
  BOOST_CHECK(!boyer_myrvold_planarity_test(g));

}





// This test creates a maximal planar graph on num_vertices vertices,
// then, if num_vertices is at least 5, adds an additional edge to
// create a non-planar graph.

template <typename Graph, typename VertexIndexUpdater>
void test_maximal_planar(VertexIndexUpdater vertex_index, std::size_t num_vertices)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;

  Graph g;
  std::vector<vertex_t> vmap;
  for(std::size_t i = 0; i < num_vertices; ++i)
    vmap.push_back(add_vertex(g));

  vertex_index.reset(g);

  BOOST_CHECK(boyer_myrvold_planarity_test(g));
  //create a cycle
  for(std::size_t i = 0; i < num_vertices; ++i)
    {
      add_edge(vmap[i], vmap[(i+1) % num_vertices], g);
      BOOST_CHECK(boyer_myrvold_planarity_test(g));
    }

  //triangulate the interior of the cycle.
  for(std::size_t i = 2; i < num_vertices - 1; ++i)
    {
      add_edge(vmap[0], vmap[i], g);
      BOOST_CHECK(boyer_myrvold_planarity_test(g));
    }

  //triangulate the exterior of the cycle.
  for(std::size_t i = 3; i < num_vertices; ++i)
    {
      add_edge(vmap[1], vmap[i], g);
      BOOST_CHECK(boyer_myrvold_planarity_test(g));
    }

  //Now add an additional edge, forcing the graph to be non-planar.
  if (num_vertices > 4)
    {
      add_edge(vmap[2], vmap[4], g);
      BOOST_CHECK(!boyer_myrvold_planarity_test(g));      
    }

}





int test_main(int, char* [])
{
  typedef adjacency_list 
    <vecS, 
    vecS, 
    undirectedS,
    property<vertex_index_t, int>
    > 
    VVgraph_t;
  
  typedef adjacency_list 
    <vecS, 
    listS, 
    undirectedS,
    property<vertex_index_t, int>
    > 
    VLgraph_t;

  typedef adjacency_list
    <listS, 
    vecS, 
    undirectedS,
    property<vertex_index_t, int>
    > 
    LVgraph_t;

  typedef adjacency_list 
    <listS, 
    listS, 
    undirectedS,
    property<vertex_index_t, int>
    > 
    LLgraph_t;

  typedef adjacency_list 
    <setS, 
    setS, 
    undirectedS,
    property<vertex_index_t, int>
    > 
    SSgraph_t;

  test_K_5<VVgraph_t>(NoVertexIndexUpdater());
  test_K_3_3<VVgraph_t>(NoVertexIndexUpdater());
  test_maximal_planar<VVgraph_t>(NoVertexIndexUpdater(), 3);
  test_maximal_planar<VVgraph_t>(NoVertexIndexUpdater(), 6);
  test_maximal_planar<VVgraph_t>(NoVertexIndexUpdater(), 10);
  test_maximal_planar<VVgraph_t>(NoVertexIndexUpdater(), 20);
  test_maximal_planar<VVgraph_t>(NoVertexIndexUpdater(), 50);

  test_K_5<VLgraph_t>(VertexIndexUpdater());
  test_K_3_3<VLgraph_t>(VertexIndexUpdater());
  test_maximal_planar<VLgraph_t>(VertexIndexUpdater(), 3);
  test_maximal_planar<VLgraph_t>(VertexIndexUpdater(), 6);
  test_maximal_planar<VLgraph_t>(VertexIndexUpdater(), 10);
  test_maximal_planar<VLgraph_t>(VertexIndexUpdater(), 20);
  test_maximal_planar<VLgraph_t>(VertexIndexUpdater(), 50);
  
  test_K_5<LVgraph_t>(NoVertexIndexUpdater());
  test_K_3_3<LVgraph_t>(NoVertexIndexUpdater());
  test_maximal_planar<LVgraph_t>(NoVertexIndexUpdater(), 3);
  test_maximal_planar<LVgraph_t>(NoVertexIndexUpdater(), 6);
  test_maximal_planar<LVgraph_t>(NoVertexIndexUpdater(), 10);
  test_maximal_planar<LVgraph_t>(NoVertexIndexUpdater(), 20);
  test_maximal_planar<LVgraph_t>(NoVertexIndexUpdater(), 50);

  test_K_5<LLgraph_t>(VertexIndexUpdater());
  test_K_3_3<LLgraph_t>(VertexIndexUpdater());
  test_maximal_planar<LLgraph_t>(VertexIndexUpdater(), 3);
  test_maximal_planar<LLgraph_t>(VertexIndexUpdater(), 6);
  test_maximal_planar<LLgraph_t>(VertexIndexUpdater(), 10);
  test_maximal_planar<LLgraph_t>(VertexIndexUpdater(), 20);
  test_maximal_planar<LLgraph_t>(VertexIndexUpdater(), 50);

  test_K_5<SSgraph_t>(VertexIndexUpdater());
  test_K_3_3<SSgraph_t>(VertexIndexUpdater());
  test_maximal_planar<SSgraph_t>(VertexIndexUpdater(), 3);
  test_maximal_planar<SSgraph_t>(VertexIndexUpdater(), 6);
  test_maximal_planar<SSgraph_t>(VertexIndexUpdater(), 10);
  test_maximal_planar<SSgraph_t>(VertexIndexUpdater(), 20);
  test_maximal_planar<SSgraph_t>(VertexIndexUpdater(), 50);

  return 0;
}
