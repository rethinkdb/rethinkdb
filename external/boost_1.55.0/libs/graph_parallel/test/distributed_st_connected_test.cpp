// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/st_connected.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/small_world_generator.hpp>
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

using namespace boost;
using boost::graph::distributed::mpi_process_group;

// Set up the vertex names
enum vertex_id_t { u, v, w, x, y, z, N };
char vertex_names[] = { 'u', 'v', 'w', 'x', 'y', 'z' };

void 
test_distributed_st_connected() {

  typedef adjacency_list<listS,
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         // Vertex properties
                         property<vertex_color_t, default_color_type> >
    Graph;

  // Specify the edges in the graph
  {
    typedef std::pair<int, int> E;
    E edge_array[] = { E(u, u), E(u, v), E(u, w), E(v, w), E(x, y), 
                       E(x, z), E(z, y), E(z, z) };
    Graph g(edge_array, edge_array + sizeof(edge_array) / sizeof(E), N);

    bool connected = st_connected(g, vertex(u, g), vertex(z, g), 
                                  get(vertex_color, g), get(vertex_owner, g));

    assert(!connected);
  }

  {
    typedef std::pair<int, int> E;
    E edge_array[] = { E(u, v), E(u, w), E(u, x), E(x, v), E(y, x),
                       E(v, y), E(w, y), E(w, z), E(z, z) };
    Graph g(edge_array, edge_array + sizeof(edge_array) / sizeof(E), N);

    bool connected = st_connected(g, vertex(u, g), vertex(z, g), 
                                  get(vertex_color, g), get(vertex_owner, g));

    assert(connected);
  }


}

int test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  test_distributed_st_connected();
  return 0;
}
