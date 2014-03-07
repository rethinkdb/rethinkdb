// Copyright (C) 2005, 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#define PBGL_ACCOUNTING

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/delta_stepping_shortest_paths.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/random.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <iostream>
#include <iomanip>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

/****************************************************************************
 * Timing                                                                   *
 ****************************************************************************/
typedef double time_type;

inline time_type get_time()
{
  return MPI_Wtime();
}

std::string print_time(time_type t)
{
  std::ostringstream out;
  out << std::setiosflags(std::ios::fixed) << std::setprecision(2) << t;
  return out.str();
}

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
 * Verification                                                             *
 ****************************************************************************/
template <typename Graph, typename DistanceMap, typename WeightMap>
void
verify_shortest_paths(const Graph& g, DistanceMap distance, 
                      const WeightMap& weight) {

  distance.set_max_ghost_cells(0);

  BGL_FORALL_VERTICES_T(v, g, Graph) {
    BGL_FORALL_OUTEDGES_T(v, e, g, Graph) {
      get(distance, target(e, g));
    }
  }

  synchronize(process_group(g));

  BGL_FORALL_VERTICES_T(v, g, Graph) {
    BGL_FORALL_OUTEDGES_T(v, e, g, Graph) {
      assert(get(distance, target(e, g)) <= 
             get(distance, source(e, g)) + get(weight, e));
    }
  }
}

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

void 
test_distributed_shortest_paths(int n, double p, int c, int seed)
{
  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         VertexProperties,
                         WeightedEdge> Graph;

  typedef graph_traits<Graph>::vertices_size_type vertices_size_type;

  // Build a random number generator
  minstd_rand gen;
  gen.seed(seed);

  // Build a random graph
  Graph g(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, p),
          erdos_renyi_iterator<minstd_rand, Graph>(),
          make_generator_iterator(gen, uniform_int<int>(0, c)),
          n);

  uniform_int<vertices_size_type> rand_vertex(0, n);

  graph::distributed::delta_stepping_shortest_paths(g, 
                                                    vertex(rand_vertex(gen), g),
                                                    dummy_property_map(),
                                                    get(&VertexProperties::distance, g),
                                                    get(&WeightedEdge::weight, g));

  verify_shortest_paths(g, 
                        get(&VertexProperties::distance, g), 
                        get(&WeightedEdge::weight, g));
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  int n = 1000;
  double p = 0.01;
  int c = 100;
  int seed = 1;

  if (argc > 1) n             = lexical_cast<int>(argv[1]);
  if (argc > 2) p             = lexical_cast<double>(argv[2]);
  if (argc > 3) c             = lexical_cast<int>(argv[3]);
  if (argc > 4) seed          = lexical_cast<int>(argv[4]);

  test_distributed_shortest_paths(n, p, c, seed);

  return 0;
}
