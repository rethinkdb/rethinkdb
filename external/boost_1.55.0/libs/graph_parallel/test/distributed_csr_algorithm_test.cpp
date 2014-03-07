// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine

// A test of the distributed compressed sparse row graph type
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/concepts.hpp>

#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/graph/rmat_graph_generator.hpp>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/distributed/delta_stepping_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/distributed/page_rank.hpp>
#include <boost/graph/distributed/boman_et_al_graph_coloring.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/distributed/betweenness_centrality.hpp>
#include <boost/graph/distributed/dehne_gotz_min_spanning_tree.hpp>
#include <boost/graph/distributed/st_connected.hpp>

#if 0 // Contains internal AdjList types not present in CSR graph
#  include <boost/graph/distributed/connected_components_parallel_search.hpp>
#endif

#include <boost/graph/distributed/vertex_list_adaptor.hpp> // Needed for MST

#include <boost/random/linear_congruential.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include <boost/test/minimal.hpp>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif


/****************************************************************************
 * Edge weight generator iterator                                           *
 ****************************************************************************/
template<typename F, typename RandomGenerator>
class generator_iterator
{
public:
  typedef std::input_iterator_tag iterator_category;
  typedef typename F::result_type value_type;
  typedef const value_type&       reference;
  typedef const value_type*       pointer;
  typedef void                    difference_type;

  explicit 
  generator_iterator(RandomGenerator& gen, const F& f = F()) 
    : f(f), gen(&gen) 
  { 
    value = this->f(gen); 
  }

  reference operator*() const  { return value; }
  pointer   operator->() const { return &value; }

  generator_iterator& operator++()
  {
    value = f(*gen);
    return *this;
  }

  generator_iterator operator++(int)
  {
    generator_iterator temp(*this);
    ++(*this);
    return temp;
  }

  bool operator==(const generator_iterator& other) const
  { return f == other.f; }

  bool operator!=(const generator_iterator& other) const
  { return !(*this == other); }

private:
  F f;
  RandomGenerator* gen;
  value_type value;
};

template<typename F, typename RandomGenerator>
inline generator_iterator<F, RandomGenerator> 
make_generator_iterator( RandomGenerator& gen, const F& f)
{ return generator_iterator<F, RandomGenerator>(gen, f); }


/****************************************************************************
 * Printing DFS Visitor                                                     *
 ****************************************************************************/

struct printing_dfs_visitor
{
  template<typename Vertex, typename Graph>
  void initialize_vertex(Vertex v, const Graph& g)
  {
    vertex_event("initialize_vertex", v, g);
  }

  template<typename Vertex, typename Graph>
  void start_vertex(Vertex v, const Graph& g)
  {
    vertex_event("start_vertex", v, g);
  }

  template<typename Vertex, typename Graph>
  void discover_vertex(Vertex v, const Graph& g)
  {
    vertex_event("discover_vertex", v, g);
  }

  template<typename Edge, typename Graph>
  void examine_edge(Edge e, const Graph& g)
  {
    edge_event("examine_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g)
  {
    edge_event("tree_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void back_edge(Edge e, const Graph& g)
  {
    edge_event("back_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void forward_or_cross_edge(Edge e, const Graph& g)
  {
    edge_event("forward_or_cross_edge", e, g);
  }

  template<typename Vertex, typename Graph>
  void finish_vertex(Vertex v, const Graph& g)
  {
    vertex_event("finish_vertex", v, g);
  }

private:
  template<typename Vertex, typename Graph>
  void vertex_event(const char* name, Vertex v, const Graph& g)
  {
    std::cerr << "#" << process_id(g.process_group()) << ": " << name << "("
              << get_vertex_name(v, g) << ": " << local(v) << "@" << owner(v)
              << ")\n";
  }

  template<typename Edge, typename Graph>
  void edge_event(const char* name, Edge e, const Graph& g)
  {
    std::cerr << "#" << process_id(g.process_group()) << ": " << name << "("
              << get_vertex_name(source(e, g), g) << ": "
              << local(source(e, g)) << "@" << owner(source(e, g)) << ", "
              << get_vertex_name(target(e, g), g) << ": "
              << local(target(e, g)) << "@" << owner(target(e, g)) << ")\n";
  }

};

using namespace boost;
using boost::graph::distributed::mpi_process_group;

typedef int weight_type;

struct WeightedEdge {
  WeightedEdge(weight_type weight = 0) : weight(weight) { }
  
  weight_type weight;

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/)
  {
    ar & weight;
  }
};

struct VertexProperties {
  VertexProperties(int d = 0)
    : distance(d) { }

  int distance;

  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/)
  {
    ar & distance;
  }
};

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  typedef compressed_sparse_row_graph<directedS, VertexProperties, WeightedEdge, 
                                      no_property, distributedS<mpi_process_group> >
    Digraph;

  // Make sure we can build graphs using common graph generators
  typedef sorted_erdos_renyi_iterator<minstd_rand, Digraph> ERIter;
  typedef small_world_iterator<minstd_rand, Digraph> SWIter;
  typedef sorted_rmat_iterator<minstd_rand, Digraph> RMATIter;

  typedef graph_traits<Digraph>::edge_descriptor edge_descriptor;

  int n = 40;
  int k = 3;
  double prob = 0.1;
  int C = 10;
  double a = 0.5, b = 0.1, c = 0.25, d = 0.15;
  int iterations = 50;
  int num_colors = n / 10;
  int lookahead = C / 10;

  minstd_rand gen;

  // Directed Graphs
  Digraph g(ERIter(gen, n, prob), ERIter(), 
            make_generator_iterator(gen, uniform_int<int>(0, C)),
            n);
  Digraph g2(SWIter(gen, n, k, prob), SWIter(), n);
  Digraph g3(RMATIter(gen, n, size_t(n*n*prob), a, b, c, d), RMATIter(), n);

  // Test BFS
  breadth_first_search(g, vertex(0, g), visitor(bfs_visitor<>()));

  // Test SSSP Algorithms
  graph::distributed::delta_stepping_shortest_paths(g, 
                                                    vertex(0, g),
                                                    dummy_property_map(),
                                                    get(&VertexProperties::distance, g),
                                                    get(&WeightedEdge::weight, g));

  dijkstra_shortest_paths(g, vertex(0, g),
                          distance_map(get(&VertexProperties::distance, g)).
                          weight_map(get(&WeightedEdge::weight, g)));

  dijkstra_shortest_paths(g, vertex(0, g),
                          distance_map(get(&VertexProperties::distance, g)).
                          weight_map(get(&WeightedEdge::weight, g)).
                          lookahead(lookahead));

  // Test PageRank
  using boost::graph::n_iterations;

  std::vector<double> ranks(num_vertices(g));

  page_rank(g,
            make_iterator_property_map(ranks.begin(),
                                       get(boost::vertex_index, g)),
            n_iterations(iterations), 0.85, vertex(0, g));

  // Test Graph Coloring
  typedef property_map<Digraph, vertex_index_t>::type vertex_index_map;

  std::vector<int> colors_vec(num_vertices(g));
  iterator_property_map<int*, vertex_index_map> color(&colors_vec[0],
                                                      get(vertex_index, g));

  graph::boman_et_al_graph_coloring(g, color, num_colors);

  // Test DFS
  //
  // DFS requires an undirected graph, currently CSR graphs must be directed
#if 0
  std::vector<vertex_descriptor> parent(num_vertices(g));
  std::vector<vertex_descriptor> explore(num_vertices(g));

  boost::graph::tsin_depth_first_visit
    (g,
     vertex(0, g),
     printing_dfs_visitor(),
     color,
     make_iterator_property_map(parent.begin(), get(vertex_index, g)),
     make_iterator_property_map(explore.begin(), get(vertex_index, g)),
     get(vertex_index, g));
#endif

  // Test S-T Connected
  st_connected(g, vertex(0, g), vertex(1, g), color, get(vertex_owner, g));

  // Test Connected Components
  //
  // CC requires an undirected graph, currently CSR graphs must be directed
#if 0
  std::vector<int> local_components_vec(num_vertices(g));
  typedef iterator_property_map<std::vector<int>::iterator, 
                                vertex_index_map> ComponentMap;
  ComponentMap component(local_components_vec.begin(), get(vertex_index, g));

  assert(connected_components(g, component) == 
         connected_components_ps(g, component));
#endif

  // Test Betweenness Centrality
  //
  // Betweenness Centrality is broken at the moment
  typedef iterator_property_map<std::vector<int>::iterator, vertex_index_map> 
    CentralityMap;

  std::vector<int> centralityS(num_vertices(g), 0);
  CentralityMap centrality(centralityS.begin(), get(vertex_index, g));

  brandes_betweenness_centrality(g, centrality);

  //
  // Test MST
  //
  std::vector<edge_descriptor> mst_edges;

  dense_boruvka_minimum_spanning_tree(make_vertex_list_adaptor(g), 
                                      get(&WeightedEdge::weight, g),
                                      std::back_inserter(mst_edges));

  mst_edges.clear();
  merge_local_minimum_spanning_trees(make_vertex_list_adaptor(g), 
                                     get(&WeightedEdge::weight, g),
                                     std::back_inserter(mst_edges));

  mst_edges.clear();
  boruvka_then_merge(make_vertex_list_adaptor(g), 
                     get(&WeightedEdge::weight, g),
                     std::back_inserter(mst_edges));

  mst_edges.clear();
  boruvka_mixed_merge(make_vertex_list_adaptor(g), 
                      get(&WeightedEdge::weight, g),
                      std::back_inserter(mst_edges));

  // Test Strong Components
  //
  // Can't build reverse graph of a CSR graph
#if 0
  std::vector<int> local_components_vec(num_vertices(g));
  typedef iterator_property_map<std::vector<int>::iterator, 
                                vertex_index_map> ComponentMap;
  ComponentMap component(local_components_vec.begin(), get(vertex_index, g));

  int num_components = strong_components(g, component);
#endif

  std::ofstream out("dcsr.dot");
  write_graphviz(out, g);

  return 0;
}
