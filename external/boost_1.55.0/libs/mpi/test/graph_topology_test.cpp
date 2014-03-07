// Copyright (C) 2007 Trustees of Indiana University

// Authors: Douglas Gregor
//          Andrew Lumsdaine

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A test of the communicator that passes data around a ring and
// verifies that the same data makes it all the way. Should test all
// of the various kinds of data that can be sent (primitive types, POD
// types, serializable objects, etc.)
#include <boost/mpi/graph_communicator.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/test/minimal.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/isomorphism.hpp>
#include <algorithm> // for random_shuffle
#include <boost/serialization/vector.hpp>
#include <boost/mpi/collectives/broadcast.hpp>

using boost::mpi::communicator;
using boost::mpi::graph_communicator;
using namespace boost;

int test_main(int argc, char* argv[])
{
  boost::function_requires< IncidenceGraphConcept<graph_communicator> >();
  boost::function_requires< AdjacencyGraphConcept<graph_communicator> >();
  boost::function_requires< VertexListGraphConcept<graph_communicator> >();
  boost::function_requires< EdgeListGraphConcept<graph_communicator> >();

  double prob = 0.1;

  boost::mpi::environment env(argc, argv);

  communicator world;

  // Random number generator
  minstd_rand gen;

  // Build a random graph with as many vertices as there are processes
  typedef adjacency_list<listS, vecS, bidirectionalS> Graph;
  sorted_erdos_renyi_iterator<minstd_rand, Graph>
    first(gen, world.size(), prob), last;
  Graph graph(first, last, world.size());

  // Display the original graph
  if (world.rank() == 0) {
    std::cout << "Original, random graph:\n";
    BGL_FORALL_VERTICES(v, graph, Graph) {
      BGL_FORALL_OUTEDGES(v, e, graph, Graph) {
        std::cout << source(e, graph) << " -> " << target(e, graph) 
                  << std::endl;
      }
    }
  }

  // Create an arbitrary mapping from vertices to integers
  typedef property_map<Graph, vertex_index_t>::type GraphVertexIndexMap;
  std::vector<int> graph_alt_index_vec(num_vertices(graph));
  iterator_property_map<int*, GraphVertexIndexMap> 
    graph_alt_index(&graph_alt_index_vec[0], get(vertex_index, graph));

  // Rank 0 will populate the alternative index vector
  if (world.rank() == 0) {
    int index = 0;
    BGL_FORALL_VERTICES(v, graph, Graph)
      put(graph_alt_index, v, index++);

    std::random_shuffle(graph_alt_index_vec.begin(), graph_alt_index_vec.end());
  }
  broadcast(world, graph_alt_index_vec, 0);

  // Display the original graph with the remapping
  if (world.rank() == 0) {
    std::cout << "Original, random graph with remapped vertex numbers:\n";
    BGL_FORALL_VERTICES(v, graph, Graph) {
      BGL_FORALL_OUTEDGES(v, e, graph, Graph) {
        std::cout << get(graph_alt_index, source(e, graph)) << " -> " 
                  << get(graph_alt_index, target(e, graph)) << std::endl;
      }
    }
  }

  // Create a communicator with a topology equivalent to the graph
  graph_communicator graph_comm(world, graph, graph_alt_index, false);

  // The communicator's topology should have the same number of
  // vertices and edges and the original graph
  BOOST_CHECK((int)num_vertices(graph) == num_vertices(graph_comm));
  BOOST_CHECK((int)num_edges(graph) == num_edges(graph_comm));

  // Display the communicator graph
  if (graph_comm.rank() == 0) {
    std::cout << "Communicator graph:\n";
    BGL_FORALL_VERTICES(v, graph_comm, graph_communicator) {
      BGL_FORALL_OUTEDGES(v, e, graph_comm, graph_communicator) {
        std::cout << source(e, graph_comm) << " -> " << target(e, graph_comm) 
                  << std::endl;
      }
    }

    std::cout << "Communicator graph via edges():\n";
    BGL_FORALL_EDGES(e, graph_comm, graph_communicator)
      std::cout << source(e, graph_comm) << " -> " << target(e, graph_comm) 
                << std::endl;
  }
  (graph_comm.barrier)();

  // Verify the isomorphism
  if (graph_comm.rank() == 0)
    std::cout << "Verifying isomorphism..." << std::endl;
  BOOST_CHECK(verify_isomorphism(graph, graph_comm, graph_alt_index));

  return 0;
}
