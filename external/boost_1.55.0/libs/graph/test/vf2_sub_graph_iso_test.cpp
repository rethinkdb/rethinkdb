//=======================================================================
// Boost.Graph library vf2_sub_graph_iso test
// Adapted from isomorphism.cpp and mcgregor_subgraphs_test.cpp
//
// Copyright (C) 2012 Flavio De Lorenzi (fdlorenzi@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// Revision History:
//   8 April 2013: Fixed a typo in random_functor. (Flavio De Lorenzi) 

#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <time.h> 
#include <boost/test/minimal.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/random.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/graphviz.hpp>

using namespace boost;


template <typename Generator>
struct random_functor {
  random_functor(Generator& g) : g(g) { }
  std::size_t operator()(std::size_t n) {
    boost::uniform_int<std::size_t> distrib(0, n-1);
    boost::variate_generator<Generator&, boost::uniform_int<std::size_t> >
      x(g, distrib);
    return x();
  }
  Generator& g;
};


template<typename Graph1, typename Graph2>
void randomly_permute_graph(Graph1& g1, const Graph2& g2) {
  BOOST_REQUIRE(num_vertices(g1) <= num_vertices(g2));
  BOOST_REQUIRE(num_edges(g1) == 0);
  
  typedef typename graph_traits<Graph1>::vertex_descriptor vertex1;
  typedef typename graph_traits<Graph2>::vertex_descriptor vertex2;
  typedef typename graph_traits<Graph1>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph2>::edge_iterator edge_iterator;
  
  boost::mt19937 gen;
  random_functor<boost::mt19937> rand_fun(gen);
  
  // Decide new order
  std::vector<vertex2> orig_vertices;
  std::copy(vertices(g2).first, vertices(g2).second, std::back_inserter(orig_vertices));
  std::random_shuffle(orig_vertices.begin(), orig_vertices.end(), rand_fun);
  std::map<vertex2, vertex1> vertex_map;
  
  std::size_t i = 0;
  for (vertex_iterator vi = vertices(g1).first; 
       vi != vertices(g1).second; ++i, ++vi) {
    vertex_map[orig_vertices[i]] = *vi;
    put(vertex_name_t(), g1, *vi, get(vertex_name_t(), g2, orig_vertices[i]));
  }

  for (edge_iterator ei = edges(g2).first; ei != edges(g2).second; ++ei) {
    typename std::map<vertex2, vertex1>::iterator si = vertex_map.find(source(*ei, g2)),
      ti = vertex_map.find(target(*ei, g2));
    if ((si != vertex_map.end()) && (ti != vertex_map.end()))
      add_edge(si->second, ti->second, get(edge_name_t(), g2, *ei), g1);
  }
}


template<typename Graph>
void generate_random_digraph(Graph& g, double edge_probability, 
                             int max_parallel_edges, double parallel_edge_probability,
                             int max_edge_name, int max_vertex_name) {

  BOOST_REQUIRE((0 <= edge_probability) && (edge_probability <= 1));
  BOOST_REQUIRE((0 <= parallel_edge_probability) && (parallel_edge_probability <= 1));
  BOOST_REQUIRE(0 <= max_parallel_edges);
  BOOST_REQUIRE(0 <= max_edge_name);
  BOOST_REQUIRE(0 <= max_vertex_name);

  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  boost::mt19937 random_gen;
  boost::uniform_real<double> dist_real(0.0, 1.0);
  boost::variate_generator<boost::mt19937&, boost::uniform_real<double> >
    random_real_dist(random_gen, dist_real);

  for (vertex_iterator u = vertices(g).first; u != vertices(g).second; ++u) {
    for (vertex_iterator v = vertices(g).first; v != vertices(g).second; ++v) {
      if (random_real_dist() <= edge_probability) {
        add_edge(*u, *v, g);
        for (int i = 0; i < max_parallel_edges; ++i) {
          if (random_real_dist() <= parallel_edge_probability)
            add_edge(*u, *v, g);
        }
      }
    }
  }
  
  {
    boost::uniform_int<int> dist_int(0, max_edge_name);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<int> >
      random_int_dist(random_gen, dist_int);
    randomize_property<vertex_name_t>(g, random_int_dist);
  }

  {
    boost::uniform_int<int> dist_int(0, max_vertex_name);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<int> >
      random_int_dist(random_gen, dist_int);
    
    randomize_property<edge_name_t>(g, random_int_dist);
  }

}


template <typename Graph1,
          typename Graph2,
          typename EdgeEquivalencePredicate,
          typename VertexEquivalencePredicate>
struct test_callback {
  
  test_callback(const Graph1& graph1, const Graph2& graph2, 
                EdgeEquivalencePredicate edge_comp, 
                VertexEquivalencePredicate vertex_comp, bool output) 
    : graph1_(graph1), graph2_(graph2), edge_comp_(edge_comp), vertex_comp_(vertex_comp),
      output_(output) {}
  
  template <typename CorrespondenceMap1To2,
            typename CorrespondenceMap2To1>
  bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) {
    
    bool verified;
    BOOST_CHECK(verified = verify_vf2_subgraph_iso(graph1_, graph2_, f, edge_comp_, vertex_comp_));
  
    // Output (sub)graph isomorphism map
    if (!verified || output_) {
      std::cout << "Verfied: " << std::boolalpha << verified << std::endl;
      std::cout << "Num vertices: " << num_vertices(graph1_) << ' ' << num_vertices(graph2_) << std::endl;
      BGL_FORALL_VERTICES_T(v, graph1_, Graph1) 
        std::cout << '(' << get(vertex_index_t(), graph1_, v) << ", " 
                  << get(vertex_index_t(), graph2_, get(f, v)) << ") ";
    
      std::cout << std::endl;
    }
    
    return true;
  }
 
private:
  const Graph1& graph1_;
  const Graph2& graph2_;
  EdgeEquivalencePredicate edge_comp_;
  VertexEquivalencePredicate vertex_comp_;
  bool output_;
};


void test_vf2_sub_graph_iso(int n1, int n2, double edge_probability, 
                            int max_parallel_edges, double parallel_edge_probability,
                            int max_edge_name, int max_vertex_name, bool output) {

  typedef property<edge_name_t, int> edge_property;
  typedef property<vertex_name_t, int, property<vertex_index_t, int> > vertex_property;
  
  typedef adjacency_list<listS, listS, bidirectionalS, vertex_property, edge_property> graph1;
  typedef adjacency_list<vecS, vecS, bidirectionalS, vertex_property, edge_property> graph2;
  
  graph1 g1(n1);
  graph2 g2(n2);
  generate_random_digraph(g2, edge_probability, max_parallel_edges, parallel_edge_probability,
                          max_edge_name, max_vertex_name);
  randomly_permute_graph(g1, g2);

  int v_idx = 0;
  for (graph_traits<graph1>::vertex_iterator vi = vertices(g1).first;
       vi != vertices(g1).second; ++vi) {
    put(vertex_index_t(), g1, *vi, v_idx++);
  }


  // Create vertex and edge predicates
  typedef property_map<graph1, vertex_name_t>::type vertex_name_map1;
  typedef property_map<graph2, vertex_name_t>::type vertex_name_map2;
  
  typedef property_map_equivalent<vertex_name_map1, vertex_name_map2> vertex_predicate;
  vertex_predicate vertex_comp =
    make_property_map_equivalent(get(vertex_name, g1), get(vertex_name, g2));
  
  typedef property_map<graph1, edge_name_t>::type edge_name_map1;
  typedef property_map<graph2, edge_name_t>::type edge_name_map2;
  
  typedef property_map_equivalent<edge_name_map1, edge_name_map2> edge_predicate;
  edge_predicate edge_comp =
    make_property_map_equivalent(get(edge_name, g1), get(edge_name, g2));
  
  
  std::clock_t start = std::clock();
  
  // Create callback
  test_callback<graph1, graph2, edge_predicate, vertex_predicate> 
    callback(g1, g2, edge_comp, vertex_comp, output);

  std::cout << std::endl;
  BOOST_CHECK(vf2_subgraph_iso(g1, g2, callback, vertex_order_by_mult(g1),
                               edges_equivalent(edge_comp).vertices_equivalent(vertex_comp)));

  std::clock_t end1 = std::clock();
  std::cout << "vf2_subgraph_iso: elapsed time (clock cycles): " << (end1 - start) << std::endl;

  if (num_vertices(g1) == num_vertices(g2)) {
    std::cout << std::endl;
    BOOST_CHECK(vf2_graph_iso(g1, g2, callback, vertex_order_by_mult(g1),
                              edges_equivalent(edge_comp).vertices_equivalent(vertex_comp)));
    
    std::clock_t end2 = std::clock();
    std::cout << "vf2_graph_iso: elapsed time (clock cycles): " << (end2 - end1) << std::endl;
  }
  
  if (output) {
    std::fstream file_graph1("graph1.dot", std::fstream::out);
    write_graphviz(file_graph1, g1,
                   make_label_writer(get(boost::vertex_name, g1)),
                   make_label_writer(get(boost::edge_name, g1)));
    
    std::fstream file_graph2("graph2.dot", std::fstream::out);
    write_graphviz(file_graph2, g2,
                   make_label_writer(get(boost::vertex_name, g2)),
                   make_label_writer(get(boost::edge_name, g2)));
  }
}

int test_main(int argc, char* argv[]) {

  int num_vertices_g1 = 10;
  int num_vertices_g2 = 20;
  double edge_probability = 0.4;
  int max_parallel_edges = 2;
  double parallel_edge_probability = 0.4;
  int max_edge_name = 5;
  int max_vertex_name = 5;
  bool output = false;
  
  if (argc > 1) {
    num_vertices_g1 = boost::lexical_cast<int>(argv[1]);
  }
  
  if (argc > 2) {
    num_vertices_g2 = boost::lexical_cast<int>(argv[2]);
  }
  
  if (argc > 3) {
    edge_probability = boost::lexical_cast<double>(argv[3]);
  }
  
  if (argc > 4) {
    max_parallel_edges = boost::lexical_cast<int>(argv[4]);
  }
  
  if (argc > 5) {
    parallel_edge_probability = boost::lexical_cast<double>(argv[5]);
  }
  
  if (argc > 6) {
    max_edge_name = boost::lexical_cast<int>(argv[6]);
  }
  
  if (argc > 7) {
    max_vertex_name = boost::lexical_cast<int>(argv[7]);
  }
  
  if (argc > 8) {
    output = boost::lexical_cast<bool>(argv[8]);
  }

  test_vf2_sub_graph_iso(num_vertices_g1, num_vertices_g2, edge_probability, 
                         max_parallel_edges, parallel_edge_probability, 
                         max_edge_name, max_vertex_name, output);
  
  return 0;
}
