//  (C) Copyright Jeremy Siek 2004
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <set>

#include <boost/test/minimal.hpp>

#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/graph_test.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "test_graph.hpp"

// UNDER CONSTRUCTION

int test_main(int, char*[])
{
  using namespace boost;
  typedef adjacency_list<vecS, vecS, bidirectionalS,
    property<vertex_color_t, int>,
    property<edge_index_t, std::size_t, property<edge_weight_t, int> >
  > graph_t;
  typedef subgraph<graph_t> subgraph_t;
  typedef graph_traits<subgraph_t>::vertex_descriptor vertex_t;

  mt19937 gen;
  for (int t = 0; t < 100; t += 5) {
    subgraph_t g;
    int N = t + 2;
    std::vector<vertex_t> vertex_set;
    std::vector< std::pair<vertex_t, vertex_t> > edge_set;
    generate_random_graph(g, N, N * 2, gen,
                          std::back_inserter(vertex_set),
                          std::back_inserter(edge_set));

    graph_test< subgraph_t > gt;

    gt.test_incidence_graph(vertex_set, edge_set, g);
    gt.test_bidirectional_graph(vertex_set, edge_set, g);
    gt.test_adjacency_graph(vertex_set, edge_set, g);
    gt.test_vertex_list_graph(vertex_set, g);
    gt.test_edge_list_graph(vertex_set, edge_set, g);
    gt.test_adjacency_matrix(vertex_set, edge_set, g);

    std::vector<vertex_t> sub_vertex_set;
    std::vector<vertex_t> sub_global_map;
    std::vector<vertex_t> global_sub_map(num_vertices(g));
    std::vector< std::pair<vertex_t, vertex_t> > sub_edge_set;

    subgraph_t& g_s = g.create_subgraph();

    const std::set<vertex_t>::size_type Nsub = N/2;

    // Collect a set of random vertices to put in the subgraph
    std::set<vertex_t> verts;
    while (verts.size() < Nsub)
      verts.insert(random_vertex(g, gen));

    for (std::set<vertex_t>::iterator it = verts.begin();
        it != verts.end(); ++it) {
      vertex_t v_global = *it;
      vertex_t v = add_vertex(v_global, g_s);
      sub_vertex_set.push_back(v);
      sub_global_map.push_back(v_global);
      global_sub_map[v_global] = v;
    }

    // compute induced edges
    BGL_FORALL_EDGES(e, g, subgraph_t)
      if (container_contains(sub_global_map, source(e, g))
          && container_contains(sub_global_map, target(e, g)))
        sub_edge_set.push_back(std::make_pair(global_sub_map[source(e, g)],
                                              global_sub_map[target(e, g)]));

    gt.test_incidence_graph(sub_vertex_set, sub_edge_set, g_s);
    gt.test_bidirectional_graph(sub_vertex_set, sub_edge_set, g_s);
    gt.test_adjacency_graph(sub_vertex_set, sub_edge_set, g_s);
    gt.test_vertex_list_graph(sub_vertex_set, g_s);
    gt.test_edge_list_graph(sub_vertex_set, sub_edge_set, g_s);
    gt.test_adjacency_matrix(sub_vertex_set, sub_edge_set, g_s);

    if (num_vertices(g_s) == 0)
      return 0;
    std::vector<int> weights;
    for (unsigned i = 0; i < num_vertices(g_s); ++i)
    weights.push_back(i*2);
    gt.test_vertex_property_graph(weights, vertex_color_t(), g_s);

    // A regression test: the copy constructor of subgraph did not
    // copy one of the members, so local_edge->global_edge mapping
    // was broken.
    {
        subgraph_t g;
        graph_t::vertex_descriptor v1, v2;
        v1 = add_vertex(g);
        v2 = add_vertex(g);
        add_edge(v1, v2, g);

        subgraph_t sub = g.create_subgraph(vertices(g).first, vertices(g).second);

        graph_t::edge_iterator ei, ee;
        for (boost::tie(ei, ee) = edges(sub); ei != ee; ++ei) {
            // This used to segfault.
            get(edge_weight, sub, *ei);
        }
    }

    // Bootstrap the test_graph framework.
    // TODO: Subgraph is fundamentally broken for property types.
    // TODO: Under construction.
    {
        using namespace boost;
        typedef property<edge_index_t, size_t, EdgeBundle> EdgeProp;
        typedef adjacency_list<vecS, vecS, directedS, VertexBundle, EdgeProp> BaseGraph;
        typedef subgraph<BaseGraph> Graph;
        typedef graph_traits<Graph>::vertex_descriptor Vertex;
        Graph g;
        Vertex v = add_vertex(g);

        typedef property_map<Graph, int VertexBundle::*>::type BundleMap;
        BundleMap map = get(&VertexBundle::value, g);
        get(map, v);
//         put(map, v, 5);
//         BOOST_ASSERT(get(map, v) == 5);

//         test_graph(g);
        return 0;
    }
  }
  return 0;
}
