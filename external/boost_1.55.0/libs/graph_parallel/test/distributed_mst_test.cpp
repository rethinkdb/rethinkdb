// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/dehne_gotz_min_spanning_tree.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/vertex_list_adaptor.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <iostream>
#include <cstdlib>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;
using boost::graph::distributed::mpi_process_group;

template<typename Graph, typename WeightMap, typename InputIterator>
int
total_weight(const Graph& g, WeightMap weight_map, 
             InputIterator first, InputIterator last)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  int total_weight = 0;
  while (first != last) {
    total_weight += get(weight_map, *first);
    if (process_id(g.process_group()) == 0) {
      vertex_descriptor u = source(*first, g);
      vertex_descriptor v = target(*first, g);
      std::cout << "(" << g.distribution().global(owner(u), local(u))
                << ", " << g.distribution().global(owner(v), local(v))
                << ")\n";
    }
    ++first;
  }

  return total_weight;
}

void 
test_distributed_dense_boruvka()
{
  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         // Vertex properties
                         no_property,
                         // Edge properties
                         property<edge_weight_t, int> > Graph;

  typedef graph_traits<Graph>::edge_descriptor edge_descriptor;

  typedef std::pair<int, int> E;

  const int num_nodes = 5;
  E edge_array[] = { E(0, 2), E(1, 3), E(1, 4), E(2, 1), E(2, 3),
    E(3, 4), E(4, 0), E(4, 1)
  };
  int weights[] = { 1, 1, 2, 7, 3, 1, 1, 1 };
  std::size_t num_edges = sizeof(edge_array) / sizeof(E);

  Graph g(edge_array, edge_array + num_edges, weights, num_nodes);

  {
    if (process_id(g.process_group()) == 0)
      std::cerr << "--Dense Boruvka--\n";
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    WeightMap weight_map = get(edge_weight, g);
    
    std::vector<edge_descriptor> mst_edges;
    dense_boruvka_minimum_spanning_tree(make_vertex_list_adaptor(g), 
                                        weight_map, 
                                        std::back_inserter(mst_edges));
    int w = total_weight(g, weight_map, mst_edges.begin(), mst_edges.end());
    BOOST_CHECK(w == 4);
    BOOST_CHECK(mst_edges.size() == 4);
  }

  {
    if (process_id(g.process_group()) == 0)
      std::cerr << "--Merge local MSTs--\n";
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    WeightMap weight_map = get(edge_weight, g);
    
    std::vector<edge_descriptor> mst_edges;
    merge_local_minimum_spanning_trees(make_vertex_list_adaptor(g), weight_map,
                                       std::back_inserter(mst_edges));
    if (process_id(g.process_group()) == 0) {
      int w = total_weight(g, weight_map, mst_edges.begin(), mst_edges.end());
      BOOST_CHECK(w == 4);
      BOOST_CHECK(mst_edges.size() == 4);
    }
  }

  {
    if (process_id(g.process_group()) == 0)
      std::cerr << "--Boruvka then Merge--\n";
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    WeightMap weight_map = get(edge_weight, g);
    
    std::vector<edge_descriptor> mst_edges;
    boruvka_then_merge(make_vertex_list_adaptor(g), weight_map,
                        std::back_inserter(mst_edges));
    if (process_id(g.process_group()) == 0) {
      int w = total_weight(g, weight_map, mst_edges.begin(), mst_edges.end());
      BOOST_CHECK(w == 4);
      BOOST_CHECK(mst_edges.size() == 4);
    }
  }

  {
    if (process_id(g.process_group()) == 0)
      std::cerr << "--Boruvka mixed Merge--\n";
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    WeightMap weight_map = get(edge_weight, g);
    
    std::vector<edge_descriptor> mst_edges;
    boruvka_mixed_merge(make_vertex_list_adaptor(g), weight_map,
                        std::back_inserter(mst_edges));
    if (process_id(g.process_group()) == 0) {
      int w = total_weight(g, weight_map, mst_edges.begin(), mst_edges.end());
      BOOST_CHECK(w == 4);
      BOOST_CHECK(mst_edges.size() == 4);
    }
  }
}

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);
  test_distributed_dense_boruvka();
  return 0;
}
