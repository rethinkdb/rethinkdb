// Copyright (C) 2006 The Trustees of Indiana University.

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

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/distributed/betweenness_centrality.hpp>
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

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

#ifdef CSR
  typedef compressed_sparse_row_graph<directedS, no_property, WeightedEdge, 
                                      no_property, distributedS<mpi_process_group> >
    Graph;
  typedef compressed_sparse_row_graph<directedS, no_property, WeightedEdge>
    seqGraph;
#else
  typedef adjacency_list<vecS, 
                         distributedS<mpi_process_group, vecS>,
                         directedS,
                         no_property,
                         property<edge_weight_t, int> > Graph;

  typedef adjacency_list<vecS, vecS, directedS, no_property, 
                         property<edge_weight_t, int> > seqGraph;
#endif


  typedef sorted_erdos_renyi_iterator<minstd_rand, Graph> ERIter;

  int n = 100;
  double prob = 0.1;
  int C = 3;

  minstd_rand gen;

  gen.seed(1); 

  // NOTE: Weighted betweenness centrality only works with non-zero edge weights

  // Build graphs
  Graph g(edges_are_sorted, ERIter(gen, n, prob), ERIter(), 
          make_generator_iterator(gen, uniform_int<int>(1, C)),
          n);

  gen.seed(1);  // Re-seed PRNG so we get the same graph

  seqGraph sg(
#ifdef CSR
              edges_are_sorted,
#endif
              ERIter(gen, n, prob), ERIter(), 
              make_generator_iterator(gen, uniform_int<int>(1, C)),
              n);

  // Test Betweenness Centrality
  typedef property_map<Graph, vertex_index_t>::const_type IndexMap;
  typedef iterator_property_map<std::vector<int>::iterator, IndexMap>
    CentralityMap;

  std::vector<int> centralityS(num_vertices(g), 0);
  CentralityMap centrality(centralityS.begin(), get(vertex_index, g));

  brandes_betweenness_centrality(g, centrality);

  std::vector<int> weightedCentralityS(num_vertices(g), 0);
  CentralityMap weightedCentrality(weightedCentralityS.begin(), get(vertex_index, g));

  brandes_betweenness_centrality(g, centrality_map(weightedCentrality).
#ifdef CSR
                                 weight_map(get(&WeightedEdge::weight, g)));
#else
                                 weight_map(get(edge_weight, g)));
#endif

  int g_cpd = central_point_dominance(g, centrality);

  //
  //  Non-distributed (embarassingly parallel) Betweenness Centrality
  //
  typedef boost::graph::parallel::process_group_type<Graph>::type 
    process_group_type;

  process_group_type pg = process_group(g);

  process_group_type::process_id_type id = process_id(pg);
  process_group_type::process_size_type p = num_processes(pg);    

  typedef property_map<seqGraph, vertex_index_t>::const_type seqIndexMap;
  typedef iterator_property_map<std::vector<int>::iterator, seqIndexMap> seqCentralityMap;

  std::vector<int> nonDistributedCentralityS(num_vertices(sg), 0);
  seqCentralityMap nonDistributedCentrality(nonDistributedCentralityS.begin(), get(vertex_index, sg));

  std::vector<int> nonDistributedWeightedCentralityS(num_vertices(sg), 0);
  seqCentralityMap nonDistributedWeightedCentrality(nonDistributedWeightedCentralityS.begin(), 
                                                    get(vertex_index, sg));

  non_distributed_brandes_betweenness_centrality(pg, sg, nonDistributedCentrality);

  non_distributed_brandes_betweenness_centrality(pg, sg, 
                                                 centrality_map(nonDistributedWeightedCentrality).
#ifdef CSR
                                                 weight_map(get(&WeightedEdge::weight, sg)));
#else
                                                 weight_map(get(edge_weight, sg)));
#endif

  //
  // Verify
  //
  std::vector<int> seqCentralityS(num_vertices(sg), 0);
  seqCentralityMap seqCentrality(seqCentralityS.begin(), get(vertex_index, sg));

  std::vector<int> seqWeightedCentralityS(num_vertices(sg), 0);
  seqCentralityMap seqWeightedCentrality(seqWeightedCentralityS.begin(), get(vertex_index, sg));

  brandes_betweenness_centrality(sg, seqCentrality);

  brandes_betweenness_centrality(sg, centrality_map(seqWeightedCentrality).
#ifdef CSR
                                 weight_map(get(&WeightedEdge::weight, sg)));
#else
                                 weight_map(get(edge_weight, sg)));
#endif

  int sg_cpd = central_point_dominance(sg, seqCentrality);

  // Verify exact betweenness centrality
  //
  // We're cheating when we map vertices in g to vertices in sg
  // since we know that g is using the block distribution here
  typedef property_map<Graph, vertex_owner_t>::const_type OwnerMap;
  typedef property_map<Graph, vertex_local_t>::const_type LocalMap;

  OwnerMap owner = get(vertex_owner, g);
  LocalMap local = get(vertex_local, g);

  bool passed = true;
  
  {
    typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
    vertex_iterator v, v_end;
    
    for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v) {
      if (get(centrality, *v) != seqCentralityS[(n/p) * get(owner, *v) + get(local, *v)]) {
        std::cerr << "  " << id << ": Error - centrality of " << get(local, *v) << "@" << get(owner, *v)
                  << " does not match the sequential result (" << get(centrality, *v) << " vs. " 
                  << seqCentralityS[(n/p) * get(owner, *v) + get(local, *v)] << ")\n";
        passed = false;
      }
      
      if (get(weightedCentrality, *v) != seqWeightedCentralityS[(n/p) * get(owner, *v) + get(local, *v)]) {
        std::cerr << "  " << id << ": Error - weighted centrality of " << get(local, *v) << "@" << get(owner, *v)
                  << " does not match the sequential result (" << get(weightedCentrality, *v) << " vs. " 
                  << seqWeightedCentralityS[(n/p) * get(owner, *v) + get(local, *v)] << ")\n";
        passed = false;
      }
    }
  }

  if (id == 0) {
    typedef graph_traits<seqGraph>::vertex_iterator vertex_iterator;
    vertex_iterator v, v_end;
  
    for (boost::tie(v, v_end) = vertices(sg); v != v_end; ++v) {
      if (get(seqCentrality, *v) != get(nonDistributedCentrality, *v)) {
        std::cerr << "  " << id << ": Error - non-distributed centrality of " << *v
                  << " does not match the sequential result (" << get(nonDistributedCentrality, *v) 
                  << " vs. " << get(seqCentrality, *v) << ")\n";
        passed = false;
      }

      if (get(seqWeightedCentrality, *v) != get(nonDistributedWeightedCentrality, *v)) {
        std::cerr << "  " << id << ": Error - non-distributed weighted centrality of " << *v
                  << " does not match the sequential result (" << get(nonDistributedWeightedCentrality, *v) 
                  << " vs. " << get(seqCentrality, *v) << ")\n";
        passed = false;
      }
    }
  }

  if (g_cpd != sg_cpd) {
    passed = false;
    std::cerr << "Central point dominance did not match the sequential result\n";
  }

  if (!passed)
    MPI_Abort(MPI_COMM_WORLD, -1);

  return 0;
}
