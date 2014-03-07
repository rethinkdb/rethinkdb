// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/distributed/connected_components_parallel_search.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/distributed/graphviz.hpp>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <boost/random.hpp>
#include <boost/test/minimal.hpp>

#include <boost/graph/distributed/compressed_sparse_row_graph.hpp>
#include <boost/graph/rmat_graph_generator.hpp>

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

template<typename T>
class map_lt
{
public:
  bool operator()() const { return false; }
  bool operator()(T x, T y) const { return (owner(x) < owner(y) || (owner(x) == owner(y) && local(x) < local(y))); }
};

void 
test_distributed_connected_components(int n, double _p, bool verify, bool emit_dot_file, int seed, bool parallel_search)
{
//   typedef adjacency_list<listS, 
//                          distributedS<mpi_process_group, vecS>,
//                          undirectedS> Graph;

  typedef compressed_sparse_row_graph<directedS, no_property, no_property, no_property,
                                      distributedS<mpi_process_group> > Graph;

  minstd_rand gen;

  gen.seed(seed);

  mpi_process_group pg;
  parallel::variant_distribution<mpi_process_group> distrib 
    = parallel::block(pg, n);

  minstd_rand dist_gen;
#if 0
  if (false) {
    distrib = parallel::random_distribution(pg, dist_gen, n);
  } else if (true) {
    distrib = parallel::oned_block_cyclic(pg, 13);
  }
#endif

//   Graph g(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, _p/2),
//           erdos_renyi_iterator<minstd_rand, Graph>(),
//           n, pg, distrib);

  int m = int(n * n * _p/2);
  double a = 0.57, b = 0.19, c = 0.19, d = 0.05;

  // Last boolean parameter makes R-MAT bidirectional
  Graph g(sorted_unique_rmat_iterator<minstd_rand, Graph>(gen, n, m, a, b, c, d, true),
          sorted_unique_rmat_iterator<minstd_rand, Graph>(),
          n, pg, distrib);

  synchronize(g);

  std::vector<int> local_components_vec(num_vertices(g));
  typedef iterator_property_map<std::vector<int>::iterator, property_map<Graph, vertex_index_t>::type> ComponentMap;
  ComponentMap component(local_components_vec.begin(), get(vertex_index, g));

  int num_components = 0;

  time_type start = get_time();
  if (parallel_search) {
    num_components = connected_components_ps(g, component);
  } else {
    num_components = connected_components(g, component);
  }
  time_type end = get_time();
  if (process_id(g.process_group()) == 0) 
    std::cerr << "Connected Components time = " << print_time(end - start) << " seconds.\n"
              << num_components << " components identified\n";

  if ( verify )
    {
      if ( process_id(g.process_group()) == 0 )
        {
          component.set_max_ghost_cells(0);
          for (int i = 0; i < n; ++i)
            get(component, vertex(i, g));
          synchronize(component);

          // Check against the sequential version
          typedef adjacency_list<listS, 
            vecS,
            undirectedS,
            // Vertex properties
            no_property,
            // Edge properties
            no_property > Graph2;
          
          gen.seed(seed);
          
//        Graph2 g2(erdos_renyi_iterator<minstd_rand, Graph>(gen, n, _p/2),
//                  erdos_renyi_iterator<minstd_rand, Graph>(),
//                  n);

          Graph2 g2( sorted_unique_rmat_iterator<minstd_rand, Graph>(gen, n, m, a, b, c, d, true),
                     sorted_unique_rmat_iterator<minstd_rand, Graph>(),
                     n);
          
          std::vector<int> component2 (n);
          int tmp;
          tmp = connected_components(g2, make_iterator_property_map(component2.begin(), get(vertex_index, g2)));
          std::cerr << "Verifier found " << tmp << " components" << std::endl;
          
          // Make sure components and component2 match
          std::map<int, int> c2c;
          int i;
          // This fails if there are more components in 'component' than
          // 'component2' because multiple components in 'component' may 
          // legitimately map to the same component number in 'component2'.
          // We can either test the mapping in the opposite direction or
          // just assert that the numbers of components found by both 
          // algorithms is the same
          for ( i = 0; i < n; i++ )
            if ( c2c.find( get(component, vertex(i, g)) ) == c2c.end() )
              c2c[get(component, vertex(i, g))] = component2[i];
            else
              if ( c2c[get(component, vertex(i, g))] != component2[i] )
                break;
          
          if ( i < n || num_components != tmp) {
            printf("Unable to verify CC result...\n");
          } else
            printf("Passed verification... %i connected components\n", 
                   (int)c2c.size());
        }
      else 
        {
          synchronize(component);
        }
      if ( emit_dot_file )
        write_graphviz("cc.dot", g, paint_by_number(component));
    }
}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);

  if ( argc < 6 ) {
    test_distributed_connected_components(10000, 0.001, true, false, 1, false);
    test_distributed_connected_components(10000, 0.001, true, false, 1, true);
  }
  else
    test_distributed_connected_components
      (atoi(argv[1]), atof(argv[2]), 
       argv[3]==std::string("true"), argv[4]==std::string("true"),
       argc == 6? 1 : atoi(argv[6]),
       argv[5]==std::string("true"));

  return 0;
}
