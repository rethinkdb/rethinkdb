// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/page_rank.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/test/minimal.hpp>
#include <vector>
#include <iostream>
#include <stdlib.h>

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

bool close_to(double x, double y)
{
  double diff = x - y;
  if (diff < 0) diff = -diff;
  double base = (y == 0? x : y);
  if (base != 0) return diff / base < 0.01;
  else return true;
}

// Make convenient labels for the vertices
enum vertex_id_t { A, B, C, D, N };

void test_distributed_page_rank(int iterations)
{
  using namespace boost::graph;

  // create a typedef for the Graph type
  typedef adjacency_list<vecS, 
                         distributedS<mpi_process_group, vecS>,
                         bidirectionalS 
                         > Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  // writing out the edges in the graph
  typedef std::pair<int, int> Edge;
  Edge edge_array[] =
    { Edge(A,B), Edge(A,C), Edge(B,C), Edge(C,A), Edge(D,C) };
  const int num_edges = sizeof(edge_array)/sizeof(edge_array[0]);

  // declare a graph object
  Graph g(edge_array, edge_array + num_edges, N);

  std::vector<double> ranks(num_vertices(g));

  page_rank(g,
            make_iterator_property_map(ranks.begin(),
                                       get(boost::vertex_index, g)),
            n_iterations(iterations), 0.85, N);
  
  double local_sum = 0.0;
  for(unsigned int i = 0; i < num_vertices(g); ++i) {
    std::cout << (char)('A' + g.distribution().global(i)) << " = " 
              << ranks[i] << std::endl;
    local_sum += ranks[i];
  }
  double sum=0.;
  boost::mpi::reduce(communicator(g.process_group()),
                     local_sum, sum, std::plus<double>(), 0);
  if (process_id(g.process_group()) == 0) {
    std::cout << "Sum = " << sum << "\n\n";
    BOOST_CHECK(close_to(sum, 4)); // 1 when alpha=0
  }

  //   double expected_ranks0[N] = {0.400009, 0.199993, 0.399998, 0.0};
  double expected_ranks[N] = {1.49011, 0.783296, 1.5766, 0.15};
  for (int i = 0; i < N; ++i) {
    vertex_descriptor v = vertex(i, g);
    if (v != Graph::null_vertex()
        && owner(v) == process_id(g.process_group())) {
      BOOST_CHECK(close_to(ranks[local(v)], expected_ranks[i]));
    }
  }
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  int iterations = 50;
  if (argc > 1) {
    iterations = atoi(argv[1]);
  }

  test_distributed_page_rank(iterations);

  return 0;
}
