// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Douglas Gregor
//           Andrew Lumsdaine

// SCC won't work with CSR currently due to the way the reverse graph 
// is constructed in the SCC algorithm

#include <boost/graph/use_mpi.hpp>

// #define CSR

#ifdef CSR
#  include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#else
#  include <boost/graph/distributed/adjacency_list.hpp>
#endif

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/distributed/graphviz.hpp>
#include <iostream>
#include <cstdlib>
#include <iomanip>
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

using namespace boost;
using boost::graph::distributed::mpi_process_group;

void 
test_distributed_strong_components(int n, double _p, bool verify, bool emit_dot_file, int seed)
{
#ifdef CSR
  typedef compressed_sparse_row_graph<directedS, no_property, no_property, no_property,
    distributedS<mpi_process_group> > Graph;
#else
  typedef adjacency_list<listS, 
                         distributedS<mpi_process_group, vecS>,
                         bidirectionalS > Graph;
#endif

  minstd_rand gen;

  gen.seed(seed);

  mpi_process_group pg;
  parallel::variant_distribution<mpi_process_group> distrib 
    = parallel::block(pg, n);

#ifdef CSR
  Graph g(sorted_erdos_renyi_iterator<minstd_rand, Graph>(gen, n, _p/2),
          sorted_erdos_renyi_iterator<minstd_rand, Graph>(),
          n, pg, distrib);
#else
  Graph g(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, _p/2),
          erdos_renyi_iterator<minstd_rand, Graph>(),
          n, pg, distrib);
#endif

  synchronize(g);

  std::vector<int> local_components_vec(num_vertices(g));
  typedef iterator_property_map<std::vector<int>::iterator, property_map<Graph, vertex_index_t>::type> ComponentMap;
  ComponentMap component(local_components_vec.begin(), get(vertex_index, g));

  int num_components = 0;

  time_type start = get_time();
  num_components = strong_components(g, component);
  time_type end = get_time();
  if (process_id(g.process_group()) == 0)
    std::cerr << "Erdos-Reyni graph:\n" << n << " Vertices   " << _p << " Edge probability  " 
              << num_processes(pg) << " Processors\n"
              << "Strong Components time = " << print_time(end - start) << " seconds.\n"
              << num_components << " components identified\n";
  

  if ( verify )
    {
      if ( process_id(g.process_group()) == 0 )
        {
          for (int i = 0; i < n; ++i)
            get(component, vertex(i, g));
          synchronize(component);

          // Check against the sequential version
          typedef adjacency_list<listS, vecS, directedS> Graph2;
          
          gen.seed(seed);
          
          Graph2 g2(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, _p/2),
                    erdos_renyi_iterator<minstd_rand, Graph>(),
                    n);

          std::vector<int> component2(n);
          int seq_num_components = strong_components(g2, make_iterator_property_map(component2.begin(), get(vertex_index, g2)));

          assert(num_components == seq_num_components);

          // Make sure components and component2 match
          std::map<int, int> c2c;
          int i;
          for ( i = 0; i < n; i++ )
            if ( c2c.find( get(component, vertex(i, g)) ) == c2c.end() )
              c2c[get(component, vertex(i, g))] = component2[i];
            else
              if ( c2c[get(component, vertex(i, g))] != component2[i] )
                break;
          
          if ( i < n )
            std::cerr << "Unable to verify SCC result...\n";
          else
            std::cerr << "Passed verification... " << seq_num_components << " strong components\n"; 
        }
      else 
        {
          synchronize(component);
        }
      if ( emit_dot_file )
        write_graphviz("scc.dot", g, paint_by_number(component));
    }
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  if (argc == 1)
    test_distributed_strong_components(10000, 0.0005, true, false, 1);
  else if ( argc < 5 )
    std::cerr << "usage: test_distributed_strong_components <int num_vertices> <double p> <bool verify?> <bool emit_dotfile?> [seed]\n";
  else
    test_distributed_strong_components
      (atoi(argv[1]), atof(argv[2]), 
       argv[3]==std::string("true"), argv[4]==std::string("true"),
       argc == 5? 1 : atoi(argv[5]));

  return 0;
}
