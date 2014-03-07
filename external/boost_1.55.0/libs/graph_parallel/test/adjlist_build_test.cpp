// Copyright (C) 2007  Douglas Gregor

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/test/minimal.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/lexical_cast.hpp>
#include <ctime>

using namespace boost;
using boost::graph::distributed::mpi_process_group;

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif


int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);

  int n = 10000;
  double p = 3e-3;
  int seed = std::time(0);
  int immediate_response_percent = 10;

  if (argc > 1) n = lexical_cast<int>(argv[1]);
  if (argc > 2) p = lexical_cast<double>(argv[2]);
  if (argc > 3) seed = lexical_cast<int>(argv[3]);

  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         bidirectionalS> Graph;

  mpi_process_group pg;
  int rank = process_id(pg);
  int numprocs = num_processes(pg);
  bool i_am_root = rank == 0;

  // broadcast the seed  
  broadcast(pg, seed, 0);

  // Random number generator
  minstd_rand gen;
  minstd_rand require_response_gen;

  if (i_am_root) {
    std::cout << "n = " << n << ", p = " << p << ", seed = " << seed
              << "\nBuilding graph with the iterator constructor... ";
    std::cout.flush();
  }

  // Build a graph using the iterator constructor, where each of the
  // processors has exactly the same information.
  gen.seed(seed);
  Graph g1(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, p),
           erdos_renyi_iterator<minstd_rand, Graph>(),
           n, pg, Graph::graph_property_type());
  // NGE: Grrr, the default graph property is needed to resolve an 
  //      ambiguous overload in the adjaceny list constructor

  // Build another, identical graph using add_edge
  if (i_am_root) {
    std::cout << "done.\nBuilding graph with add_edge from the root...";
    std::cout.flush();
  }

  gen.seed(seed);
  require_response_gen.seed(1);
  Graph g2(n, pg);
  if (i_am_root) {
    // The root will add all of the edges, some percentage of which
    // will require an immediate response from the owner of the edge.
    for (erdos_renyi_iterator<minstd_rand, Graph> first(gen, n, p), last;
         first != last; ++first) {
      Graph::lazy_add_edge lazy
        = add_edge(vertex(first->first, g2), vertex(first->second, g2), g2);

      if ((int)require_response_gen() % 100 < immediate_response_percent) {
        // Send out-of-band to require a response 
        std::pair<graph_traits<Graph>::edge_descriptor, bool> result(lazy);
        BOOST_CHECK(source(result.first, g2) == vertex(first->first, g2));
        BOOST_CHECK(target(result.first, g2) == vertex(first->second, g2));
      }
    }
  }

  if (i_am_root) {
    std::cout << "synchronizing...";
    std::cout.flush();
  }

  synchronize(g2);

  // Verify that the two graphs are indeed identical.
  if (i_am_root) {
    std::cout << "done.\nVerifying graphs...";
    std::cout.flush();
  }

  // Check the number of vertices
  if (num_vertices(g1) != num_vertices(g2)) {
    std::cerr << g1.processor() << ": g1 has " << num_vertices(g1) 
              << " vertices, g2 has " << num_vertices(g2) << " vertices.\n";
    communicator(pg).abort(-1);
  }

  // Check the number of edges
  if (num_edges(g1) != num_edges(g2)) {
    std::cerr << g1.processor() << ": g1 has " << num_edges(g1) 
              << " edges, g2 has " << num_edges(g2) << " edges.\n";
    communicator(pg).abort(-1);
  }

  // Check the in-degree and out-degree of each vertex
  graph_traits<Graph>::vertex_iterator vfirst1, vlast1, vfirst2, vlast2;
  boost::tie(vfirst1, vlast1) = vertices(g1);
  boost::tie(vfirst2, vlast2) = vertices(g2);
  for(; vfirst1 != vlast1 && vfirst2 != vlast2; ++vfirst1, ++vfirst2) {
    if (out_degree(*vfirst1, g1) != out_degree(*vfirst2, g2)) {
      std::cerr << g1.processor() << ": out-degree mismatch ("
                << out_degree(*vfirst1, g1) << " vs. " 
                << out_degree(*vfirst2, g2) << ").\n";
      communicator(pg).abort(-1);
    }

    if (in_degree(*vfirst1, g1) != in_degree(*vfirst2, g2)) {
      std::cerr << g1.processor() << ": in-degree mismatch (" 
                << in_degree(*vfirst1, g1) << " vs. " 
                << in_degree(*vfirst2, g2) << ").\n";
      communicator(pg).abort(-1);
    }
  }

  // TODO: Check the actual edge targets

  // Build another, identical graph using add_edge
  if (i_am_root) {
    std::cout << "done.\nBuilding graph with add_edge from everywhere...";
    std::cout.flush();
  }

  gen.seed(seed);
  require_response_gen.seed(1);
  Graph g3(n, pg);

  {
    // Each processor will take a chunk of incoming edges and add
    // them. Some percentage of the edge additions will require an
    // immediate response from the owner of the edge. This should
    // create a lot of traffic when building the graph, but should
    // produce a graph identical to the other graphs.
    typedef graph_traits<Graph>::edges_size_type edges_size_type;

    erdos_renyi_iterator<minstd_rand, Graph> first(gen, n, p);
    edges_size_type chunk_size = edges_size_type(p*n*n)/numprocs;
    edges_size_type start = chunk_size * rank;
    edges_size_type remaining_edges = 
      (rank < numprocs - 1? chunk_size 
       : edges_size_type(p*n*n) - start);

    // Spin the generator to the first edge we're responsible for
    for (; start; ++first, --start) ;

    for (; remaining_edges; --remaining_edges, ++first) {
      Graph::lazy_add_edge lazy
        = add_edge(vertex(first->first, g3), vertex(first->second, g3), g3);

      if ((int)require_response_gen() % 100 < immediate_response_percent) {
        // Send out-of-band to require a response 
        std::pair<graph_traits<Graph>::edge_descriptor, bool> result(lazy);
        BOOST_CHECK(source(result.first, g3) == vertex(first->first, g3));
        BOOST_CHECK(target(result.first, g3) == vertex(first->second, g3));
      }
    }
  }

  if (i_am_root) {
    std::cout << "synchronizing...";
    std::cout.flush();
  }

  synchronize(g3);

  // Verify that the two graphs are indeed identical.
  if (i_am_root) {
    std::cout << "done.\nVerifying graphs...";
    std::cout.flush();
  }

  // Check the number of vertices
  if (num_vertices(g1) != num_vertices(g3)) {
    std::cerr << g1.processor() << ": g1 has " << num_vertices(g1) 
              << " vertices, g3 has " << num_vertices(g3) << " vertices.\n";
    communicator(pg).abort(-1);
  }

  // Check the number of edges
  if (num_edges(g1) != num_edges(g3)) {
    std::cerr << g1.processor() << ": g1 has " << num_edges(g1) 
              << " edges, g3 has " << num_edges(g3) << " edges.\n";
    communicator(pg).abort(-1);
  }

  // Check the in-degree and out-degree of each vertex
  boost::tie(vfirst1, vlast1) = vertices(g1);
  boost::tie(vfirst2, vlast2) = vertices(g3);
  for(; vfirst1 != vlast1 && vfirst2 != vlast2; ++vfirst1, ++vfirst2) {
    if (out_degree(*vfirst1, g1) != out_degree(*vfirst2, g3)) {
      std::cerr << g1.processor() << ": out-degree mismatch ("
                << out_degree(*vfirst1, g1) << " vs. " 
                << out_degree(*vfirst2, g3) << ").\n";
      communicator(pg).abort(-1);
    }

    if (in_degree(*vfirst1, g1) != in_degree(*vfirst2, g3)) {
      std::cerr << g1.processor() << ": in-degree mismatch (" 
                << in_degree(*vfirst1, g1) << " vs. " 
                << in_degree(*vfirst2, g3) << ").\n";
      communicator(pg).abort(-1);
    }
  }

  // TODO: Check the actual edge targets

  if (i_am_root) {
    std::cout << "done.\n";
    std::cout.flush();
  }

  return 0;
}
