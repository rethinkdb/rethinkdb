// (C) Copyright 2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef TEST_PROPERTIES_HPP
#define TEST_PROPERTIES_HPP

#include <boost/concept/assert.hpp>

template<typename T> T const& as_const(T& x) { return x; }
template<typename T> void ignore(T const&) { }

template<typename Graph>
void test_graph_bundle(Graph& g, boost::mpl::true_) {
  using namespace boost;
  std::cout << "...test_graph_bundle\n";

  GraphBundle& b1 = g[graph_bundle];
  GraphBundle& b2 = get_property(g);
  ignore(b1); ignore(b2);

  GraphBundle const& cb1 = as_const(g)[graph_bundle];
  GraphBundle const& cb2 = get_property(g);
  ignore(cb1); ignore(cb2);
}

template<typename Graph>
void test_graph_bundle(Graph& g, boost::mpl::false_)
{ }

/** @name Test Vertex Bundle
 * Exercise the vertex bundle. Note that this is expected to be of type
 * VertexBundle.
 */
//@{
template <typename Graph, typename VertexSet>
void test_vertex_bundle(Graph& g, VertexSet const& verts, boost::mpl::true_) {
  using namespace boost;
  BOOST_CONCEPT_ASSERT((GraphConcept<Graph>));
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  BOOST_CONCEPT_ASSERT((PropertyGraphConcept<Graph, Vertex, vertex_bundle_t>));

    // Test bundling via the graph object on the lollipop vertex.
  Vertex v = verts[5];
  VertexBundle& b = g[v];
  b.value = 10;
  BOOST_ASSERT(g[v].value == 10);

  // Test bundling via the property map.
  typedef typename property_map<Graph, int VertexBundle::*>::type BundleMap;
  BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept<BundleMap, Vertex>));
  BundleMap map = get(&VertexBundle::value, g);
  put(map, v, 5);
  BOOST_ASSERT(get(map, v) == 5);

  typedef typename property_map<Graph, int VertexBundle::*>::const_type ConstBundleMap;
  BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<ConstBundleMap, Vertex>));
  ConstBundleMap cmap = get(&VertexBundle::value, (Graph const&)g);
  BOOST_ASSERT(get(cmap, v) == 5);
}

template <typename Graph, typename VertexSet>
void test_vertex_bundle(Graph&, VertexSet const&, boost::mpl::false_)
{ }
//@}

/** @name Test Edge Bundle
 * Exercise the edge bundle. Note that this is expected to be of type
 * EdgeBundle.
 */
//@{
template <typename Graph, typename VertexSet>
void test_edge_bundle(Graph& g, VertexSet const& verts, boost::mpl::true_) {
    using namespace boost;
    BOOST_CONCEPT_ASSERT((GraphConcept<Graph>));
    typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
    BOOST_CONCEPT_ASSERT((PropertyGraphConcept<Graph, Edge, edge_bundle_t>));

    std::cout << "...test_edge_bundle\n";

    // Test bundling via the graph object on the lollipop edge.
    Edge e = boost::edge(verts[5], verts[3], g).first;
    EdgeBundle& b = g[e];
    b.value = 10;
    BOOST_ASSERT(g[e].value == 10);

    // Test bundling via the property map.
    typedef typename boost::property_map<Graph, int EdgeBundle::*>::type BundleMap;
    BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept<BundleMap, Edge>));
    BundleMap map = get(&EdgeBundle::value, g);
    put(map, e, 5);
    BOOST_ASSERT(get(map, e) == 5);

    typedef typename boost::property_map<Graph, int EdgeBundle::*>::const_type ConstBundleMap;
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<BundleMap, Edge>));
    ConstBundleMap cmap = get(&EdgeBundle::value, (Graph const&)g);
    BOOST_ASSERT(get(cmap, e) == 5);
}

template <typename Graph, typename VertexSet>
void test_edge_bundle(Graph&, VertexSet const&, boost::mpl::false_)
{ }
//@}

/**
 * Test the properties of a graph. Basically, we expect these to be one of
 * bundled or not. This test could also be expanded to test non-bundled
 * properties. This just bootstraps the tests.
 */
template <typename Graph, typename VertexSet>
void test_properties(Graph& g, VertexSet const& verts) {
  using namespace boost;

  typename has_bundled_graph_property<Graph>::type graph_bundled;
  typename has_bundled_vertex_property<Graph>::type vertex_bundled;
  typename has_bundled_edge_property<Graph>::type edge_bundled;

  test_graph_bundle(g, graph_bundled);
  test_vertex_bundle(g, verts, vertex_bundled);
  test_edge_bundle(g, verts, edge_bundled);
}
//@}

#endif
