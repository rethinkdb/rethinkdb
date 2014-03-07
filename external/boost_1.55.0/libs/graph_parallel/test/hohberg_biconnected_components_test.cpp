// Copyright (C) 2005-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//
// Test of Hohberg's distributed biconnected components algorithm.
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/hohberg_biconnected_components.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/test/minimal.hpp>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using boost::graph::distributed::mpi_process_group;

using namespace boost;

template<typename Graph>
void check_components(const Graph& g, std::size_t num_comps)
{
  std::size_t not_mapped = (std::numeric_limits<std::size_t>::max)();
  std::vector<std::size_t> color_to_name(num_comps, not_mapped);
  BGL_FORALL_EDGES_T(e, g, Graph) {
    BOOST_CHECK(get(edge_color, g, e) < num_comps);
    if (color_to_name[get(edge_color, g, e)] == not_mapped)
      color_to_name[get(edge_color, g, e)] = get(edge_name, g, e);
    BOOST_CHECK(color_to_name[get(edge_color,g,e)] == get(edge_name,g,e));

    if (color_to_name[get(edge_color,g,e)] != get(edge_name,g,e)) {
      for (std::size_t i = 0; i < color_to_name.size(); ++i)
        std::cerr << color_to_name[i] << ' ';
        
      std::cerr << std::endl;

      std::cerr << color_to_name[get(edge_color,g,e)] << " != "
                << get(edge_name,g,e) << " on edge " 
                << local(source(e, g)) << " -> " << local(target(e, g)) 
                << std::endl;
    }
  }
}

template<typename Graph>
void 
test_small_hohberg_biconnected_components(Graph& g, int n, int comps_expected,
                                          bool single_component = true)
{
  using boost::graph::distributed::hohberg_biconnected_components;

  bool is_root = (process_id(process_group(g)) == 0);

  if (single_component) {
    for (int i = 0; i < n; ++i) {
      if (is_root) std::cerr << "Testing with leader = " << i << std::endl;
      
      // Scramble edge_color_map
      BGL_FORALL_EDGES_T(e, g, Graph)
        put(edge_color, g, e, 17);
      
      typename graph_traits<Graph>::vertex_descriptor leader = vertex(i, g);
      int num_comps = 
        hohberg_biconnected_components(g, get(edge_color, g), &leader, 
                                       &leader + 1);
      
      BOOST_CHECK(num_comps == comps_expected);
      check_components(g, num_comps);
    }
  } 

  if (is_root) std::cerr << "Testing simple interface." << std::endl;
  synchronize(g);

  // Scramble edge_color_map
  int i = 0;
  BGL_FORALL_EDGES_T(e, g, Graph) {
    ++i;
    put(edge_color, g, e, 17);
  }
  std::cerr << process_id(process_group(g)) << " has "
            << i << " edges.\n";

  int num_comps = hohberg_biconnected_components(g, get(edge_color, g));

  BOOST_CHECK(num_comps == comps_expected);
  check_components(g, num_comps);
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  typedef adjacency_list<listS,
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         // Vertex properties
                         no_property,
                         // Edge properties
                         property<edge_name_t, std::size_t, 
                         property<edge_color_t, std::size_t> > > Graph;

  typedef std::pair<int, int> E;

  {
    // Example 1: A single component with 2 bicomponents
    E edges_init[] = { E(0, 1), E(0, 2), E(1, 3), E(2, 4), E(3, 4), E(4, 5), 
                       E(4, 6), E(5, 6) };
    const int m = sizeof(edges_init) / sizeof(E);
    std::size_t expected_components[m] = { 0, 0, 0, 0, 0, 1, 1, 1 };
    const int n = 7;
    Graph g(&edges_init[0], &edges_init[0] + m, &expected_components[0], n);

    int num_comps_expected = 
      *std::max_element(&expected_components[0], &expected_components[0] + m)
      + 1;

    test_small_hohberg_biconnected_components(g, n, num_comps_expected);
  }

  {
    // Example 2: A single component with 4 bicomponents
    E edges_init[] = { E(0, 1), E(1, 2), E(2, 0), E(2, 3), E(3, 4), E(4, 5), 
                       E(5, 2), E(3, 6), E(6, 7), E(7, 8), E(8, 6) };
    const int m = sizeof(edges_init) / sizeof(E);
    std::size_t expected_components[m] = { 0, 0, 0, 1, 1, 1, 1, 2, 3, 3, 3 };
    const int n = 9;    
    Graph g(&edges_init[0], &edges_init[0] + m, &expected_components[0], n);

    int num_comps_expected = 
      *std::max_element(&expected_components[0], &expected_components[0] + m)
      + 1;

    test_small_hohberg_biconnected_components(g, n, num_comps_expected);
  }

  {
    // Example 3: Two components, 6 bicomponents
    // This is just the concatenation of the two previous graphs.
    E edges_init[] = { /* Example 1 graph */
                       E(0, 1), E(0, 2), E(1, 3), E(2, 4), E(3, 4), E(4, 5), 
                       E(4, 6), E(5, 6),
                       /* Example 2 graph */
                       E(7, 8), E(8, 9), E(9, 7), E(9, 10), E(10, 11), 
                       E(11, 12), E(12, 9), E(10, 13), E(13, 14), E(14, 15), 
                       E(15, 13) };
    const int m = sizeof(edges_init) / sizeof(E);
    std::size_t expected_components[m] = 
      { /* Example 1 */0, 0, 0, 0, 0, 1, 1, 1,
        /* Example 2 */2, 2, 2, 3, 3, 3, 3, 4, 5, 5, 5 };
    const int n = 16;    
    Graph g(&edges_init[0], &edges_init[0] + m, &expected_components[0], n);

    int num_comps_expected = 
      *std::max_element(&expected_components[0], &expected_components[0] + m)
      + 1;

    test_small_hohberg_biconnected_components(g, n, num_comps_expected,
                                              false);
  }

  return 0;
}
