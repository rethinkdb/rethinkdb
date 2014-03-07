// (C) Copyright Jeremy Siek 2004
// (C) Copyright Andrew Sutton 2009
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <set>

#include <boost/random/mersenne_twister.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/graph_test.hpp>
#include <boost/graph/iteration_macros.hpp>

using namespace boost;

struct node
{
    int color;
};

struct arc
{
    int weight;
};
typedef property<edge_index_t, std::size_t, arc> arc_prop;

typedef adjacency_list<
    vecS, vecS, bidirectionalS,
    node, arc_prop
> Graph;

typedef subgraph<Graph> Subgraph;
typedef graph_traits<Subgraph>::vertex_descriptor Vertex;
typedef graph_traits<Subgraph>::edge_descriptor Edge;
typedef graph_traits<Subgraph>::vertex_iterator VertexIter;
typedef graph_traits<Subgraph>::edge_iterator EdgeIter;

int test_main(int, char*[])
{
  mt19937 gen;
  for (int t = 0; t < 100; t += 5) {
    Subgraph g;
    int N = t + 2;
    std::vector<Vertex> vertex_set;
    std::vector< std::pair<Vertex, Vertex> > edge_set;

    generate_random_graph(g, N, N * 2, gen,
                          std::back_inserter(vertex_set),
                          std::back_inserter(edge_set));
    graph_test< Subgraph > gt;

    gt.test_incidence_graph(vertex_set, edge_set, g);
    gt.test_bidirectional_graph(vertex_set, edge_set, g);
    gt.test_adjacency_graph(vertex_set, edge_set, g);
    gt.test_vertex_list_graph(vertex_set, g);
    gt.test_edge_list_graph(vertex_set, edge_set, g);
    gt.test_adjacency_matrix(vertex_set, edge_set, g);

    std::vector<Vertex> sub_vertex_set;
    std::vector<Vertex> sub_global_map;
    std::vector<Vertex> global_sub_map(num_vertices(g));
    std::vector< std::pair<Vertex, Vertex> > sub_edge_set;

    Subgraph& g_s = g.create_subgraph();

    const std::set<Vertex>::size_type Nsub = N/2;

    // Collect a set of random vertices to put in the subgraph
    std::set<Vertex> verts;
    while (verts.size() < Nsub)
      verts.insert(random_vertex(g, gen));

    for (std::set<Vertex>::iterator it = verts.begin();
        it != verts.end(); ++it) {
      Vertex v_global = *it;
      Vertex v = add_vertex(v_global, g_s);
      sub_vertex_set.push_back(v);
      sub_global_map.push_back(v_global);
      global_sub_map[v_global] = v;
    }

    // compute induced edges
    BGL_FORALL_EDGES(e, g, Subgraph)
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

    // Test property maps for vertices.
    typedef property_map<Subgraph, int node::*>::type ColorMap;
    ColorMap colors = get(&node::color, g_s);
    for(std::pair<VertexIter, VertexIter> r = vertices(g_s); r.first != r.second; ++r.first)
        colors[*r.first] = 0;

    // Test property maps for edges.
    typedef property_map<Subgraph, int arc::*>::type WeightMap;
    WeightMap weights = get(&arc::weight, g_s);
    for(std::pair<EdgeIter, EdgeIter> r = edges(g_s); r.first != r.second; ++r.first) {
        weights[*r.first] = 12;
    }

    // A regression test: the copy constructor of subgraph did not
    // copy one of the members, so local_edge->global_edge mapping
    // was broken.
    {
        Subgraph g;
        graph_traits<Graph>::vertex_descriptor v1, v2;
        v1 = add_vertex(g);
        v2 = add_vertex(g);
        add_edge(v1, v2, g);

        Subgraph sub = g.create_subgraph(vertices(g).first, vertices(g).second);

        graph_traits<Graph>::edge_iterator ei, ee;
        for (boost::tie(ei, ee) = edges(sub); ei != ee; ++ei) {
            // This used to segfault.
            get(&arc::weight, sub, *ei);
        }
    }

  }
  return 0;
}
