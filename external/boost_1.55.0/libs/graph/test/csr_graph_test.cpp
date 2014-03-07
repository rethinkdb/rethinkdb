// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Douglas Gregor
//           Andrew Lumsdaine

// The libstdc++ debug mode makes this test run for hours...
#ifdef _GLIBCXX_DEBUG
#  undef _GLIBCXX_DEBUG
#endif

// Test for the compressed sparse row graph type
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/concept_check.hpp> // for ignore_unused_variable_warning
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <boost/lexical_cast.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/limits.hpp>
#include <string>
#include <boost/graph/iteration_macros.hpp>
#include <boost/test/minimal.hpp>

// Algorithms to test against
#include <boost/graph/betweenness_centrality.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>

typedef boost::adjacency_list<> GraphT;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, GraphT> ERGen;

struct VertexData
{
  int index;
};

struct EdgeData
{
  int index_e;
};

typedef boost::compressed_sparse_row_graph<boost::directedS, VertexData, EdgeData>
  CSRGraphT;

typedef boost::compressed_sparse_row_graph<boost::bidirectionalS, VertexData, EdgeData>
  BidirCSRGraphT;

template <class G1, class VI1, class G2, class VI2, class IsomorphismMap>
void assert_graphs_equal(const G1& g1, const VI1& vi1,
                         const G2& g2, const VI2& vi2,
                         const IsomorphismMap& iso) {
  using boost::out_degree;

  BOOST_CHECK (num_vertices(g1) == num_vertices(g2));
  BOOST_CHECK (num_edges(g1) == num_edges(g2));

  typedef typename boost::graph_traits<G1>::vertex_iterator vertiter1;
  {
    vertiter1 i, iend;
    for (boost::tie(i, iend) = vertices(g1); i != iend; ++i) {
      typename boost::graph_traits<G1>::vertex_descriptor v1 = *i;
      typename boost::graph_traits<G2>::vertex_descriptor v2 = iso[v1];

      BOOST_CHECK (vi1[v1] == vi2[v2]);

      BOOST_CHECK (out_degree(v1, g1) == out_degree(v2, g2));
      std::vector<std::size_t> edges1(out_degree(v1, g1));
      typename boost::graph_traits<G1>::out_edge_iterator oe1, oe1end;
      for (boost::tie(oe1, oe1end) = out_edges(v1, g1); oe1 != oe1end; ++oe1) {
        BOOST_CHECK (source(*oe1, g1) == v1);
        edges1.push_back(vi1[target(*oe1, g1)]);
      }
      std::vector<std::size_t> edges2(out_degree(v2, g2));
      typename boost::graph_traits<G2>::out_edge_iterator oe2, oe2end;
      for (boost::tie(oe2, oe2end) = out_edges(v2, g2); oe2 != oe2end; ++oe2) {
        BOOST_CHECK (source(*oe2, g2) == v2);
        edges2.push_back(vi2[target(*oe2, g2)]);
      }

      std::sort(edges1.begin(), edges1.end());
      std::sort(edges2.begin(), edges2.end());
      if (edges1 != edges2) {
        std::cerr << "For vertex " << v1 << std::endl;
        std::cerr << "edges1:";
        for (size_t i = 0; i < edges1.size(); ++i) std::cerr << " " << edges1[i];
        std::cerr << std::endl;
        std::cerr << "edges2:";
        for (size_t i = 0; i < edges2.size(); ++i) std::cerr << " " << edges2[i];
        std::cerr << std::endl;
      }
      BOOST_CHECK (edges1 == edges2);
    }
  }

  {
    std::vector<std::pair<std::size_t, std::size_t> > all_edges1;
    std::vector<std::pair<std::size_t, std::size_t> > all_edges2;
    typename boost::graph_traits<G1>::edge_iterator ei1, ei1end;
    for (boost::tie(ei1, ei1end) = edges(g1); ei1 != ei1end; ++ei1)
      all_edges1.push_back(std::make_pair(vi1[source(*ei1, g1)],
                                          vi1[target(*ei1, g1)]));
    typename boost::graph_traits<G2>::edge_iterator ei2, ei2end;
    for (boost::tie(ei2, ei2end) = edges(g2); ei2 != ei2end; ++ei2)
      all_edges2.push_back(std::make_pair(vi2[source(*ei2, g2)],
                                          vi2[target(*ei2, g2)]));
    std::sort(all_edges1.begin(), all_edges1.end());
    std::sort(all_edges2.begin(), all_edges2.end());
    BOOST_CHECK (all_edges1 == all_edges2);
  }
}

template <typename Structure>
void check_consistency_one(const Structure& g) {
  // Do a bunch of tests on the graph internal data
  // Check that m_rowstart entries are valid, and that entries after
  // m_last_source + 1 are all zero
  BOOST_CHECK(g.m_rowstart[0] == 0);
  for (size_t i = 0;
       i < g.m_rowstart.size() - 1;
       ++i) {
    BOOST_CHECK(g.m_rowstart[i + 1] >= g.m_rowstart[i]);
    BOOST_CHECK(g.m_rowstart[i + 1] <= g.m_rowstart.back());
  }
  // Check that m_column entries are within range
  for (size_t i = 0; i < g.m_rowstart.back(); ++i) {
    BOOST_CHECK(g.m_column[i] < g.m_rowstart.size() - 1);
  }
}

template <typename Graph>
void check_consistency_dispatch(const Graph& g,
                                boost::incidence_graph_tag) {
  check_consistency_one(g.m_forward);
}

template <class G>
void assert_bidir_equal_in_both_dirs(const G& g) {
  BOOST_CHECK (g.m_forward.m_rowstart.size() == g.m_backward.m_rowstart.size());
  BOOST_CHECK (g.m_forward.m_column.size() == g.m_backward.m_column.size());
  typedef typename boost::graph_traits<G>::vertex_descriptor Vertex;
  typedef typename boost::graph_traits<G>::edges_size_type EdgeIndex;
  std::vector<boost::tuple<EdgeIndex, Vertex, Vertex> > edges_forward, edges_backward;
  for (Vertex i = 0; i < g.m_forward.m_rowstart.size() - 1; ++i) {
    for (EdgeIndex j = g.m_forward.m_rowstart[i];
         j < g.m_forward.m_rowstart[i + 1]; ++j) {
      edges_forward.push_back(boost::make_tuple(j, i, g.m_forward.m_column[j]));
    }
  }
  for (Vertex i = 0; i < g.m_backward.m_rowstart.size() - 1; ++i) {
    for (EdgeIndex j = g.m_backward.m_rowstart[i];
         j < g.m_backward.m_rowstart[i + 1]; ++j) {
      edges_backward.push_back(boost::make_tuple(g.m_backward.m_edge_properties[j], g.m_backward.m_column[j], i));
    }
  }
  std::sort(edges_forward.begin(), edges_forward.end());
  std::sort(edges_backward.begin(), edges_backward.end());
  BOOST_CHECK (edges_forward == edges_backward);
}

template <typename Graph>
void check_consistency_dispatch(const Graph& g,
                                boost::bidirectional_graph_tag) {
  check_consistency_one(g.m_forward);
  check_consistency_one(g.m_backward);
  assert_bidir_equal_in_both_dirs(g);
}

template <typename Graph>
void check_consistency(const Graph& g) {
  check_consistency_dispatch(g, typename boost::graph_traits<Graph>::traversal_category());
}

template<typename OrigGraph>
void graph_test(const OrigGraph& g)
{
  // Check copying of a graph
  CSRGraphT g2(g);
  check_consistency(g2);
  BOOST_CHECK((std::size_t)std::distance(edges(g2).first, edges(g2).second)
              == num_edges(g2));
  assert_graphs_equal(g, boost::identity_property_map(),
                      g2, boost::identity_property_map(),
                      boost::identity_property_map());

  // Check constructing a graph from iterators
  CSRGraphT g3(boost::edges_are_sorted,
              boost::make_transform_iterator(edges(g2).first,
                                              boost::detail::make_edge_to_index_pair(g2)),
               boost::make_transform_iterator(edges(g2).second,
                                              boost::detail::make_edge_to_index_pair(g2)),
               num_vertices(g));
  check_consistency(g3);
  BOOST_CHECK((std::size_t)std::distance(edges(g3).first, edges(g3).second)
              == num_edges(g3));
  assert_graphs_equal(g2, boost::identity_property_map(),
                      g3, boost::identity_property_map(),
                      boost::identity_property_map());

  // Check constructing a graph using in-place modification of vectors
  {
    std::vector<std::size_t> sources(num_edges(g2));
    std::vector<std::size_t> targets(num_edges(g2));
    std::size_t idx = 0;
    // Edges actually sorted
    BGL_FORALL_EDGES(e, g2, CSRGraphT) {
      sources[idx] = source(e, g2);
      targets[idx] = target(e, g2);
      ++idx;
    }
    CSRGraphT g3a(boost::construct_inplace_from_sources_and_targets,
                  sources,
                  targets,
                  num_vertices(g2));
    check_consistency(g3a);
    assert_graphs_equal(g2, boost::identity_property_map(),
                        g3a, boost::identity_property_map(),
                        boost::identity_property_map());
  }
  {
    std::vector<std::size_t> sources(num_edges(g2));
    std::vector<std::size_t> targets(num_edges(g2));
    std::size_t idx = 0;
    // Edges reverse-sorted
    BGL_FORALL_EDGES(e, g2, CSRGraphT) {
      sources[num_edges(g2) - 1 - idx] = source(e, g2);
      targets[num_edges(g2) - 1 - idx] = target(e, g2);
      ++idx;
    }
    CSRGraphT g3a(boost::construct_inplace_from_sources_and_targets,
                  sources,
                  targets,
                  num_vertices(g2));
    check_consistency(g3a);
    assert_graphs_equal(g2, boost::identity_property_map(),
                        g3a, boost::identity_property_map(),
                        boost::identity_property_map());
  }
  {
    std::vector<std::size_t> sources(num_edges(g2));
    std::vector<std::size_t> targets(num_edges(g2));
    std::size_t idx = 0;
    // Edges scrambled using Fisher-Yates shuffle (Durstenfeld variant) from
    // Wikipedia
    BGL_FORALL_EDGES(e, g2, CSRGraphT) {
      sources[idx] = source(e, g2);
      targets[idx] = target(e, g2);
      ++idx;
    }
    boost::minstd_rand gen(1);
    if (num_edges(g) != 0) {
      for (std::size_t i = num_edges(g) - 1; i > 0; --i) {
        std::size_t scrambled = boost::uniform_int<>(0, i)(gen);
        if (scrambled == i) continue;
        using std::swap;
        swap(sources[i], sources[scrambled]);
        swap(targets[i], targets[scrambled]);
      }
    }
    CSRGraphT g3a(boost::construct_inplace_from_sources_and_targets,
                  sources,
                  targets,
                  num_vertices(g2));
    check_consistency(g3a);
    assert_graphs_equal(g2, boost::identity_property_map(),
                        g3a, boost::identity_property_map(),
                        boost::identity_property_map());
  }

  CSRGraphT::edge_iterator ei, ei_end;

  // Check edge_from_index (and implicitly the edge_index property map) for
  // each edge in g2
  std::size_t last_src = 0;
  for (boost::tie(ei, ei_end) = edges(g2); ei != ei_end; ++ei) {
    BOOST_CHECK(edge_from_index(get(boost::edge_index, g2, *ei), g2) == *ei);
    std::size_t src = get(boost::vertex_index, g2, source(*ei, g2));
    (void)(std::size_t)get(boost::vertex_index, g2, target(*ei, g2));
    BOOST_CHECK(src >= last_src);
    last_src = src;
  }

  // Check out edge iteration and vertex iteration for sortedness
  CSRGraphT::vertex_iterator vi, vi_end;
  std::size_t last_vertex = 0;
  bool first_iter = true;
  for (boost::tie(vi, vi_end) = vertices(g2); vi != vi_end; ++vi) {
    std::size_t v = get(boost::vertex_index, g2, *vi);
    BOOST_CHECK(first_iter || v > last_vertex);
    last_vertex = v;
    first_iter = false;

    CSRGraphT::out_edge_iterator oei, oei_end;
    for (boost::tie(oei, oei_end) = out_edges(*vi, g2); oei != oei_end; ++oei) {
      BOOST_CHECK(source(*oei, g2) == *vi);
    }

    // Find a vertex for testing
    CSRGraphT::vertex_descriptor test_vertex = vertex(num_vertices(g2) / 2, g2);
    int edge_count = 0;
    CSRGraphT::out_edge_iterator oei2, oei2_end;
    for (boost::tie(oei2, oei_end) = out_edges(*vi, g2); oei2 != oei_end; ++oei2) {
      if (target(*oei2, g2) == test_vertex)
        ++edge_count;
    }
  }

  // Run brandes_betweenness_centrality, which touches on a whole lot
  // of things, including VertexListGraph and IncidenceGraph
  using namespace boost;
  std::vector<double> vertex_centralities(num_vertices(g3));
  std::vector<double> edge_centralities(num_edges(g3));
  brandes_betweenness_centrality
    (g3,
     make_iterator_property_map(vertex_centralities.begin(),
                                get(boost::vertex_index, g3)),
     make_iterator_property_map(edge_centralities.begin(),
                                get(boost::edge_index, g3)));
    // Extra qualifications for aCC

  // Invert the edge centralities and use these as weights to
  // Kruskal's MST algorithm, which will test the EdgeListGraph
  // capabilities.
  double max_val = (std::numeric_limits<double>::max)();
  for (std::size_t i = 0; i < num_edges(g3); ++i)
    edge_centralities[i] =
      edge_centralities[i] == 0.0? max_val : 1.0 / edge_centralities[i];

  typedef boost::graph_traits<CSRGraphT>::edge_descriptor edge_descriptor;
  std::vector<edge_descriptor> mst_edges;
  mst_edges.reserve(num_vertices(g3));
  kruskal_minimum_spanning_tree
    (g3, std::back_inserter(mst_edges),
     weight_map(make_iterator_property_map(edge_centralities.begin(),
                                           get(boost::edge_index, g3))));
}

void graph_test(int nnodes, double density, int seed)
{
  boost::minstd_rand gen(seed);
  std::cout << "Testing " << nnodes << " density " << density << std::endl;
  GraphT g(ERGen(gen, nnodes, density), ERGen(), nnodes);
  graph_test(g);
}

void test_graph_properties()
{
  using namespace boost;

  typedef compressed_sparse_row_graph<directedS,
                                      no_property,
                                      no_property,
                                      property<graph_name_t, std::string> >
    CSRGraphT;

  CSRGraphT g;
  BOOST_CHECK(get_property(g, graph_name) == "");
  set_property(g, graph_name, "beep");
  BOOST_CHECK(get_property(g, graph_name) == "beep");
}

struct Vertex
{
  double centrality;
};

struct Edge
{
  Edge(double weight) : weight(weight), centrality(0.0) { }

  double weight;
  double centrality;
};

void test_vertex_and_edge_properties()
{
  using namespace boost;
  typedef compressed_sparse_row_graph<directedS, Vertex, Edge>
    CSRGraphWithPropsT;

  typedef std::pair<int, int> E;
  E edges_init[6] = { E(0, 1), E(0, 3), E(1, 2), E(3, 1), E(3, 4), E(4, 2) };
  double weights[6] = { 1.0, 1.0, 0.5, 1.0, 1.0, 0.5 };
  double centrality[5] = { 0.0, 1.5, 0.0, 1.0, 0.5 };

  CSRGraphWithPropsT g(boost::edges_are_sorted, &edges_init[0], &edges_init[0] + 6, &weights[0], 5, 6);
  brandes_betweenness_centrality
    (g,
     centrality_map(get(&Vertex::centrality, g)).
     weight_map(get(&Edge::weight, g)).
     edge_centrality_map(get(&Edge::centrality, g)));

  BGL_FORALL_VERTICES(v, g, CSRGraphWithPropsT)
    BOOST_CHECK(g[v].centrality == centrality[v]);
}

int test_main(int argc, char* argv[])
{
  // Optionally accept a seed value
  int seed = int(std::time(0));
  if (argc > 1) seed = boost::lexical_cast<int>(argv[1]);

  std::cout << "Seed = " << seed << std::endl;
  {
    std::cout << "Testing empty graph" << std::endl;
    CSRGraphT g;
    graph_test(g);
  }
  //  graph_test(1000, 0.05, seed);
  //  graph_test(1000, 0.0, seed);
  //  graph_test(1000, 0.1, seed);
  graph_test(1000, 0.001, seed);
  graph_test(1000, 0.0005, seed);

  test_graph_properties();
  test_vertex_and_edge_properties();

  {
    std::cout << "Testing CSR graph built from unsorted edges" << std::endl;
    std::pair<int, int> unsorted_edges[] = {std::make_pair(5, 0), std::make_pair(3, 2), std::make_pair(4, 1), std::make_pair(4, 0), std::make_pair(0, 2), std::make_pair(5, 2)};
    CSRGraphT g(boost::edges_are_unsorted, unsorted_edges, unsorted_edges + sizeof(unsorted_edges) / sizeof(*unsorted_edges), 6);

    // Test vertex and edge bundle access
    boost::ignore_unused_variable_warning(
      (VertexData&)get(get(boost::vertex_bundle, g), vertex(0, g)));
    boost::ignore_unused_variable_warning(
      (const VertexData&)get(get(boost::vertex_bundle, (const CSRGraphT&)g), vertex(0, g)));
    boost::ignore_unused_variable_warning(
      (VertexData&)get(boost::vertex_bundle, g, vertex(0, g)));
    boost::ignore_unused_variable_warning(
      (const VertexData&)get(boost::vertex_bundle, (const CSRGraphT&)g, vertex(0, g)));
    put(boost::vertex_bundle, g, vertex(0, g), VertexData());
    boost::ignore_unused_variable_warning(
      (EdgeData&)get(get(boost::edge_bundle, g), *edges(g).first));
    boost::ignore_unused_variable_warning(
      (const EdgeData&)get(get(boost::edge_bundle, (const CSRGraphT&)g), *edges(g).first));
    boost::ignore_unused_variable_warning(
      (EdgeData&)get(boost::edge_bundle, g, *edges(g).first));
    boost::ignore_unused_variable_warning(
      (const EdgeData&)get(boost::edge_bundle, (const CSRGraphT&)g, *edges(g).first));
    put(boost::edge_bundle, g, *edges(g).first, EdgeData());

    CSRGraphT g2(boost::edges_are_unsorted_multi_pass, unsorted_edges, unsorted_edges + sizeof(unsorted_edges) / sizeof(*unsorted_edges), 6);
    graph_test(g);
    graph_test(g2);
    assert_graphs_equal(g, boost::identity_property_map(),
                        g2, boost::identity_property_map(),
                        boost::identity_property_map());
    std::cout << "Testing bidir CSR graph built from unsorted edges" << std::endl;
    BidirCSRGraphT g2b(boost::edges_are_unsorted_multi_pass, unsorted_edges, unsorted_edges + sizeof(unsorted_edges) / sizeof(*unsorted_edges), 6);
    graph_test(g2b);
    assert_graphs_equal(g, boost::identity_property_map(),
                        g2b, boost::identity_property_map(),
                        boost::identity_property_map());
    // Check in edge access
    typedef boost::graph_traits<BidirCSRGraphT>::in_edge_iterator in_edge_iterator;
    std::pair<in_edge_iterator, in_edge_iterator> ie(in_edges(vertex(0, g2b), g2b));

    std::cout << "Testing CSR graph built using add_edges" << std::endl;
    // Test building a graph using add_edges on unsorted lists
    CSRGraphT g3(boost::edges_are_unsorted, unsorted_edges, unsorted_edges, 6); // Empty range
    add_edges(unsorted_edges, unsorted_edges + 3, g3);
    EdgeData edge_data[3];
    add_edges(unsorted_edges + 3, unsorted_edges + 6, edge_data, edge_data + 3, g3);
    graph_test(g3);
    assert_graphs_equal(g, boost::identity_property_map(),
                        g3, boost::identity_property_map(),
                        boost::identity_property_map());
  }

  return 0;
}
