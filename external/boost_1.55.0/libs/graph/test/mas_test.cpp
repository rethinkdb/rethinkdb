//            Copyright Fernando Vilas 2012.
//     Based on stoer_wagner_test.cpp by Daniel Trebbien.
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
#include <boost/graph/maximum_adjacency_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

#include <boost/graph/iteration_macros.hpp>

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

template <typename Graph, typename KeyedUpdatablePriorityQueue>
class mas_edge_connectivity_visitor : public boost::default_mas_visitor {
  public:
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename KeyedUpdatablePriorityQueue::key_type weight_type;
#if 0
    mas_edge_connectivity_visitor(const mas_edge_connectivity_visitor<Graph, KeyedUpdatablePriorityQueue>& r)
      : m_pq(r.m_pq), m_curr(r.m_curr), m_prev(r.m_prev), 
        m_reach_weight(r.m_reach_weight) {
          BOOST_TEST_MESSAGE( "COPY CTOR" );
        }
#endif
    explicit mas_edge_connectivity_visitor(KeyedUpdatablePriorityQueue& pq)
      : m_pq(pq),
        m_curr(new vertex_descriptor(0)), m_prev(new vertex_descriptor(0)),
        m_reach_weight(new weight_type(0)) {
      //    BOOST_TEST_MESSAGE( "CTOR" );
        }

  void clear() {
    *m_curr = 0;
    *m_prev = 0;
    *m_reach_weight = 0;
  }

  //template <typename Vertex> //, typename Graph>
  //void start_vertex(Vertex u, const Graph& g) {
  void start_vertex(vertex_descriptor u, const Graph& g) {
    *m_prev = *m_curr;
    *m_curr = u;
    //BOOST_TEST_MESSAGE( "Initializing Vertex(weight): " << u << "(" << *m_reach_weight << ")" );
    *m_reach_weight = get(m_pq.keys(), u);
  }

  vertex_descriptor curr() const { return *m_curr; }
  vertex_descriptor prev() const { return *m_prev; }
  weight_type reach_weight() const { return *m_reach_weight; }

  private:

    const KeyedUpdatablePriorityQueue& m_pq;
    boost::shared_ptr<vertex_descriptor> m_curr, m_prev;
    boost::shared_ptr<weight_type> m_reach_weight;
};


// the example from Stoer & Wagner (1997)
// Check various implementations of the ArgPack where
// the weights are provided in it, and one case where
// they are not.
BOOST_AUTO_TEST_CASE(test0)
{
  typedef boost::graph_traits<undirected_graph>::vertex_descriptor vertex_descriptor;
  typedef boost::graph_traits<undirected_graph>::edge_descriptor edge_descriptor;

  edge_t edges[] = {{0, 1}, {1, 2}, {2, 3},
    {0, 4}, {1, 4}, {1, 5}, {2, 6}, {3, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}};
  weight_type ws[] = {2, 3, 4, 3, 2, 2, 2, 2, 2, 3, 1, 3};
  undirected_graph g(edges, edges + 12, ws, 8, 12);

  weight_map_type weights = get(boost::edge_weight, g);

  std::map<vertex_descriptor, vertex_descriptor> assignment;
  boost::associative_property_map<std::map<vertex_descriptor, vertex_descriptor> > assignments(assignment);

  typedef boost::shared_array_property_map<weight_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> distances_type;
  distances_type distances = boost::make_shared_array_property_map(num_vertices(g), weight_type(0), get(boost::vertex_index, g));
  typedef std::vector<vertex_descriptor>::size_type index_in_heap_type;
  typedef boost::shared_array_property_map<index_in_heap_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> indicesInHeap_type;
  indicesInHeap_type indicesInHeap = boost::make_shared_array_property_map(num_vertices(g), index_in_heap_type(-1), get(boost::vertex_index, g));
  boost::d_ary_heap_indirect<vertex_descriptor, 22, indicesInHeap_type, distances_type, std::greater<weight_type> > pq(distances, indicesInHeap);

  mas_edge_connectivity_visitor<undirected_graph,boost::d_ary_heap_indirect<vertex_descriptor, 22, indicesInHeap_type, distances_type, std::greater<weight_type> > >  test_vis(pq);

  boost::maximum_adjacency_search(g,
        boost::weight_map(weights).
        visitor(test_vis).
        root_vertex(*vertices(g).first).
        vertex_assignment_map(assignments).
        max_priority_queue(pq));

  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(6));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), 5);

  test_vis.clear();
  boost::maximum_adjacency_search(g,
        boost::weight_map(weights).
        visitor(test_vis).
        root_vertex(*vertices(g).first).
        max_priority_queue(pq));

  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(6));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), 5);

  test_vis.clear();
  boost::maximum_adjacency_search(g,
        boost::weight_map(weights).
        visitor(test_vis).
        max_priority_queue(pq));

  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(6));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), 5);

  boost::maximum_adjacency_search(g,
        boost::weight_map(weights).
        visitor(boost::make_mas_visitor(boost::null_visitor())));

  boost::maximum_adjacency_search(g,
        boost::weight_map(weights));

  boost::maximum_adjacency_search(g,
        boost::root_vertex(*vertices(g).first));

  test_vis.clear();
  boost::maximum_adjacency_search(g,
      boost::weight_map(boost::make_constant_property<edge_descriptor>(weight_type(1))).
      visitor(test_vis).
      max_priority_queue(pq));
  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(3));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), 2);

}

// Check the unweighted case
// with and without providing a weight_map
BOOST_AUTO_TEST_CASE(test1)
{
  typedef boost::graph_traits<undirected_unweighted_graph>::vertex_descriptor vertex_descriptor;
  typedef boost::graph_traits<undirected_unweighted_graph>::edge_descriptor edge_descriptor;

  edge_t edge_list[] = {{0, 1}, {1, 2}, {2, 3},
    {0, 4}, {1, 4}, {1, 5}, {2, 6}, {3, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}};
  undirected_unweighted_graph g(edge_list, edge_list + 12, 8);

  std::map<vertex_descriptor, vertex_descriptor> assignment;
  boost::associative_property_map<std::map<vertex_descriptor, vertex_descriptor> > assignments(assignment);

  typedef unsigned weight_type;
  typedef boost::shared_array_property_map<weight_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> distances_type;
  distances_type distances = boost::make_shared_array_property_map(num_vertices(g), weight_type(0), get(boost::vertex_index, g));
  typedef std::vector<vertex_descriptor>::size_type index_in_heap_type;
  typedef boost::shared_array_property_map<index_in_heap_type, boost::property_map<undirected_graph, boost::vertex_index_t>::const_type> indicesInHeap_type;
  indicesInHeap_type indicesInHeap = boost::make_shared_array_property_map(num_vertices(g), index_in_heap_type(-1), get(boost::vertex_index, g));
  boost::d_ary_heap_indirect<vertex_descriptor, 22, indicesInHeap_type, distances_type, std::greater<weight_type> > pq(distances, indicesInHeap);

  mas_edge_connectivity_visitor<undirected_unweighted_graph,boost::d_ary_heap_indirect<vertex_descriptor, 22, indicesInHeap_type, distances_type, std::greater<weight_type> > >  test_vis(pq);

  boost::maximum_adjacency_search(g, 
         boost::weight_map(boost::make_constant_property<edge_descriptor>(weight_type(1))).visitor(test_vis).max_priority_queue(pq));

  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(3));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), weight_type(2));

  weight_type ws[] = {2, 3, 4, 3, 2, 2, 2, 2, 2, 3, 1, 3};
  std::map<edge_descriptor, weight_type> wm;

  weight_type i = 0;
  BGL_FORALL_EDGES(e, g, undirected_unweighted_graph) {
    wm[e] = ws[i];
    ++i;
  }
  boost::associative_property_map<std::map<edge_descriptor, weight_type> > ws_map(wm);

  boost::maximum_adjacency_search(g, boost::weight_map(ws_map).visitor(test_vis).max_priority_queue(pq));
  BOOST_CHECK_EQUAL(test_vis.curr(), vertex_descriptor(7));
  BOOST_CHECK_EQUAL(test_vis.prev(), vertex_descriptor(6));
  BOOST_CHECK_EQUAL(test_vis.reach_weight(), weight_type(5));

}

#include <boost/graph/iteration_macros_undef.hpp>

