// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>

#define CSR

#ifdef CSR
#  include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#else
#  include <boost/graph/distributed/adjacency_list.hpp>
#endif

#include <boost/test/minimal.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/queue.hpp>

#include <boost/graph/parallel/distribution.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <sys/time.h>
#include <time.h>

#include <boost/random.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>

#include <boost/random/linear_congruential.hpp>

#include <boost/graph/distributed/graphviz.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/pending/queue.hpp>

#include <boost/graph/rmat_graph_generator.hpp>
#include <boost/graph/distributed/betweenness_centrality.hpp>
#include <boost/graph/distributed/filtered_graph.hpp>
#include <boost/graph/parallel/container_traits.hpp>

#include <boost/graph/properties.hpp>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <stdint.h>

using namespace boost;

// #define DEBUG

typedef rand48 RandomGenerator;

/****************************************************************************
 * Timing
 ****************************************************************************/
#ifndef PBGL_ACCOUNTING 

typedef double time_type;

inline time_type get_time()
{
  timeval tp;
  gettimeofday(&tp, 0);
  return tp.tv_sec + tp.tv_usec / 1000000.0;
}

std::string print_time(time_type t)
{
  std::ostringstream out;
  out << std::setiosflags(std::ios::fixed) << std::setprecision(2) << t;
  return out.str();
}

#endif // PBGL_ACCOUNTING

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

template<typename Graph, typename DistanceMap, typename WeightMap, typename ColorMap>
struct ssca_visitor : bfs_visitor<> 
{
  typedef typename property_traits<WeightMap>::value_type Weight;
  typedef typename property_traits<ColorMap>::value_type ColorValue;
  typedef color_traits<ColorValue> Color;

  ssca_visitor(DistanceMap& distance, const WeightMap& weight, ColorMap& color,
               Weight max_)
    : distance(distance), weight(weight), color(color), max_(max_) {}

  template<typename Edge>
  void tree_edge(Edge e, const Graph& g)
  {
    int new_distance = get(weight, e) == (std::numeric_limits<Weight>::max)() ? 
      (std::numeric_limits<Weight>::max)() : get(distance, source(e, g)) + get(weight, e);

    put(distance, target(e, g), new_distance);
    if (new_distance > max_)
      put(color, target(e, g), Color::black());
  }

 private:
  DistanceMap& distance;
  const WeightMap& weight;
  ColorMap& color;
  Weight max_;
};

// Generate source vertices for approximate BC
template <typename Graph, typename Buffer>
void
generate_sources(const Graph& g, Buffer sources, 
                 typename graph_traits<Graph>::vertices_size_type num_sources)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  typedef typename boost::graph::parallel::process_group_type<Graph>::type 
    process_group_type;
  process_group_type pg = g.process_group();

  typename process_group_type::process_id_type id = process_id(pg);
  typename process_group_type::process_size_type p = num_processes(pg);    
  
  // Don't feel like adding a special case for num_sources < p
  assert(num_sources >= p);
  
  minstd_rand gen;
  uniform_int<vertices_size_type> rand_vertex(0, num_vertices(g) - 1);
  std::vector<vertex_descriptor> all_sources, local_sources;
  vertices_size_type local_vertices = vertices_size_type(floor((double)num_sources / p));
  local_vertices += (id < (num_sources - (p * local_vertices)) ? 1 : 0); 
  
  while (local_vertices > 0) {
    vertex_iterator iter = vertices(g).first;
    std::advance(iter, rand_vertex(gen));
    
    if (out_degree(*iter, g) != 0
        && std::find(local_sources.begin(), local_sources.end(), *iter) == local_sources.end()) {
      local_sources.push_back(*iter);
      --local_vertices;
    }
  }
  all_gather(pg, local_sources.begin(), local_sources.end(), all_sources);
  std::sort(all_sources.begin(), all_sources.end());
  for (typename std::vector<vertex_descriptor>::iterator iter = all_sources.begin(); 
       iter != all_sources.end(); ++iter)
    sources.push(*iter);
}

// Kernel 2 - Classify large sets
template <typename Graph, typename WeightMap>
void classify_sets(const Graph& g, const WeightMap& weight_map, 
                   std::vector<std::pair<typename graph_traits<Graph>::vertex_descriptor,
                                         typename graph_traits<Graph>::vertex_descriptor> > & global_S)
{
  typedef typename boost::graph::parallel::process_group_type<Graph>::type 
    process_group_type;
  process_group_type pg = g.process_group();

  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  std::vector<std::pair<vertex_descriptor, vertex_descriptor> > S;

  time_type start = get_time();

#ifdef CSR
  typedef typename property_map<Graph, vertex_owner_t>::const_type OwnerMap;
  typedef typename property_map<Graph, vertex_local_t>::const_type LocalMap;
  OwnerMap owner = get(vertex_owner, g);
  LocalMap local = get(vertex_local, g);
#endif

  int max_ = 0;
  BGL_FORALL_EDGES_T(e, g, Graph) {
#ifdef CSR
    if (get(owner, source(e, g)) == process_id(pg)) {
#endif
    int w = get(weight_map, e);
    if (w > max_) {
      max_ = w;
      S.clear();
    }

    if (w >= max_)
      S.push_back(std::make_pair(source(e, g), target(e, g)));
#ifdef CSR
    }
#endif
  }

  int global_max = all_reduce(pg, max_, boost::parallel::maximum<int>());
  if (max_ < global_max)
    S.clear();

  global_S.clear();

  all_gather(pg, S.begin(), S.end(), global_S);
  
  // This is probably unnecessary as long as the sets of edges owned by procs is disjoint
  std::sort(global_S.begin(), global_S.end());
  std::unique(global_S.begin(), global_S.end());

  synchronize(pg);

  time_type end = get_time();

  if (process_id(pg) == 0) {
    std::cerr << "    Distributed Graph: " << print_time(end - start) << std::endl
              << "      Max int weight = " << global_max << std::endl;
  }
}

template <typename ProcessGroup, typename Graph, typename WeightMap, 
          typename EdgeVector>
void seq_classify_sets(const ProcessGroup& pg, const Graph& g, 
                       const WeightMap& weight_map, EdgeVector& S)
{
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename property_traits<WeightMap>::value_type edge_weight_type;

  time_type start = get_time();

  edge_weight_type max_ = 0;
  BGL_FORALL_EDGES_T(e, g, Graph) {
    edge_weight_type w = get(weight_map, e);
    if (w > max_) {
      max_ = w;
      S.clear();
    }

    if (w >= max_)
      S.push_back(e);
  }

  synchronize(pg);

  time_type end = get_time();

  if (process_id(pg) == 0) 
    std::cerr << "    Non-Distributed Graph: " << print_time(end - start) << std::endl
              << "      Max int weight = " << max_ << std::endl;
}

// Kernel 3 - Graph Extraction
template <typename Graph, typename OwnerMap, typename LocalMap, 
          typename WeightMap, typename DistanceMap, typename ColorMap,
          typename EdgeVector>
void subgraph_extraction(Graph& g, const OwnerMap& owner, const LocalMap& local,
                         const WeightMap& weight_map, DistanceMap distances,
                         ColorMap color_map, const EdgeVector& S,
                         int subGraphEdgeLength)
{
  // Nick: I think turning the vertex black when the maximum distance is
  //       exceeded will prevent BFS from exploring beyond the subgraph.
  //       Unfortunately we can't run subgraph extraction in parallel
  //       because the subgraphs may overlap

  typedef typename property_traits<ColorMap>::value_type ColorValue;
  typedef color_traits<ColorValue> Color;

  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  typedef typename boost::graph::parallel::process_group_type<Graph>::type 
    process_group_type;

  typedef boost::graph::distributed::distributed_queue<process_group_type, 
    OwnerMap, queue<vertex_descriptor> > queue_t;

  process_group_type pg = g.process_group();
  typename process_group_type::process_id_type id = process_id(pg);

  queue_t Q(pg, owner);

  EdgeVector sources(S.begin(), S.end());

#ifdef DEBUG
  std::vector<std::vector<vertex_descriptor> > subgraphs;
#endif

  synchronize(pg);

  typedef typename std::vector<std::pair<vertex_descriptor, vertex_descriptor> >::iterator
    source_iterator;

  time_type start = get_time();
  for (source_iterator iter = sources.begin(); iter != sources.end(); ++iter) {
    // Reinitialize distance and color maps every BFS
    BGL_FORALL_VERTICES_T(v, g, Graph) { 
      if (get(owner, v) == id) {
        local_put(color_map, v, Color::white()); 
        local_put(distances, v, (std::numeric_limits<int>::max)());
      }
    }

    vertex_descriptor u = iter->first, v = iter->second;

    local_put(distances, u, 0);
    local_put(distances, v, 0);

    while (!Q.empty()) Q.pop();
    if (get(owner, u) == id)
      Q.push(u);

    local_put(color_map, u, Color::gray()); 

    breadth_first_search(g, v, Q,
                         ssca_visitor<Graph, DistanceMap, WeightMap, ColorMap>
                           (distances, weight_map, color_map, subGraphEdgeLength),
                         color_map);
                           
    // At this point all vertices with distance > 0 in addition to the 
    // starting vertices compose the subgraph.
#ifdef DEBUG
    subgraphs.push_back(std::vector<vertex_descriptor>());    
    std::vector<vertex_descriptor>& subgraph = subgraphs.back();

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (get(distances, v) < (std::numeric_limits<int>::max)())
        subgraph.push_back(v);
    }
#endif    
  }

  synchronize(pg);

  time_type end = get_time();

#ifdef DEBUG
  for (unsigned int i = 0; i < subgraphs.size(); i++) {
    all_gather(pg, subgraphs[i].begin(), subgraphs[i].end(), subgraphs[i]);
    std::sort(subgraphs[i].begin(), subgraphs[i].end()); 
    subgraphs[i].erase(std::unique(subgraphs[i].begin(), subgraphs[i].end()),
                       subgraphs[i].end()); 
  }

  if (process_id(pg) == 0)
    for (int i = 0; abs(i) < subgraphs.size(); i++) {
      std::cerr << "Subgraph " << i << " :\n";
      for (int j = 0; abs(j) < subgraphs[i].size(); j++)
        std::cerr << "  " << get(local, subgraphs[i][j]) << "@" 
                  << get(owner, subgraphs[i][j]) << std::endl;
    }
#endif

  if (process_id(pg) == 0)
    std::cerr << "    Distributed Graph: " << print_time(end - start) << std::endl;
}

template <typename ProcessGroup, typename Graph, typename WeightMap, 
          typename DistanceMap, typename ColorMap, typename EdgeVector>
void seq_subgraph_extraction(const ProcessGroup& pg, const Graph& g, 
                             const WeightMap& weight_map, DistanceMap distances,
                             ColorMap color_map, const EdgeVector& S,
                             int subGraphEdgeLength)
{
  // Nick: I think turning the vertex black when the maximum distance is
  //       exceeded will prevent BFS from exploring beyond the subgraph.

  using boost::graph::distributed::mpi_process_group;

  typedef typename property_traits<ColorMap>::value_type ColorValue;
  typedef color_traits<ColorValue> Color;

  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  boost::queue<vertex_descriptor> Q;

  std::vector<edge_descriptor> sources(S.begin(), S.end());

#ifdef DEBUG
  std::vector<std::vector<vertex_descriptor> > subgraphs;
#endif

  synchronize(pg);
    
  typedef ProcessGroup process_group_type;
  
  typename process_group_type::process_id_type id = process_id(pg);
  typename process_group_type::process_size_type p = num_processes(pg);    

  time_type start = get_time();

  for (int i = id; i < sources.size(); i += p) {

    // Reinitialize distance and color maps every BFS
    BGL_FORALL_VERTICES_T(v, g, Graph) { 
      put(color_map, v, Color::white()); 
      put(distances, v, (std::numeric_limits<int>::max)());
    }

    vertex_descriptor u = source(sources[i], g),
      v = target(sources[i], g);

    put(distances, u, 0);
    put(distances, v, 0); 

    while (!Q.empty()) Q.pop();
    Q.push(u);

    put(color_map, u, Color::gray()); 

    breadth_first_search(g, v, Q,
                         ssca_visitor<Graph, DistanceMap, WeightMap, ColorMap>
                           (distances, weight_map, color_map, subGraphEdgeLength),
                         color_map);

#ifdef DEBUG
    subgraphs.push_back(std::vector<vertex_descriptor>());    
    std::vector<vertex_descriptor>& subgraph = subgraphs.back();

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (get(distances, v) < (std::numeric_limits<int>::max)())
        subgraph.push_back(v);
    }
#endif    
  }

  synchronize(pg);

  time_type end = get_time();

#ifdef DEBUG
  std::vector<vertex_descriptor> ser_subgraphs;

  for (int i = 0; i < subgraphs.size(); ++i) {
    for (int j = 0; j < subgraphs[i].size(); ++j)
      ser_subgraphs.push_back(subgraphs[i][j]);
    ser_subgraphs.push_back(graph_traits<Graph>::null_vertex());
  }

  all_gather(pg, ser_subgraphs.begin(), ser_subgraphs.end(), ser_subgraphs);

  int i = 0;
  typename std::vector<vertex_descriptor>::iterator iter(ser_subgraphs.begin());

  while (iter != ser_subgraphs.end()) {
    std::cerr << "Subgraph " << i << " :\n";
    while (*iter != graph_traits<Graph>::null_vertex()) {
      std::cerr << "  " << *iter << std::endl;
      ++iter;
    }
    ++i;
    ++iter;
  }
#endif

  if (process_id(pg) == 0)
    std::cerr << "    Non-Distributed Graph: " << print_time(end - start) << std::endl;
}

template <typename ProcessGroup, typename Graph, typename CentralityMap>
void
extract_max_bc_vertices(const ProcessGroup& pg, const Graph& g, const CentralityMap& centrality,
                        std::vector<typename graph_traits<Graph>::vertex_descriptor>& max_bc_vec)
{
  using boost::graph::parallel::process_group;
  using boost::parallel::all_gather;
  using boost::parallel::all_reduce;

  // Find set of vertices with highest BC score
  typedef typename property_traits<CentralityMap>::value_type centrality_type;
  std::vector<centrality_type> max_bc_vertices;
  centrality_type max_ = 0;

  max_bc_vec.clear();

  BGL_FORALL_VERTICES_T(v, g, Graph) {
    if (get(centrality, v) == max_)
      max_bc_vec.push_back(v);
    else if (get(centrality, v) > max_) {
      max_ = get(centrality, v);
      max_bc_vec.clear();
      max_bc_vec.push_back(v);      
    }
  }

  centrality_type global_max = all_reduce(pg, max_, boost::parallel::minimum<int>());

  if (global_max > max_)
    max_bc_vec.clear();

  all_gather(pg, max_bc_vec.begin(), max_bc_vec.end(), max_bc_vec);
}


// Function object to filter edges divisible by 8
// EdgeWeightMap::value_type must be integral!
template <typename EdgeWeightMap>
struct edge_weight_not_divisible_by_eight {
  typedef typename property_traits<EdgeWeightMap>::value_type weight_type;

  edge_weight_not_divisible_by_eight() { }
  edge_weight_not_divisible_by_eight(EdgeWeightMap weight) : m_weight(weight) { }
  template <typename Edge>
  bool operator()(const Edge& e) const {
    return (get(m_weight, e) & ((std::numeric_limits<weight_type>::max)() - 7)) != get(m_weight, e);
  }

  EdgeWeightMap m_weight;
};

//
// Vertex and Edge properties
//
#ifdef CSR
typedef int weight_type;

struct WeightedEdge {
  WeightedEdge(weight_type weight = 0) : weight(weight) { }
  
  weight_type weight;
};

struct VertexProperties {
  VertexProperties(int distance = 0, default_color_type color = white_color)
    : distance(distance), color(color) { }

  int distance;
  default_color_type color;
};
#endif

template <typename RandomGenerator, typename ProcessGroup, typename vertices_size_type,
          typename edges_size_type>
void
run_non_distributed_graph_tests(RandomGenerator& gen, const ProcessGroup& pg,
                                vertices_size_type n, edges_size_type m, 
                                std::size_t maxEdgeWeight, uint64_t seed, 
                                int K4Alpha, double a, double b, double c, double d, 
                                int subGraphEdgeLength, bool show_degree_dist, 
                                bool full_bc, bool verify)
{
#ifdef CSR
  typedef compressed_sparse_row_graph<directedS, VertexProperties, WeightedEdge>
    seqGraph;
#else
  typedef adjacency_list<vecS, vecS, directedS, 
                         // Vertex properties
                         property<vertex_distance_t, int,
                         property<vertex_color_t, default_color_type> >,
                         // Edge properties
                         property<edge_weight_t, int> > seqGraph;
#endif

  // Generate sequential graph for non_distributed betweenness centrality
  // Reseed the PRNG to get the same graph
  gen.seed(seed);

  synchronize(pg);

  time_type start = get_time();

#ifdef CSR
  seqGraph sg(edges_are_sorted,
              sorted_unique_rmat_iterator<RandomGenerator, seqGraph>(gen, n, m, a, b, c, d),
              sorted_unique_rmat_iterator<RandomGenerator, seqGraph>(),
              make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
              n);
#else
  seqGraph sg(unique_rmat_iterator<RandomGenerator, seqGraph>(gen, n, m, a, b, c, d),
              unique_rmat_iterator<RandomGenerator, seqGraph>(),
              make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
              n);
#endif

  // Not strictly necessary to synchronize here, but it make sure that the
  // time we measure is the time needed for all copies of the graph to be 
  // constructed
  synchronize(pg);

  time_type end = get_time();

  if (process_id(pg) == 0)
    std::cerr<< "Kernel 1:\n" 
             << "    Non-Distributed Graph: " << print_time(end - start) << std::endl;

  std::map<int, int> degree_dist;
  if ( show_degree_dist ) {
    BGL_FORALL_VERTICES_T(v, sg, seqGraph) {
      degree_dist[out_degree(v, sg)]++;
    }

    std::cerr << "Degree - Fraction of vertices of that degree\n";
    for (std::map<int, int>::iterator iter = degree_dist.begin(); 
         iter != degree_dist.end(); ++iter)
      std::cerr << "  " << iter->first << " - " << double(iter->second) / num_vertices(sg) << std::endl << std::endl;
  }

  //
  // Kernel 2 - Classify large sets
  //
  std::vector<graph_traits<seqGraph>::edge_descriptor> seqS;

  if (process_id(pg) == 0)
    std::cerr << "Kernel 2:\n";

  seq_classify_sets(pg, sg,
#ifdef CSR
                    get(&WeightedEdge::weight, sg),
#else
                    get(edge_weight, sg),
#endif
                    seqS);

  //
  // Kernel 3 - Graph Extraction
  //
#ifdef CSR
  typedef weight_type weight_t;
  weight_t unit_weight(1);
#else
  int unit_weight(1);;
#endif

  if (process_id(pg) == 0)
    std::cerr << "Kernel 3:\n";

  seq_subgraph_extraction(pg, sg,
#ifdef CSR
//                        get(&WeightedEdge::weight, sg), 
                          ref_property_map<graph_traits<seqGraph>::edge_descriptor, weight_t>(unit_weight),
                          get(&VertexProperties::distance, sg),
                          get(&VertexProperties::color, sg), 
#else
//                        get(edge_weight, sg), 
                          ref_property_map<graph_traits<seqGraph>::edge_descriptor, int>(unit_weight),
                          get(vertex_distance, sg), 
                          get(vertex_color, sg), 
#endif
                          seqS, subGraphEdgeLength); 

#ifdef CSR
  typedef property_map<seqGraph, weight_type WeightedEdge::*>::type seqEdgeWeightMap;
  edge_weight_not_divisible_by_eight<seqEdgeWeightMap> sg_filter(get(&WeightedEdge::weight, sg));
#else
  typedef property_map<seqGraph, edge_weight_t>::type seqEdgeWeightMap;
  edge_weight_not_divisible_by_eight<seqEdgeWeightMap> sg_filter(get(edge_weight, sg));
#endif

  typedef filtered_graph<const seqGraph, edge_weight_not_divisible_by_eight<seqEdgeWeightMap> > 
    filteredSeqGraph;

  filteredSeqGraph fsg(sg, sg_filter);

  std::vector<graph_traits<seqGraph>::vertex_descriptor> max_seq_bc_vec;

  // Non-Distributed Centrality Map
  typedef property_map<seqGraph, vertex_index_t>::const_type seqIndexMap;
  typedef iterator_property_map<std::vector<int>::iterator, seqIndexMap> seqCentralityMap;

  std::vector<int> non_distributed_centralityS(num_vertices(sg), 0);
  seqCentralityMap non_distributed_centrality(non_distributed_centralityS.begin(), 
                                              get(vertex_index, sg));

  vertices_size_type n0 = 0;
  BGL_FORALL_VERTICES_T(v, fsg, filteredSeqGraph) {
    if (out_degree(v, fsg) == 0) ++n0;
  }

  if (process_id(pg) == 0)
    std::cerr << "Kernel 4:\n";

  // Run Betweenness Centrality
  if (full_bc) {

    // Non-Distributed Graph BC
    start = get_time();
    non_distributed_brandes_betweenness_centrality(pg, fsg, non_distributed_centrality);
    extract_max_bc_vertices(pg, fsg, non_distributed_centrality, max_seq_bc_vec);
    end = get_time();

    edges_size_type nonDistributedExactTEPs = edges_size_type(floor(7 * n* (n - n0) / (end - start)));

    if (process_id(pg) == 0)
      std::cerr << "    non-Distributed Graph Exact = " << print_time(end - start) << " ("
                << nonDistributedExactTEPs << " TEPs)\n";
  }

  // Non-Distributed Graph Approximate BC
  std::vector<int> nonDistributedApproxCentralityS(num_vertices(sg), 0);
  seqCentralityMap nonDistributedApproxCentrality(nonDistributedApproxCentralityS.begin(), 
                                                  get(vertex_index, sg));

  queue<typename graph_traits<filteredSeqGraph>::vertex_descriptor> sources;
  {
    minstd_rand gen;
    uniform_int<vertices_size_type> rand_vertex(0, num_vertices(fsg) - 1);
    int remaining_sources = floor(pow(2, K4Alpha));
    std::vector<typename graph_traits<filteredSeqGraph>::vertex_descriptor> temp_sources;
 
    while (remaining_sources > 0) {
      typename graph_traits<filteredSeqGraph>::vertex_descriptor v = 
        vertex(rand_vertex(gen), fsg);
    
      if (out_degree(v, fsg) != 0
          && std::find(temp_sources.begin(), temp_sources.end(), v) == temp_sources.end()) {
        temp_sources.push_back(v);
        --remaining_sources;
      }
    }

    for (int i = 0; i < temp_sources.size(); ++i)
      sources.push(temp_sources[i]);
  }

  start = get_time();
  non_distributed_brandes_betweenness_centrality(pg, fsg, buffer(sources).
                                                 centrality_map(nonDistributedApproxCentrality)); 
  extract_max_bc_vertices(pg, fsg, nonDistributedApproxCentrality, max_seq_bc_vec);
  end = get_time();

  edges_size_type nonDistributedApproxTEPs = edges_size_type(floor(7 * n * pow(2, K4Alpha) / (end - start)));

  if (process_id(pg) == 0)
    std::cerr << "    Non-Distributed Graph Approximate (" << floor(pow(2, K4Alpha)) << " sources) = " 
              << print_time(end - start) << " (" << nonDistributedApproxTEPs << " TEPs)\n";

  // Verify Correctness of Kernel 4
  if (full_bc && verify && process_id(pg) == 0) {

    std::vector<int> seq_centralityS(num_vertices(fsg), 0);
    seqCentralityMap seq_centrality(seq_centralityS.begin(), get(vertex_index, fsg));

    max_seq_bc_vec.clear();
    property_traits<seqCentralityMap>::value_type max_ = 0;

    start = get_time();
    brandes_betweenness_centrality(fsg, seq_centrality);

    typedef filtered_graph<const seqGraph, edge_weight_not_divisible_by_eight<seqEdgeWeightMap> >
      filteredSeqGraph;

    BGL_FORALL_VERTICES_T(v, fsg, filteredSeqGraph ) {
      if (get(seq_centrality, v) == max_)
        max_seq_bc_vec.push_back(v);
      else if (get(seq_centrality, v) > max_) {
        max_ = get(seq_centrality, v);
        max_seq_bc_vec.clear();
        max_seq_bc_vec.push_back(v);      
      }
    }

    end = get_time();

    edges_size_type sequentialTEPs = edges_size_type(floor(7 * n* (n - n0) / (end - start)));

    std::cerr << "    Sequential = " << print_time(end - start) << " (" << sequentialTEPs << " TEPs)\n";

    typename ProcessGroup::process_id_type id = process_id(pg);
    typename ProcessGroup::process_size_type p = num_processes(pg);    

    assert((double)n/p == floor((double)n/p));
    
    std::cerr << "\nVerifying non-scalable betweenness centrality...\n";

    {
      bool passed = true;

      // Verify non-scalable betweenness centrality
      BGL_FORALL_VERTICES_T(v, sg, seqGraph) {
        if (get(non_distributed_centrality, v) != get(seq_centrality, v)) {
          std::cerr << "  " << id << ": Error - centrality of " << v
                    << " does not match the sequential result (" 
                    << get(non_distributed_centrality, v) << " vs. " 
                    << get(seq_centrality, v) << ")\n";
          passed = false;
        }
      }

      if (passed)
        std::cerr << "  PASSED\n";
    }

  }    

}

template <typename RandomGenerator, typename ProcessGroup, typename vertices_size_type,
          typename edges_size_type>
void
run_distributed_graph_tests(RandomGenerator& gen, const ProcessGroup& pg,
                            vertices_size_type n, edges_size_type m, 
                            std::size_t maxEdgeWeight, uint64_t seed, 
                            int K4Alpha, double a, double b, double c, double d, 
                            int subGraphEdgeLength, bool show_degree_dist, 
                            bool emit_dot_file, bool full_bc, bool verify)
{
#ifdef CSR
  typedef compressed_sparse_row_graph<directedS, VertexProperties, WeightedEdge, no_property,
                                      distributedS<ProcessGroup> > Graph;
#else
  typedef adjacency_list<vecS, 
                         distributedS<ProcessGroup, vecS>,
                         directedS, 
                         // Vertex properties
                         property<vertex_distance_t, int,
                         property<vertex_color_t, default_color_type> >,
                         // Edge properties
                         property<edge_weight_t, int> > Graph;
#endif

  gen.seed(seed);

  parallel::variant_distribution<ProcessGroup> distrib 
    = parallel::block(pg, n);

  typedef typename ProcessGroup::process_id_type process_id_type;
  process_id_type id = process_id(pg);

  typedef typename property_map<Graph, vertex_owner_t>::const_type OwnerMap;
  typedef typename property_map<Graph, vertex_local_t>::const_type LocalMap;

  typedef keep_local_edges<parallel::variant_distribution<ProcessGroup>,
                           process_id_type>
    EdgeFilter; 

  //
  // Kernel 1 - Graph construction
  // Nick: The benchmark specifies that we only have to time graph generation from
  //       edge tuples, the generator generates the edge tuples at graph construction
  //       time so we're timing some overhead in the random number generator, etc.
  synchronize(pg);

  time_type start = get_time();

#ifdef CSR
//   typedef sorted_unique_rmat_iterator<RandomGenerator, Graph, EdgeFilter> RMATIter;
  typedef sorted_rmat_iterator<RandomGenerator, Graph, keep_all_edges> RMATIter;

  Graph g(//RMATIter(gen, n, m, a, b, c, d, false, true, EdgeFilter(distrib, id)),
          RMATIter(gen, n, m, a, b, c, d, true, keep_all_edges()),
          RMATIter(),
          make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
          n, pg, distrib);
#else
  typedef unique_rmat_iterator<RandomGenerator, Graph, EdgeFilter> RMATIter;
  Graph g(RMATIter(gen, n, m, a, b, c, d, true EdgeFilter(distrib, id)),
          RMATIter(),
          make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
          n, pg, distrib);
#endif

  synchronize(pg);

  time_type end = get_time();

  if (id == 0)
    std::cerr<< "Kernel 1:\n" 
             << "    Distributed Graph: " << print_time(end - start) << std::endl;

  if ( emit_dot_file )
    write_graphviz("ssca.dot", g);

  //
  // Kernel 2 - Classify large sets
  //
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  std::vector<std::pair<vertex_descriptor, vertex_descriptor> > S;

  if (id == 0)
    std::cerr << "Kernel 2:\n";

  classify_sets(g,
#ifdef CSR
                get(&WeightedEdge::weight, g),
#else
                get(edge_weight, g),
#endif
                S);

  //
  // Kernel 3 - Graph Extraction
  //
  OwnerMap owner = get(vertex_owner, g);
  LocalMap local = get(vertex_local, g);

  if (id == 0)
    std::cerr << "Kernel 3:\n";

#ifdef CSR
  typedef weight_type weight_t;
  weight_t unit_weight(1);
#else
  int unit_weight(1);;
#endif

  subgraph_extraction(g, owner, local, 
#ifdef CSR
//                    get(&WeightedEdge::weight, g), 
                      ref_property_map<typename graph_traits<Graph>::edge_descriptor, weight_t>(unit_weight),
                      get(&VertexProperties::distance, g),
                      get(&VertexProperties::color, g),
#else
//                    get(edge_weight, g), 
                      ref_property_map<graph_traits<Graph>::edge_descriptor, int>(unit_weight),
                      get(vertex_distance, g), 
                      get(vertex_color, g), 
#endif
                      S, subGraphEdgeLength);

  //
  // Kernel 4 - Betweenness Centrality
  //

  // Filter edges with weights divisible by 8
#ifdef CSR
  typedef typename property_map<Graph, weight_type WeightedEdge::*>::type EdgeWeightMap;
  edge_weight_not_divisible_by_eight<EdgeWeightMap> filter(get(&WeightedEdge::weight, g));
#else
  typedef typename property_map<Graph, edge_weight_t>::type EdgeWeightMap;
  edge_weight_not_divisible_by_eight<EdgeWeightMap> filter(get(edge_weight, g));
#endif

  typedef filtered_graph<const Graph, edge_weight_not_divisible_by_eight<EdgeWeightMap> > 
    filteredGraph;
  filteredGraph fg(g, filter);

  // Vectors of max BC scores for all tests
  std::vector<typename graph_traits<Graph>::vertex_descriptor> max_bc_vec; 

  // Distributed Centrality Map
  typedef typename property_map<Graph, vertex_index_t>::const_type IndexMap;
  typedef iterator_property_map<std::vector<int>::iterator, IndexMap> CentralityMap;

  std::vector<int> centralityS(num_vertices(g), 0);
  CentralityMap centrality(centralityS.begin(), get(vertex_index, g));

  // Calculate number of vertices of degree 0
  vertices_size_type local_n0 = 0, n0;
  BGL_FORALL_VERTICES_T(v, fg, filteredGraph) {
    if (out_degree(v, g) == 0) local_n0++;
  }
  n0 = boost::parallel::all_reduce(pg, local_n0, std::plus<vertices_size_type>());

  if (id == 0)
    std::cerr << "Kernel 4:\n";

  // Run Betweenness Centrality
  if (full_bc) {
    
    // Distributed Graph Full BC
    start = get_time();
    brandes_betweenness_centrality(fg, centrality);
    extract_max_bc_vertices(pg, g, centrality, max_bc_vec);
    end = get_time();
    
    edges_size_type exactTEPs = edges_size_type(floor(7 * n* (n - n0) / (end - start)));
    
    if (id == 0)
      std::cerr << "    Exact = " << print_time(end - start) << " ("
                << exactTEPs << " TEPs)\n";
  }

  // Distributed Graph Approximate BC
  std::vector<int> approxCentralityS(num_vertices(g), 0);
  CentralityMap approxCentrality(approxCentralityS.begin(), get(vertex_index, g));

  queue<vertex_descriptor> sources;
  generate_sources(g, sources, vertices_size_type(floor(pow(2, K4Alpha))));
  
  start = get_time();
  brandes_betweenness_centrality(fg, buffer(sources).centrality_map(approxCentrality)); 
  extract_max_bc_vertices(pg, fg, approxCentrality, max_bc_vec);
  end = get_time();
  
  edges_size_type approxTEPs = edges_size_type(floor(7 * n * pow(2, K4Alpha) / (end - start)));
  
  if (id == 0)
    std::cerr << "    Approximate (" << floor(pow(2, K4Alpha)) << " sources) = " 
              << print_time(end - start) << " (" << approxTEPs << " TEPs)\n";


  // Verify Correctness of Kernel 4
  if (full_bc && verify && id == 0) {

    // Build non-distributed graph to verify against
    typedef adjacency_list<vecS, vecS, directedS, 
                           // Vertex properties
                           property<vertex_distance_t, int,
                           property<vertex_color_t, default_color_type> >,
                           // Edge properties
                           property<edge_weight_t, int> > seqGraph;

    gen.seed(seed);

#ifdef CSR
    seqGraph sg(sorted_unique_rmat_iterator<RandomGenerator, seqGraph>(gen, n, m, a, b, c, d),
                sorted_unique_rmat_iterator<RandomGenerator, seqGraph>(),
                make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
                n);
#else
    seqGraph sg(unique_rmat_iterator<RandomGenerator, seqGraph>(gen, n, m, a, b, c, d),
                unique_rmat_iterator<RandomGenerator, seqGraph>(),
                make_generator_iterator(gen, uniform_int<int>(0, maxEdgeWeight)),
                n);
#endif

    typedef property_map<seqGraph, edge_weight_t>::type seqEdgeWeightMap;
    edge_weight_not_divisible_by_eight<seqEdgeWeightMap> sg_filter(get(edge_weight, sg));
    
    filtered_graph<const seqGraph, edge_weight_not_divisible_by_eight<seqEdgeWeightMap> > 
      fsg(sg, sg_filter);

    // Build sequential centrality map 
    typedef property_map<seqGraph, vertex_index_t>::const_type seqIndexMap;
    typedef iterator_property_map<std::vector<int>::iterator, seqIndexMap> seqCentralityMap;

    std::vector<int> seq_centralityS(num_vertices(sg), 0);
    seqCentralityMap seq_centrality(seq_centralityS.begin(), get(vertex_index, sg));

    std::vector<graph_traits<seqGraph>::vertex_descriptor> max_seq_bc_vec;

    max_seq_bc_vec.clear();
    property_traits<seqCentralityMap>::value_type max_ = 0;

    start = get_time();
    brandes_betweenness_centrality(fsg, seq_centrality);

    typedef filtered_graph<const seqGraph, edge_weight_not_divisible_by_eight<seqEdgeWeightMap> >
      filteredSeqGraph;

    BGL_FORALL_VERTICES_T(v, fsg, filteredSeqGraph ) {
      if (get(seq_centrality, v) == max_)
        max_seq_bc_vec.push_back(v);
      else if (get(seq_centrality, v) > max_) {
        max_ = get(seq_centrality, v);
        max_seq_bc_vec.clear();
        max_seq_bc_vec.push_back(v);      
      }
    }

    end = get_time();

    edges_size_type sequentialTEPs = edges_size_type(floor(7 * n* (n - n0) / (end - start)));

    std::cerr << "    Sequential = " << print_time(end - start) << " (" << sequentialTEPs << " TEPs)\n";
    
    typename ProcessGroup::process_size_type p = num_processes(pg);    

    assert((double)n/p == floor((double)n/p));

    std::cerr << "\nVerifying betweenness centrality...\n";

    {
      bool passed = true;

      // Verify exact betweenness centrality
      BGL_FORALL_VERTICES_T(v, g, Graph) {
        if (get(centrality, v) != seq_centralityS[(n/p) * get(owner, v) + get(local, v)]) {
          std::cerr << "  " << id << ": Error - centrality of " << get(local, v) << "@" << get(owner, v)
                    << " does not match the sequential result (" << get(centrality, v) << " vs. " 
                    << seq_centralityS[(n/p) * get(owner, v) + get(local, v)] << ")\n";
          passed = false;
        }
      }

      if (passed)
        std::cerr << "  PASSED\n";
    }      
  }    
}

void usage()
{
  std::cerr << "SSCA benchmark.\n\n"
            << "Usage : ssca [options]\n\n"
            << "Options are:\n"
            << "\t--vertices v\t\t\tNumber of vertices in the graph\n"
            << "\t--edges v\t\t\tNumber of edges in the graph\n"
            << "\t--seed s\t\t\tSeed for synchronized random number generator\n"
            << "\t--full-bc\t\t\tRun full (exact) Betweenness Centrality\n"
            << "\t--max-weight miw\t\tMaximum integer edge weight\n"
            << "\t--subgraph-edge-length sel\tEdge length of subgraphs to extract in Kernel 3\n"
            << "\t--k4alpha k\t\t\tValue of K4Alpha in Kernel 4\n"
            << "\t--scale s\t\t\tSCALE parameter for the SSCA benchmark (sets n, m, and C)\n"
            << "\t--dot\t\t\t\tEmit a dot file containing the graph\n"
            << "\t--verify\t\t\tVerify result\n"
            << "\t--degree-dist\t\t\t Output degree distribution of graph\n"
            << "\t--no-distributed-graph\t\tOmit distributed graph tests\n";
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  using boost::graph::distributed::mpi_process_group;

#ifdef CSR
  typedef compressed_sparse_row_graph<directedS, VertexProperties, WeightedEdge, no_property,
                                      distributedS<mpi_process_group> > Graph;
#else
  typedef adjacency_list<vecS, 
                         distributedS<mpi_process_group, vecS>,
                         directedS, 
                         // Vertex properties
                         property<vertex_distance_t, int,
                         property<vertex_color_t, default_color_type> >,
                         // Edge properties
                         property<edge_weight_t, int> > Graph;
#endif

  typedef graph_traits<Graph>::vertices_size_type vertices_size_type;
  typedef graph_traits<Graph>::edges_size_type edges_size_type;

  RandomGenerator gen;
  
  // Default args
  vertices_size_type n = 100;
  edges_size_type m = 8*n;
  uint64_t seed = 1;
  int maxEdgeWeight = 100, 
      subGraphEdgeLength = 8,
      K4Alpha = 0.5;
  double a = 0.57, b = 0.19, c = 0.19, d = 0.05;
  bool emit_dot_file = false, verify = false, full_bc = true,
    distributed_graph = true, show_degree_dist = false,
    non_distributed_graph = true;

  mpi_process_group pg;

  if (argc == 1) {
    if (process_id(pg) == 0)
      usage();
    exit(-1);
  }

  // Parse args
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--vertices")
      n = boost::lexical_cast<vertices_size_type>( argv[i+1] );

    if (arg == "--seed")
      seed = boost::lexical_cast<uint64_t>( argv[i+1] );

    if (arg == "--full-bc")
      full_bc = (argv[i+1]== "true");

    if (arg == "--max-weight")
      maxEdgeWeight = boost::lexical_cast<int>( argv[i+1] );

    if (arg == "--subgraph-edge-length")
      subGraphEdgeLength = boost::lexical_cast<int>( argv[i+1] );
    
    if (arg == "--edges")
      m = boost::lexical_cast<edges_size_type>( argv[i+1] );

    if (arg == "--k4alpha")
      K4Alpha = boost::lexical_cast<int>( argv[i+1] );

    if (arg == "--dot")
      emit_dot_file = true;

    if (arg == "--verify")
      verify = true;

    if (arg == "--degree-dist")
      show_degree_dist = true;

    if (arg == "--no-distributed-graph")
      distributed_graph = false;

    if (arg == "--no-non-distributed-graph")
      non_distributed_graph = false;

    if (arg == "--scale") {
      vertices_size_type scale  = boost::lexical_cast<vertices_size_type>( argv[i+1] );
      maxEdgeWeight = n = vertices_size_type(floor(pow(2, scale)));
      m = 8 * n;
    }

    if (arg == "--help") {
      if (process_id(pg) == 0)
        usage();
      exit(-1);
    }
  }

  if (non_distributed_graph) {
    if (process_id(pg) == 0)
      std::cerr << "Non-Distributed Graph Tests\n";

    run_non_distributed_graph_tests(gen, pg, n, m, maxEdgeWeight, seed, K4Alpha, a, b, c, d, 
                                    subGraphEdgeLength, show_degree_dist, full_bc, verify);
  }

  if (distributed_graph) {
    if (process_id(pg) == 0)
      std::cerr << "Distributed Graph Tests\n";

    run_distributed_graph_tests(gen, pg, n, m, maxEdgeWeight, seed, K4Alpha, a, b, c, d, 
                                subGraphEdgeLength, show_degree_dist, emit_dot_file, 
                                full_bc, verify);
  }

  return 0;
}
