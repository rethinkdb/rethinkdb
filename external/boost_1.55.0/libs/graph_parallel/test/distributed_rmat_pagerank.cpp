// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Brian Barrett
//           Nick Edmonds
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/distributed/page_rank.hpp>
#include <boost/graph/rmat_graph_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/vector_property_map.hpp>

#include <iostream>
#include <cstdlib>
#include <iomanip>

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

void
test_filtered_rmat_pagerank(int n, int m, double a, double b, double c, double d, int iters)
{
  mpi_process_group pg;
  std::size_t id = process_id(pg);

  if (id == 0) printf("INFO: Params: n=%d, m=%d, a=%f, b=%f, c=%f, d=%f, iters=%d.\n",
                      n, m, a, b, c, d, iters);

  typedef parallel::variant_distribution<mpi_process_group> Distribution;
  Distribution distrib = parallel::block(pg, n);

  typedef adjacency_list<vecS, 
        distributedS<mpi_process_group, vecS>,
        bidirectionalS> Graph;

  typedef scalable_rmat_iterator<mpi_process_group, Distribution, rand48, Graph>
    RMATIter;

  if (id == 0) printf("INFO: Generating graph.\n");

  rand48 gen;
  Graph g(RMATIter(pg, distrib, gen, n, m, a, b, c, d, true), 
          RMATIter(), n, pg, distrib);

  synchronize(g);

  if (id == 0) printf("INFO: Starting PageRank.\n");

  std::vector<double> ranks(num_vertices(g));

  time_type start = get_time();
  page_rank(g, make_iterator_property_map(ranks.begin(), get(boost::vertex_index, g)),
            graph::n_iterations(iters), 0.85, n);
  time_type end = get_time();

  if (process_id(g.process_group()) == 0) {
      std::cout << "INFO: Test Complete. time = " << 
          print_time(end - start) << "s." << std::endl;
      printf("RESULT: %d %d %d %f %f %f %f %f\n", 
             num_processes(pg), n, m, a, b, c, d, end - start);
  }

}

int
main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  if (argc < 8) 
    test_filtered_rmat_pagerank(40, 200, 0.58, 0.19, 0.19, 0.04, 10);
  else
    test_filtered_rmat_pagerank(atoi(argv[1]), atoi(argv[2]), atof(argv[3]), 
                                atof(argv[4]), atof(argv[5]), atof(argv[6]),
                                atoi(argv[7]));

  return 0;
}
