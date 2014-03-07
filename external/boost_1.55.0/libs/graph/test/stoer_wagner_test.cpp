//            Copyright Daniel Trebbien 2010.
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or the copy at
//         http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/read_dimacs.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple.hpp>

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, boost::property<boost::edge_weight_t, int> > undirected_graph;
typedef boost::property_map<undirected_graph, boost::edge_weight_t>::type weight_map_type;
typedef boost::property_traits<weight_map_type>::value_type weight_type;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> undirected_unweighted_graph;

std::string test_dir;

boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] ) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " path-to-libs-graph-test" << std::endl;
    throw boost::unit_test::framework::setup_error("Invalid command line arguments");
  }
  test_dir = argv[1];
  return 0;
}

struct edge_t
{
  unsigned long first;
  unsigned long second;
};

// the example from Stoer & Wagner (1997)
BOOST_AUTO_TEST_CASE(test0)
{
  edge_t edges[] = {{0, 1}, {1, 2}, {2, 3},
    {0, 4}, {1, 4}, {1, 5}, {2, 6}, {3, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}};
  weight_type ws[] = {2, 3, 4, 3, 2, 2, 2, 2, 2, 3, 1, 3};
  undirected_graph g(edges, edges + 12, ws, 8, 12);
  
  weight_map_type weights = get(boost::edge_weight, g);
  std::map<int, bool> parity;
  boost::associative_property_map<std::map<int, bool> > parities(parity);
  int w = boost::stoer_wagner_min_cut(g, weights, boost::parity_map(parities));
  BOOST_CHECK_EQUAL(w, 4);
  const bool parity0 = get(parities, 0);
  BOOST_CHECK_EQUAL(parity0, get(parities, 1));
  BOOST_CHECK_EQUAL(parity0, get(parities, 4));
  BOOST_CHECK_EQUAL(parity0, get(parities, 5));
  const bool parity2 = get(parities, 2);
  BOOST_CHECK_NE(parity0, parity2);
  BOOST_CHECK_EQUAL(parity2, get(parities, 3));
  BOOST_CHECK_EQUAL(parity2, get(parities, 6));
  BOOST_CHECK_EQUAL(parity2, get(parities, 7));
}

BOOST_AUTO_TEST_CASE(test1)
{
  { // if only one vertex, can't run `boost::stoer_wagner_min_cut`
    undirected_graph g;
    add_vertex(g);
    
    BOOST_CHECK_THROW(boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g)), boost::bad_graph);
  }{ // three vertices with one multi-edge
    typedef boost::graph_traits<undirected_graph>::vertex_descriptor vertex_descriptor;
    
    edge_t edges[] = {{0, 1}, {1, 2}, {1, 2}, {2, 0}};
    weight_type ws[] = {3, 1, 1, 1};
    undirected_graph g(edges, edges + 4, ws, 3, 4);
    
    weight_map_type weights = get(boost::edge_weight, g);
    std::map<int, bool> parity;
    boost::associative_property_map<std::map<int, bool> > parities(parity);
    std::map<vertex_descriptor, vertex_descriptor> assignment;
    boost::associative_property_map<std::map<vertex_descriptor, vertex_descriptor> > assignments(assignment);
    int w = boost::stoer_wagner_min_cut(g, weights, boost::parity_map(parities).vertex_assignment_map(assignments));
    BOOST_CHECK_EQUAL(w, 3);
    const bool parity2 = get(parities, 2),
      parity0 = get(parities, 0);
    BOOST_CHECK_NE(parity2, parity0);
    BOOST_CHECK_EQUAL(parity0, get(parities, 1));
  }
}

// example by Daniel Trebbien
BOOST_AUTO_TEST_CASE(test2)
{
  edge_t edges[] = {{5, 2}, {0, 6}, {5, 6},
    {3, 1}, {0, 1}, {6, 3}, {4, 6}, {2, 4}, {5, 3}};
  weight_type ws[] = {1, 3, 4, 6, 4, 1, 2, 5, 2};
  undirected_graph g(edges, edges + 9, ws, 7, 9);
  
  std::map<int, bool> parity;
  boost::associative_property_map<std::map<int, bool> > parities(parity);
  int w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::parity_map(parities));
  BOOST_CHECK_EQUAL(w, 3);
  const bool parity2 = get(parities, 2);
  BOOST_CHECK_EQUAL(parity2, get(parities, 4));
  const bool parity5 = get(parities, 5);
  BOOST_CHECK_NE(parity2, parity5);
  BOOST_CHECK_EQUAL(parity5, get(parities, 3));
  BOOST_CHECK_EQUAL(parity5, get(parities, 6));
  BOOST_CHECK_EQUAL(parity5, get(parities, 1));
  BOOST_CHECK_EQUAL(parity5, get(parities, 0));
}

// example by Daniel Trebbien
BOOST_AUTO_TEST_CASE(test3)
{
  edge_t edges[] = {{3, 4}, {3, 6}, {3, 5}, {0, 4}, {0, 1}, {0, 6}, {0, 7},
    {0, 5}, {0, 2}, {4, 1}, {1, 6}, {1, 5}, {6, 7}, {7, 5}, {5, 2}, {3, 4}};
  weight_type ws[] = {0, 3, 1, 3, 1, 2, 6, 1, 8, 1, 1, 80, 2, 1, 1, 4};
  undirected_graph g(edges, edges + 16, ws, 8, 16);
  
  weight_map_type weights = get(boost::edge_weight, g);
  std::map<int, bool> parity;
  boost::associative_property_map<std::map<int, bool> > parities(parity);
  int w = boost::stoer_wagner_min_cut(g, weights, boost::parity_map(parities));
  BOOST_CHECK_EQUAL(w, 7);
  const bool parity1 = get(parities, 1);
  BOOST_CHECK_EQUAL(parity1, get(parities, 5));
  const bool parity0 = get(parities, 0);
  BOOST_CHECK_NE(parity1, parity0);
  BOOST_CHECK_EQUAL(parity0, get(parities, 2));
  BOOST_CHECK_EQUAL(parity0, get(parities, 3));
  BOOST_CHECK_EQUAL(parity0, get(parities, 4));
  BOOST_CHECK_EQUAL(parity0, get(parities, 6));
  BOOST_CHECK_EQUAL(parity0, get(parities, 7));
}

BOOST_AUTO_TEST_CASE(test4)
{
  typedef boost::graph_traits<undirected_unweighted_graph>::vertex_descriptor vertex_descriptor;
  typedef boost::graph_traits<undirected_unweighted_graph>::edge_descriptor edge_descriptor;
  
  edge_t edges[] = {{0, 1}, {1, 2}, {2, 3},
    {0, 4}, {1, 4}, {1, 5}, {2, 6}, {3, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7},
    {0, 4}, {6, 7}};
  undirected_unweighted_graph g(edges, edges + 14, 8);
  
  std::map<vertex_descriptor, bool> parity;
  boost::associative_property_map<std::map<vertex_descriptor, bool> > parities(parity);
  std::map<vertex_descriptor, vertex_descriptor> assignment;
  boost::associative_property_map<std::map<vertex_descriptor, vertex_descriptor> > assignments(assignment);
  int w = boost::stoer_wagner_min_cut(g, boost::make_constant_property<edge_descriptor>(weight_type(1)), boost::vertex_assignment_map(assignments).parity_map(parities));
  BOOST_CHECK_EQUAL(w, 2);
  const bool parity0 = get(parities, 0);
  BOOST_CHECK_EQUAL(parity0, get(parities, 1));
  BOOST_CHECK_EQUAL(parity0, get(parities, 4));
  BOOST_CHECK_EQUAL(parity0, get(parities, 5));
  const bool parity2 = get(parities, 2);
  BOOST_CHECK_NE(parity0, parity2);
  BOOST_CHECK_EQUAL(parity2, get(parities, 3));
  BOOST_CHECK_EQUAL(parity2, get(parities, 6));
  BOOST_CHECK_EQUAL(parity2, get(parities, 7));
}

// The input for the `test_prgen` family of tests comes from a program, named
// `prgen`, that comes with a package of min-cut solvers by Chandra Chekuri,
// Andrew Goldberg, David Karger, Matthew Levine, and Cliff Stein. `prgen` was
// used to generate input graphs and the solvers were used to verify the return
// value of `boost::stoer_wagner_min_cut` on the input graphs.
//
// http://www.columbia.edu/~cs2035/code.html
//
// Note that it is somewhat more difficult to verify the parities because
// "`prgen` graphs" often have several min-cuts. This is why only the cut
// weight of the min-cut is verified.

// 3 min-cuts
BOOST_AUTO_TEST_CASE(test_prgen_20_70_2)
{
  typedef boost::graph_traits<undirected_graph>::vertex_descriptor vertex_descriptor;
  
  std::ifstream ifs((test_dir + "/prgen_input_graphs/prgen_20_70_2.net").c_str());
  undirected_graph g;
  boost::read_dimacs_min_cut(g, get(boost::edge_weight, g), boost::dummy_property_map(), ifs);
  
  std::map<vertex_descriptor, std::size_t> component;
  boost::associative_property_map<std::map<vertex_descriptor, std::size_t> > components(component);
  BOOST_CHECK_EQUAL(boost::connected_components(g, components), 1U); // verify the connectedness assumption
  
  typedef boost::shared_array_property_map<weight_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> distances_type;
  distances_type distances = boost::make_shared_array_property_map(num_vertices(g), weight_type(0), get(boost::vertex_index, g));
  typedef std::vector<vertex_descriptor>::size_type index_in_heap_type;
  typedef boost::shared_array_property_map<index_in_heap_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> indicesInHeap_type;
  indicesInHeap_type indicesInHeap = boost::make_shared_array_property_map(num_vertices(g), index_in_heap_type(-1), get(boost::vertex_index, g));
  boost::d_ary_heap_indirect<vertex_descriptor, 22, indicesInHeap_type, distances_type, std::greater<weight_type> > pq(distances, indicesInHeap);
  
  int w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g), boost::max_priority_queue(pq));
  BOOST_CHECK_EQUAL(w, 3407);
}

// 7 min-cuts
BOOST_AUTO_TEST_CASE(test_prgen_50_40_2)
{
  typedef boost::graph_traits<undirected_graph>::vertex_descriptor vertex_descriptor;
  
  std::ifstream ifs((test_dir + "/prgen_input_graphs/prgen_50_40_2.net").c_str());
  undirected_graph g;
  boost::read_dimacs_min_cut(g, get(boost::edge_weight, g), boost::dummy_property_map(), ifs);
  
  std::map<vertex_descriptor, std::size_t> component;
  boost::associative_property_map<std::map<vertex_descriptor, std::size_t> > components(component);
  BOOST_CHECK_EQUAL(boost::connected_components(g, components), 1U); // verify the connectedness assumption
  
  int w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g));
  BOOST_CHECK_EQUAL(w, 10056);
}

// 6 min-cuts
BOOST_AUTO_TEST_CASE(test_prgen_50_70_2)
{
  typedef boost::graph_traits<undirected_graph>::vertex_descriptor vertex_descriptor;
  
  std::ifstream ifs((test_dir + "/prgen_input_graphs/prgen_50_70_2.net").c_str());
  undirected_graph g;
  boost::read_dimacs_min_cut(g, get(boost::edge_weight, g), boost::dummy_property_map(), ifs);
  
  std::map<vertex_descriptor, std::size_t> component;
  boost::associative_property_map<std::map<vertex_descriptor, std::size_t> > components(component);
  BOOST_CHECK_EQUAL(boost::connected_components(g, components), 1U); // verify the connectedness assumption
  
  int w = boost::stoer_wagner_min_cut(g, get(boost::edge_weight, g));
  BOOST_CHECK_EQUAL(w, 21755);
}
