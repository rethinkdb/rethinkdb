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
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/distributed/connected_components_parallel_search.hpp>
#include <boost/graph/distributed/connected_components.hpp>
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
test_filtered_rmat_cc(int n, int m, double a, double b, double c, double d)
{
  mpi_process_group pg;
  mpi_process_group::process_id_type id = process_id(pg);

  if (id == 0) printf("INFO: Params: n=%d, m=%d, a=%f, b=%f, c=%f, d=%f.\n",
                      n, m, a, b, c, d);

  parallel::variant_distribution<mpi_process_group> distrib 
    = parallel::block(pg, n);

  typedef adjacency_list<vecS, 
        distributedS<mpi_process_group, vecS>,
        undirectedS> Graph;

  typedef keep_local_edges<parallel::variant_distribution<mpi_process_group>,
                           mpi_process_group::process_id_type>
    EdgeFilter; 

  typedef unique_rmat_iterator<rand48, Graph, EdgeFilter>
    RMATIter;

  if (id == 0) printf("INFO: Generating graph.\n");

  rand48 gen;
  Graph g(RMATIter(gen, n, m, a, b, c, d, true, EdgeFilter(distrib, id)), 
          RMATIter(), n, pg, distrib);

  synchronize(g);

  if (id == 0) printf("INFO: Starting connected components.\n");

  std::vector<int> local_components_vec(num_vertices(g));
  typedef iterator_property_map<std::vector<int>::iterator, property_map<Graph, vertex_index_t>::type> ComponentMap;
  ComponentMap component(local_components_vec.begin(), get(vertex_index, g));

  time_type start = get_time();
  int num_components = connected_components(g, component);
  time_type end = get_time();

  if (process_id(g.process_group()) == 0) {
      std::cout << "INFO: Test Complete. components found = " << num_components
                << ", time = " << print_time(end - start) << "s." << std::endl;
      printf("RESULT: %d %d %d %f %f %f %f %f\n", 
             num_processes(pg), n, m, a, b, c, d, end - start);
  }

}

int
main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  if (argc < 7) 
    test_filtered_rmat_cc(40, 200, 0.58, 0.19, 0.19, 0.04);    
  else 
    test_filtered_rmat_cc(atoi(argv[1]), atoi(argv[2]),
                          atof(argv[3]), atof(argv[4]), 
                          atof(argv[5]), atof(argv[6]));

  return 0;
}
