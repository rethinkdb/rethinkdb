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
#include <boost/graph/distributed/boman_et_al_graph_coloring.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <iostream>
#include <boost/random.hpp>
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

void 
test_distributed_graph_coloring(int n, double p, int s,
                                int seed, bool emit_dot_file)
{
  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         undirectedS> Graph;

  typedef property_map<Graph, vertex_index_t>::type vertex_index_map;

  // Build a random number generator
  minstd_rand gen;
  gen.seed(seed);

  // Build a random graph
  Graph g(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, p),
          erdos_renyi_iterator<minstd_rand, Graph>(),
          n);

  // Set up color map
  std::vector<int> colors_vec(num_vertices(g));
  iterator_property_map<int*, vertex_index_map> color(&colors_vec[0],
                                                      get(vertex_index, g));

  // Run the graph coloring algorithm
  graph::boman_et_al_graph_coloring(g, color, s);

  if (process_id(g.process_group()) == 0) {
    graph::distributed::boman_et_al_graph_coloring_stats.print(std::cout);
  }

  if ( emit_dot_file ) {
    if ( process_id(g.process_group()) == 0 ) {
      for (int i = 0; i < n; ++i)
        get(color, vertex(i, g));
      synchronize(color);
    } else {
      synchronize(color);
    }
    
    write_graphviz("coloring.dot", g, paint_by_number(color));
  }
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  int n = 1000;
  double p = 0.01;
  int s = 100;
  int seed = 1;
  bool emit_dot_file = false;

  if (argc > 1) n             = lexical_cast<int>(argv[1]);
  if (argc > 2) p             = lexical_cast<double>(argv[2]);
  if (argc > 3) s             = lexical_cast<int>(argv[3]);
  if (argc > 4) seed          = lexical_cast<int>(argv[4]);
  if (argc > 5) emit_dot_file = lexical_cast<bool>(argv[5]);

  test_distributed_graph_coloring(n, p, s, seed, emit_dot_file);

  return 0;
}
