/**
 *
 * Copyright (c) 2010 Matthias Walter (xammy@xammy.homelinux.net)
 *
 * Authors: Matthias Walter
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/lookup_edge.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/bipartite.hpp>

/// Verifies a 2-coloring

template <typename Graph, typename ColorMap>
void check_two_coloring (const Graph& g, const ColorMap color_map)
{
  typedef boost::graph_traits <Graph> traits;
  typename traits::edge_iterator edge_iter, edge_end;

  for (boost::tie (edge_iter, edge_end) = boost::edges (g); edge_iter != edge_end; ++edge_iter)
  {
    typename traits::vertex_descriptor source, target;
    source = boost::source (*edge_iter, g);
    target = boost::target (*edge_iter, g);
    BOOST_REQUIRE (boost::get(color_map, source) != boost::get(color_map, target));
  }
}

/// Tests for a vertex sequence to define an odd cycle

template <typename Graph, typename RandomAccessIterator>
void check_odd_cycle (const Graph& g, RandomAccessIterator first, RandomAccessIterator beyond)
{
  typedef boost::graph_traits <Graph> traits;

  typename traits::vertex_descriptor first_vertex, current_vertex, last_vertex;

  BOOST_CHECK ((beyond - first) % 2 == 1);

  //  std::cout << "odd_cycle: " << int(*first) << std::endl;

  for (first_vertex = current_vertex = *first++; first != beyond; ++first)
  {
    //    std::cout << "odd_cycle: " << int(*first) << std::endl;

    last_vertex = current_vertex;
    current_vertex = *first;

    BOOST_REQUIRE (boost::lookup_edge (current_vertex, last_vertex, g).second);
  }

  BOOST_REQUIRE (boost::lookup_edge (first_vertex, current_vertex, g).second);
}

/// Call the is_bipartite and find_odd_cycle functions and verify their results.

template <typename Graph, typename IndexMap>
void check_bipartite (const Graph& g, IndexMap index_map, bool is_bipartite)
{
  typedef boost::graph_traits <Graph> traits;
  typedef std::vector <boost::default_color_type> partition_t;
  typedef std::vector <typename traits::vertex_descriptor> vertex_vector_t;
  typedef boost::iterator_property_map <partition_t::iterator, IndexMap> partition_map_t;

  partition_t partition (boost::num_vertices (g));
  partition_map_t partition_map (partition.begin (), index_map);

  vertex_vector_t odd_cycle (boost::num_vertices (g));

  bool first_result = boost::is_bipartite (g, index_map, partition_map);

  BOOST_REQUIRE (first_result == boost::is_bipartite(g, index_map));

  if (first_result)
    check_two_coloring (g, partition_map);

  BOOST_CHECK (first_result == is_bipartite);

  typename vertex_vector_t::iterator second_first = odd_cycle.begin ();
  typename vertex_vector_t::iterator second_beyond = boost::find_odd_cycle (g, index_map, partition_map, second_first);

  if (is_bipartite)
  {
    BOOST_CHECK (second_beyond == second_first);
    check_two_coloring (g, partition_map);
  }
  else
  {
    check_odd_cycle (g, second_first, second_beyond);
  }

  second_beyond = boost::find_odd_cycle (g, index_map, second_first);
  if (is_bipartite)
  {
    BOOST_CHECK (second_beyond == second_first);
  }
  else
  {
    check_odd_cycle (g, second_first, second_beyond);
  }
}

int test_main (int argc, char **argv)
{
  typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS> vector_graph_t;
  typedef boost::adjacency_list <boost::listS, boost::listS, boost::undirectedS> list_graph_t;
  typedef std::pair <int, int> E;

  typedef std::map <boost::graph_traits <list_graph_t>::vertex_descriptor, size_t> index_map_t;
  typedef boost::associative_property_map <index_map_t> index_property_map_t;

  /**
   * Create the graph drawn below.
   *
   *       0 - 1 - 2
   *       |       |
   *   3 - 4 - 5 - 6
   *  /      \   /
   *  |        7
   *  |        |
   *  8 - 9 - 10
   **/

  E bipartite_edges[] = { E (0, 1), E (0, 4), E (1, 2), E (2, 6), E (3, 4), E (3, 8), E (4, 5), E (4, 7), E (5, 6), E (
      6, 7), E (7, 10), E (8, 9), E (9, 10) };
  vector_graph_t bipartite_vector_graph (&bipartite_edges[0],
      &bipartite_edges[0] + sizeof(bipartite_edges) / sizeof(E), 11);
  list_graph_t
      bipartite_list_graph (&bipartite_edges[0], &bipartite_edges[0] + sizeof(bipartite_edges) / sizeof(E), 11);

  /**
   * Create the graph drawn below.
   * 
   *       2 - 1 - 0
   *       |       |
   *   3 - 6 - 5 - 4
   *  /      \   /
   *  |        7
   *  |       /
   *  8 ---- 9
   *  
   **/

  E non_bipartite_edges[] = { E (0, 1), E (0, 4), E (1, 2), E (2, 6), E (3, 4), E (3, 8), E (4, 5), E (4, 7), E (5, 6),
      E (6, 7), E (7, 9), E (8, 9) };
  vector_graph_t non_bipartite_vector_graph (&non_bipartite_edges[0], &non_bipartite_edges[0]
      + sizeof(non_bipartite_edges) / sizeof(E), 10);
  list_graph_t non_bipartite_list_graph (&non_bipartite_edges[0], &non_bipartite_edges[0] + sizeof(non_bipartite_edges)
      / sizeof(E), 10);

  /// Create index maps

  index_map_t bipartite_index_map, non_bipartite_index_map;
  boost::graph_traits <list_graph_t>::vertex_iterator vertex_iter, vertex_end;
  size_t i = 0;
  for (boost::tie (vertex_iter, vertex_end) = boost::vertices (bipartite_list_graph); vertex_iter != vertex_end; ++vertex_iter)
  {
    bipartite_index_map[*vertex_iter] = i++;
  }
  index_property_map_t bipartite_index_property_map = index_property_map_t (bipartite_index_map);

  i = 0;
  for (boost::tie (vertex_iter, vertex_end) = boost::vertices (non_bipartite_list_graph); vertex_iter != vertex_end; ++vertex_iter)
  {
    non_bipartite_index_map[*vertex_iter] = i++;
  }
  index_property_map_t non_bipartite_index_property_map = index_property_map_t (non_bipartite_index_map);

  /// Call real checks

  check_bipartite (bipartite_vector_graph, boost::get (boost::vertex_index, bipartite_vector_graph), true);
  check_bipartite (bipartite_list_graph, bipartite_index_property_map, true);

  check_bipartite (non_bipartite_vector_graph, boost::get (boost::vertex_index, non_bipartite_vector_graph), false);
  check_bipartite (non_bipartite_list_graph, non_bipartite_index_property_map, false);

  /// Test some more interfaces

  BOOST_REQUIRE (is_bipartite (bipartite_vector_graph));
  BOOST_REQUIRE (!is_bipartite (non_bipartite_vector_graph));

  return 0;
}
