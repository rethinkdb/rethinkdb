// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

// A test of the distributed compressed sparse row graph type
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/breadth_first_search.hpp>
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

using namespace boost;
using boost::graph::distributed::mpi_process_group;

void concept_checks()
{
  typedef compressed_sparse_row_graph<directedS, no_property, no_property, no_property, 
                                      distributedS<mpi_process_group> >
    Digraph;
  typedef graph_traits<Digraph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Digraph>::edge_descriptor edge_descriptor;

  function_requires< GraphConcept<Digraph> >();
  function_requires< IncidenceGraphConcept<Digraph> >();
  function_requires< AdjacencyGraphConcept<Digraph> >();

  function_requires< DistributedVertexListGraphConcept<Digraph> >();
  function_requires< DistributedEdgeListGraphConcept<Digraph> >();

  function_requires<
    ReadablePropertyGraphConcept<Digraph, vertex_descriptor, vertex_global_t> 
    >();
  function_requires<
    ReadablePropertyGraphConcept<Digraph, vertex_descriptor, vertex_owner_t>
    >();
  function_requires<
    ReadablePropertyGraphConcept<Digraph, vertex_descriptor, vertex_local_t>
    >();
  function_requires<
    ReadablePropertyGraphConcept<Digraph, vertex_descriptor, vertex_index_t>
    >();

  function_requires<
    ReadablePropertyGraphConcept<Digraph, edge_descriptor, edge_global_t>
    >();

  // DPG TBD: edge_owner, edge_local property maps

  function_requires<
    ReadablePropertyGraphConcept<Digraph, edge_descriptor, edge_index_t>
    >();

  // Check default construction
  Digraph g;
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  concept_checks();

  typedef compressed_sparse_row_graph<directedS, no_property, no_property, no_property, 
                                      distributedS<mpi_process_group> >
    Digraph;

  // Build an Erdos-Renyi graph to test with
  typedef sorted_erdos_renyi_iterator<minstd_rand, Digraph> ERIter;

  int n = 40;
  double prob = 0.1;

  minstd_rand gen;
  Digraph g(ERIter(gen, n, prob), ERIter(), n);

  breadth_first_search(g, vertex(0, g), visitor(bfs_visitor<>()));

  std::ofstream out("dcsr.dot");
  write_graphviz(out, g);
  return 0;
}
