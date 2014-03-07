// Copyright (C) 2005, 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/random/linear_congruential.hpp>
#include <iostream>
#include <boost/property_map/property_map_iterator.hpp>
#include <boost/graph/distributed/dehne_gotz_min_spanning_tree.hpp>
#include <boost/graph/distributed/vertex_list_adaptor.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/distributed/graphviz.hpp>
#include <sstream>
#include <string>
#include <boost/graph/iteration_macros.hpp>
#include <boost/test/minimal.hpp>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;
using boost::graph::distributed::mpi_process_group;

/****************************************************************************
 * Edge weight generator iterator                                           *
 ****************************************************************************/
template<typename F>
class generator_iterator
{
public:
  typedef std::input_iterator_tag iterator_category;
  typedef typename F::result_type value_type;
  typedef const value_type&       reference;
  typedef const value_type*       pointer;
  typedef void                    difference_type;

  explicit generator_iterator(const F& f = F()) : f(f) { value = this->f(); }

  reference operator*() const  { return value; }
  pointer   operator->() const { return &value; }

  generator_iterator& operator++()
  {
    value = f();
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
  value_type value;
};

template<typename F>
inline generator_iterator<F> make_generator_iterator(const F& f)
{ return generator_iterator<F>(f); }

typedef minstd_rand RandomGenerator;

template<typename Graph>
double get_mst_weight (const Graph& g)
{
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typename property_map<Graph, edge_weight_t>::const_type weight 
    = get(edge_weight, g);

  // Boruvka then merge test
  std::vector<edge_descriptor> results;
  graph::boruvka_then_merge(make_vertex_list_adaptor(g), weight,
                            std::back_inserter(results));
  if (process_id(g.process_group()) == 0)
    return accumulate(make_property_map_iterator(weight, results.begin()),
                      make_property_map_iterator(weight, results.end()),
                      0.0);
  else
    return 0.0;
}

template<typename Graph>
void test_redistribution(int n, double p, int iterations, bool debug_output)
{
  RandomGenerator gen;
  Graph g(erdos_renyi_iterator<RandomGenerator, Graph>(gen, n, p),
          erdos_renyi_iterator<RandomGenerator, Graph>(),
          make_generator_iterator(uniform_01<RandomGenerator>(gen)),
          n);

  int iter = 0;
  mpi_process_group pg = g.process_group();

  // Set the names of the vertices to be the global index in the
  // initial distribution. Then when we are debugging we'll be able to
  // see how vertices have moved.
  {
    typedef typename property_map<Graph, vertex_index_t>::type VertexIndexMap;
    typedef typename property_map<Graph, vertex_global_t>::type VertexGlobalMap;
    typename property_map<Graph, vertex_name_t>::type name_map 
      = get(vertex_name, g);

    parallel::global_index_map<VertexIndexMap, VertexGlobalMap> 
      global_index(g.process_group(), num_vertices(g), 
                   get(vertex_index, g), get(vertex_global, g));
    BGL_FORALL_VERTICES_T(v, g, Graph)
      put(name_map, v, get(global_index, v));
  }

  if (debug_output)
    write_graphviz("redist-0.dot", g,
                   make_label_writer(get(vertex_name, g)),
                   make_label_writer(get(edge_weight, g)));

  double mst_weight = get_mst_weight(g);
  if (process_id(pg) == 0)
    std::cout << "MST weight = " << mst_weight << std::endl;

  RandomGenerator nonsync_gen(process_id(pg) + gen());
  while (++iter <= iterations) {
    typename property_map<Graph, vertex_rank_t>::type to_processor_map = 
      get(vertex_rank, g);

    // Randomly assign a new distribution
    typename graph_traits<Graph>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
      put(to_processor_map, *vi, gen() % num_processes(pg));

    if (process_id(pg) == 0)
      std::cout << "Redistributing...";
    // Perform the actual redistribution
    g.redistribute(to_processor_map);

    if (process_id(pg) == 0)
      std::cout << " done." << std::endl;

    if (debug_output) {
      std::ostringstream out;
      out << "redist-" << iter << ".dot";
      write_graphviz(out.str().c_str(), g,
                     make_label_writer(get(vertex_name, g)),
                     make_label_writer(get(edge_weight, g)));
    }

    // Check that the MST weight is unchanged
    double new_mst_weight = get_mst_weight(g);
    if (process_id(pg) == 0) {
      std::cout << "MST weight = " << new_mst_weight << std::endl;
      if (std::fabs(new_mst_weight - mst_weight) > 0.0001)
        communicator(pg).abort(-1);    }
  }
}

int test_main(int argc, char** argv)
{
  int n = 1000;
  double p = 3e-3;
  int iterations = 5;
  bool debug_output = false;

  boost::mpi::environment env(argc, argv);

  if (argc > 1) n = lexical_cast<int>(argv[1]);
  if (argc > 2) p = lexical_cast<double>(argv[2]);
  if (argc > 3) iterations = lexical_cast<int>(argv[3]);
  if (argc > 4) debug_output = true;

  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         // Vertex properties
                         property<vertex_name_t, std::size_t,
                           property<vertex_rank_t, int> >,
                         // Edge properties
                         property<edge_weight_t, double> > UnstableUDGraph;
  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, listS>,
                         undirectedS,
                         // Vertex properties
                         property<vertex_name_t, std::size_t,
                           property<vertex_rank_t, int,
                              property<vertex_index_t, std::size_t> > >,
                         // Edge properties
                         property<edge_weight_t, double> > StableUDGraph;

  test_redistribution<UnstableUDGraph>(n, p, iterations, debug_output);
  test_redistribution<StableUDGraph>(n, p, iterations, debug_output);

  return 0;
}
