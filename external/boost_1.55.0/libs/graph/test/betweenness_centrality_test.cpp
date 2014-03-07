// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/betweenness_centrality.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <vector>
#include <stack>
#include <queue>
#include <boost/property_map/property_map.hpp>
#include <boost/test/minimal.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost;

const double error_tolerance = 0.001;

typedef property<edge_weight_t, double,
                 property<edge_index_t, std::size_t> > EdgeProperties;

struct weighted_edge 
{
  int source, target;
  double weight;
};

template<typename Graph>
void 
run_weighted_test(Graph*, int V, weighted_edge edge_init[], int E,
                  double correct_centrality[])
{
  Graph g(V);
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  std::vector<Vertex> vertices(V);
  {
    vertex_iterator v, v_end;
    int index = 0;
    for (boost::tie(v, v_end) = boost::vertices(g); v != v_end; ++v, ++index) {
      put(vertex_index, g, *v, index);
      vertices[index] = *v;
    }
  }

  std::vector<Edge> edges(E);
  for (int e = 0; e < E; ++e) {
    edges[e] = add_edge(vertices[edge_init[e].source],
                        vertices[edge_init[e].target], 
                        g).first;
    put(edge_weight, g, edges[e], 1.0);
  }

  std::vector<double> centrality(V);
  brandes_betweenness_centrality(
    g,
    centrality_map(
      make_iterator_property_map(centrality.begin(), get(vertex_index, g),
                                 double()))
    .vertex_index_map(get(vertex_index, g)).weight_map(get(edge_weight, g)));


  for (int v = 0; v < V; ++v) {
    BOOST_CHECK(centrality[v] == correct_centrality[v]);
  }
}

struct unweighted_edge 
{
  int source, target;
};

template<typename Graph>
void 
run_unweighted_test(Graph*, int V, unweighted_edge edge_init[], int E,
                    double correct_centrality[],
                    double* correct_edge_centrality = 0)
{
  Graph g(V);
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  std::vector<Vertex> vertices(V);
  {
    vertex_iterator v, v_end;
    int index = 0;
    for (boost::tie(v, v_end) = boost::vertices(g); v != v_end; ++v, ++index) {
      put(vertex_index, g, *v, index);
      vertices[index] = *v;
    }
  }

  std::vector<Edge> edges(E);
  for (int e = 0; e < E; ++e) {
    edges[e] = add_edge(vertices[edge_init[e].source],
                        vertices[edge_init[e].target], 
                        g).first;
    put(edge_weight, g, edges[e], 1.0);
    put(edge_index, g, edges[e], e);
  }

  std::vector<double> centrality(V);
  std::vector<double> edge_centrality1(E);

  brandes_betweenness_centrality(
    g,
    centrality_map(
      make_iterator_property_map(centrality.begin(), get(vertex_index, g),
                                 double()))
    .edge_centrality_map(
       make_iterator_property_map(edge_centrality1.begin(), 
                                  get(edge_index, g), double()))
    .vertex_index_map(get(vertex_index, g)));

  std::vector<double> centrality2(V);
  std::vector<double> edge_centrality2(E);
  brandes_betweenness_centrality(
    g,
    vertex_index_map(get(vertex_index, g)).weight_map(get(edge_weight, g))
    .centrality_map(
       make_iterator_property_map(centrality2.begin(), get(vertex_index, g),
                                  double()))
    .edge_centrality_map(
       make_iterator_property_map(edge_centrality2.begin(), 
                                  get(edge_index, g), double())));

  std::vector<double> edge_centrality3(E);
  brandes_betweenness_centrality(
    g,
    edge_centrality_map(
      make_iterator_property_map(edge_centrality3.begin(), 
                                 get(edge_index, g), double())));

  for (int v = 0; v < V; ++v) {
    BOOST_CHECK(centrality[v] == centrality2[v]);

    double relative_error = 
      correct_centrality[v] == 0.0? centrality[v]
      : (centrality[v] - correct_centrality[v]) / correct_centrality[v];
    if (relative_error < 0) relative_error = -relative_error;
    BOOST_CHECK(relative_error < error_tolerance);
  }  

  for (int e = 0; e < E; ++e) {
    BOOST_CHECK(edge_centrality1[e] == edge_centrality2[e]);
    BOOST_CHECK(edge_centrality1[e] == edge_centrality3[e]);

    if (correct_edge_centrality) {
      double relative_error = 
        correct_edge_centrality[e] == 0.0? edge_centrality1[e]
        : (edge_centrality1[e] - correct_edge_centrality[e]) 
        / correct_edge_centrality[e];
      if (relative_error < 0) relative_error = -relative_error;
      BOOST_CHECK(relative_error < error_tolerance);

      if (relative_error >= error_tolerance) {
        std::cerr << "Edge " << e << " has edge centrality " 
                  << edge_centrality1[e] << ", should be " 
                  << correct_edge_centrality[e] << std::endl;
      }
    }
  }
}

template<typename Graph>
void
run_wheel_test(Graph*, int V)
{
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  Graph g(V);
  Vertex center = *boost::vertices(g).first;

  std::vector<Vertex> vertices(V);
  {
    vertex_iterator v, v_end;
    int index = 0;
    for (boost::tie(v, v_end) = boost::vertices(g); v != v_end; ++v, ++index) {
      put(vertex_index, g, *v, index);
      vertices[index] = *v;
      if (*v != center) {
        Edge e = add_edge(*v, center, g).first;
        put(edge_weight, g, e, 1.0);
      }
    }
  }

  std::vector<double> centrality(V);
  brandes_betweenness_centrality(
    g,
    make_iterator_property_map(centrality.begin(), get(vertex_index, g),
                               double()));

  std::vector<double> centrality2(V);
  brandes_betweenness_centrality(
    g,
    centrality_map(
      make_iterator_property_map(centrality2.begin(), get(vertex_index, g),
                                 double()))
    .vertex_index_map(get(vertex_index, g)).weight_map(get(edge_weight, g)));

  relative_betweenness_centrality(
    g,
    make_iterator_property_map(centrality.begin(), get(vertex_index, g),
                               double()));

  relative_betweenness_centrality(
    g,
    make_iterator_property_map(centrality2.begin(), get(vertex_index, g),
                               double()));

  for (int v = 0; v < V; ++v) {
    BOOST_CHECK(centrality[v] == centrality2[v]);
    BOOST_CHECK((v == 0 && centrality[v] == 1)
               || (v != 0 && centrality[v] == 0));
  } 

  double dominance = 
    central_point_dominance(
      g, 
      make_iterator_property_map(centrality2.begin(), get(vertex_index, g),
                                 double()));
  BOOST_CHECK(dominance == 1.0);
}

template<typename MutableGraph>
void randomly_add_edges(MutableGraph& g, double edge_probability)
{
  typedef typename graph_traits<MutableGraph>::directed_category
    directed_category;
  const bool is_undirected = 
    is_same<directed_category, undirected_tag>::value;

  minstd_rand gen;
  uniform_01<minstd_rand, double> rand_gen(gen);

  typedef typename graph_traits<MutableGraph>::vertex_descriptor vertex;
  typename graph_traits<MutableGraph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    vertex v = *vi;
    typename graph_traits<MutableGraph>::vertex_iterator wi 
      = is_undirected? vi : vertices(g).first;
    while (wi != vi_end) {
      vertex w = *wi++;
      if (v != w) {
        if (rand_gen() < edge_probability) add_edge(v, w, g);
      }
    }
  }
}


template<typename Graph, typename VertexIndexMap, typename CentralityMap>
void 
simple_unweighted_betweenness_centrality(const Graph& g, VertexIndexMap index,
                                         CentralityMap centrality)
{
  typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex;
  typedef typename boost::graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename boost::graph_traits<Graph>::adjacency_iterator adjacency_iterator;
  typedef typename boost::graph_traits<Graph>::vertices_size_type vertices_size_type;
  typedef typename boost::property_traits<CentralityMap>::value_type centrality_type;

  vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    put(centrality, *vi, 0);

  vertex_iterator si, si_end;
  for (boost::tie(si, si_end) = vertices(g); si != si_end; ++si) {
    vertex s = *si;

    // S <-- empty stack
    std::stack<vertex> S;

    // P[w] <-- empty list, w \in V
    typedef std::vector<vertex> Predecessors;
    std::vector<Predecessors> predecessors(num_vertices(g));

    // sigma[t] <-- 0, t \in V
    std::vector<vertices_size_type> sigma(num_vertices(g), 0);

    // sigma[s] <-- 1
    sigma[get(index, s)] = 1;

    // d[t] <-- -1, t \in V
    std::vector<int> d(num_vertices(g), -1);

    // d[s] <-- 0
    d[get(index, s)] = 0;

    // Q <-- empty queue
    std::queue<vertex> Q;

    // enqueue s --> Q
    Q.push(s);

    while (!Q.empty()) {
      // dequeue v <-- Q
      vertex v = Q.front(); Q.pop();

      // push v --> S
      S.push(v);

      adjacency_iterator wi, wi_end;
      for (boost::tie(wi, wi_end) = adjacent_vertices(v, g); wi != wi_end; ++wi) {
        vertex w = *wi;

        // w found for the first time?
        if (d[get(index, w)] < 0) {
          // enqueue w --> Q
          Q.push(w);
          
          // d[w] <-- d[v] + 1
          d[get(index, w)] = d[get(index, v)] + 1;
        }

        // shortest path to w via v?
        if (d[get(index, w)] == d[get(index, v)] + 1) {
          // sigma[w] = sigma[w] + sigma[v]
          sigma[get(index, w)] += sigma[get(index, v)];

          // append v --> P[w]
          predecessors[get(index, w)].push_back(v);
        }
      }
    }

    // delta[v] <-- 0, v \in V
    std::vector<centrality_type> delta(num_vertices(g), 0);

    // S returns vertices in order of non-increasing distance from s
    while (!S.empty()) {
      // pop w <-- S
      vertex w = S.top(); S.pop();

      const Predecessors& w_preds = predecessors[get(index, w)];
      for (typename Predecessors::const_iterator vi = w_preds.begin();
           vi != w_preds.end(); ++vi) {
        vertex v = *vi;
        // delta[v] <-- delta[v] + (sigma[v]/sigma[w])*(1 + delta[w])
        delta[get(index, v)] += 
          ((centrality_type)sigma[get(index, v)]/sigma[get(index, w)])
          * (1 + delta[get(index, w)]);
      }

      if (w != s) {
        // C_B[w] <-- C_B[w] + delta[w]
        centrality[w] += delta[get(index, w)];
      }
    }
  }

  typedef typename graph_traits<Graph>::directed_category directed_category;
  const bool is_undirected = 
    is_same<directed_category, undirected_tag>::value;
  if (is_undirected) {
    vertex_iterator v, v_end;
    for(boost::tie(v, v_end) = vertices(g); v != v_end; ++v) {
      put(centrality, *v, get(centrality, *v) / centrality_type(2));
    }
  }
}

template<typename Graph>
void random_unweighted_test(Graph*, int n)
{
  Graph g(n);

  {
    typename graph_traits<Graph>::vertex_iterator v, v_end;
    int index = 0;
    for (boost::tie(v, v_end) = boost::vertices(g); v != v_end; ++v, ++index) {
      put(vertex_index, g, *v, index);
    }
  }

  randomly_add_edges(g, 0.20);

  std::cout << "Random graph with " << n << " vertices and "
            << num_edges(g) << " edges.\n";

  std::cout << "  Direct translation of Brandes' algorithm...";
  std::vector<double> centrality(n);
  simple_unweighted_betweenness_centrality(g, get(vertex_index, g),
    make_iterator_property_map(centrality.begin(), get(vertex_index, g),
                               double()));
  std::cout << "DONE.\n";

  std::cout << "  Real version, unweighted...";
  std::vector<double> centrality2(n);
  brandes_betweenness_centrality(g, 
     make_iterator_property_map(centrality2.begin(), get(vertex_index, g),
                                double()));
  std::cout << "DONE.\n";

  if (!std::equal(centrality.begin(), centrality.end(),
                  centrality2.begin())) {
    for (std::size_t v = 0; v < centrality.size(); ++v) {
      double relative_error = 
        centrality[v] == 0.0? centrality2[v]
        : (centrality2[v] - centrality[v]) / centrality[v];
      if (relative_error < 0) relative_error = -relative_error;
      BOOST_CHECK(relative_error < error_tolerance);
    }
  }

  std::cout << "  Real version, weighted...";
  std::vector<double> centrality3(n);

  for (typename graph_traits<Graph>::edge_iterator ei = edges(g).first;
       ei != edges(g).second; ++ei)
    put(edge_weight, g, *ei, 1);

  brandes_betweenness_centrality(g, 
    weight_map(get(edge_weight, g))
    .centrality_map(
       make_iterator_property_map(centrality3.begin(), get(vertex_index, g),
                                  double())));
  std::cout << "DONE.\n";

  if (!std::equal(centrality.begin(), centrality.end(),
                  centrality3.begin())) {
    for (std::size_t v = 0; v < centrality.size(); ++v) {
      double relative_error = 
        centrality[v] == 0.0? centrality3[v]
        : (centrality3[v] - centrality[v]) / centrality[v];
      if (relative_error < 0) relative_error = -relative_error;
      BOOST_CHECK(relative_error < error_tolerance);
    }
  }
}

int test_main(int argc, char* argv[])
{
  int random_test_num_vertices = 300;
  if (argc >= 2) random_test_num_vertices = boost::lexical_cast<int>(argv[1]);
  typedef adjacency_list<listS, listS, undirectedS, 
                         property<vertex_index_t, int>, EdgeProperties> 
    Graph;
  typedef adjacency_list<listS, listS, directedS, 
                         property<vertex_index_t, int>, EdgeProperties> 
    Digraph;

  struct unweighted_edge ud_edge_init1[5] = { 
    { 0, 1 },
    { 0, 3 },
    { 1, 2 },
    { 3, 2 },
    { 2, 4 }
  };
  double ud_centrality1[5] = { 0.5, 1.0, 3.5, 1.0, 0.0 };
  run_unweighted_test((Graph*)0, 5, ud_edge_init1, 5, ud_centrality1);

  // Example borrowed from the JUNG test suite
  struct unweighted_edge ud_edge_init2[10] = { 
    { 0, 1 },
    { 0, 6 },
    { 1, 2 },
    { 1, 3 },
    { 2, 4 },
    { 3, 4 },
    { 4, 5 },
    { 5, 8 },
    { 7, 8 },
    { 6, 7 },
  };
  double ud_centrality2[9] = {
    0.2142 * 28, 
    0.2797 * 28,
    0.0892 * 28,
    0.0892 * 28,
    0.2797 * 28,
    0.2142 * 28,
    0.1666 * 28,
    0.1428 * 28,
    0.1666 * 28
  };
  double ud_edge_centrality2[10] = {
    10.66666,
    9.33333,
    6.5,
    6.5,
    6.5,
    6.5,
    10.66666,
    9.33333,
    8.0,
    8.0
  };

  run_unweighted_test((Graph*)0, 9, ud_edge_init2, 10, ud_centrality2,
                      ud_edge_centrality2);

  weighted_edge dw_edge_init1[6] = {
    { 0, 1, 1.0 },
    { 0, 3, 1.0 },
    { 1, 2, 0.5 },
    { 3, 1, 1.0 },
    { 3, 4, 1.0 },
    { 4, 2, 0.5 }
  };
  double dw_centrality1[5] = { 0.0, 1.5, 0.0, 1.0, 0.5 };
  run_weighted_test((Digraph*)0, 5, dw_edge_init1, 6, dw_centrality1);

  run_wheel_test((Graph*)0, 15);

  random_unweighted_test((Graph*)0, random_test_num_vertices);

  return 0;
}

