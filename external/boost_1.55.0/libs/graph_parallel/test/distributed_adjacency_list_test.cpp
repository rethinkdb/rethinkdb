// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/local_subgraph.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <iostream>
#include <cassert>
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

template<typename Graph>
struct never
{
  typedef typename graph_traits<Graph>::edge_descriptor argument_type;
  typedef bool result_type;

  result_type operator()(argument_type) { return false; }
};

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);

  mpi_process_group pg;
  parallel::block dist(pg, 20);

  typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>,
                         bidirectionalS> Graph1;
  typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>,
                         directedS> Graph2;
  typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>,
                         undirectedS> Graph3;

  if (num_processes(pg) > 20) return -1;

  if (process_id(pg) == 0) std::cout << "Graph 1------------------\n";

  std::cout << "Processor #" << process_id(pg) << ": "
            << dist.block_size(20) << " vertices." << std::endl;
  {
    Graph1 g1(20);

    //    if (process_id(pg) == num_processes(pg)() - 1)
    //      add_vertex(g1);

    graph_traits<Graph1>::vertex_iterator v, v_end;
    int counter = 0;
    for (boost::tie(v, v_end) = vertices(g1); v != v_end; ++v) {
      std::cout << "Processor #" << process_id(pg) << ": vertex " << ++counter
                << std::endl;

      out_edges(*v, g1);
      out_degree(*v, g1);
      in_edges(*v, g1);
      in_degree(*v, g1);

      graph_traits<Graph1>::vertex_descriptor other = *v;
      other.owner = (other.owner + 1) % num_processes(pg);
      other.local = 0;
      add_edge(*v, other, g1);

      std::cout << "Adding edge from processor " << process_id(pg)
                << " to processor " << other.owner << std::endl;
    }

    synchronize(g1);
    assert((std::size_t)std::distance(edges(g1).first, edges(g1).second) == num_vertices(g1));

    if (num_vertices(g1) >= 2) {
      graph_traits<Graph1>::vertex_iterator vi = vertices(g1).first;
      graph_traits<Graph1>::vertex_descriptor u = *vi++;
      graph_traits<Graph1>::vertex_descriptor v = *vi++;
      add_edge(u, v, g1);
      assert(out_degree(u, g1) == 2);
      assert(in_degree(v, g1) == 1);
    }

    int prior_processor = (process_id(pg) + num_processes(pg) - 1)
      % num_processes(pg);
    const int n = 20;
    std::size_t vertices_in_prior = (n / num_processes(pg))
      + (n % num_processes(pg) > prior_processor? 1 : 0);

    graph_traits<Graph1>::vertex_descriptor u = *vertices(g1).first;
    if (in_degree(u, g1) != vertices_in_prior) {
      std::cout << "Processor #" << process_id(pg) << ": " << in_degree(u, g1)
                << " vs. " << vertices_in_prior << std::endl;
    }
    assert(in_degree(u, g1) == vertices_in_prior);
    std::cout << "Processor #" << process_id(pg) << ": " << num_edges(g1)
              << " edges.\n";

    local_subgraph<Graph1> local_g1(g1);
    edges(local_g1);
    vertices(local_g1);
    if (num_vertices(local_g1) > 0) {
      out_edges(*vertices(local_g1).first, local_g1);
      in_edges(*vertices(local_g1).first, local_g1);
      adjacent_vertices(*vertices(local_g1).first, local_g1);
      if (false) {
        remove_out_edge_if(*vertices(g1).first, never<Graph1>(), g1);
        remove_in_edge_if(*vertices(g1).first, never<Graph1>(), g1);
        clear_out_edges(*vertices(g1).first, g1);
        clear_in_edges(*vertices(g1).first, g1);
        clear_vertex(*vertices(g1).first, g1);
        remove_vertex(*vertices(g1).first, g1);
      }
    }
    remove_edge_if(never<Graph1>(), g1);
  }


  if (process_id(pg) == 0) std::cout << "Graph 2------------------\n";

  {
    Graph2 g2(20);

    if (process_id(pg) == num_processes(pg) - 1)
      add_vertex(g2);

    graph_traits<Graph2>::vertex_iterator v, v_end;
    int counter = 0;
    for (boost::tie(v, v_end) = vertices(g2); v != v_end; ++v) {
      std::cout << "Processor #" << process_id(pg) << ": vertex " << ++counter
                << std::endl;

      out_edges(*v, g2);
    }

    synchronize(g2);

    if (num_vertices(g2) >= 2) {
      graph_traits<Graph2>::vertex_iterator vi = vertices(g2).first;
      graph_traits<Graph2>::vertex_descriptor u = *vi++;
      graph_traits<Graph2>::vertex_descriptor v = *vi++;
      add_edge(u, v, g2);
      assert(out_degree(u, g2) == 1);
      assert(*adjacent_vertices(u, g2).first == v);
      std::cout << "Processor #" << process_id(pg) << ": " << num_edges(g2)
                << " edges.\n";
      assert(std::distance(edges(g2).first, edges(g2).second) == 1);

    }
    synchronize(g2);

    local_subgraph<Graph2> local_g2(g2);
    edges(local_g2);
    vertices(local_g2);
    if (num_vertices(local_g2) > 0) {
      out_edges(*vertices(local_g2).first, local_g2);
      adjacent_vertices(*vertices(local_g2).first, local_g2);
      remove_out_edge_if(*vertices(g2).first, never<Graph2>(), g2);
      clear_out_edges(*vertices(g2).first, g2);
      remove_vertex(*vertices(g2).first, g2);
    }
    remove_edge_if(never<Graph2>(), g2);
  }

  if (process_id(pg) == 0) std::cout << "Graph 3------------------\n";

  {
    Graph3 g3(20);

    //    if (process_id(pg) == num_processes(pg) - 1)
    //      add_vertex(g3);

    graph_traits<Graph3>::vertex_iterator v, v_end;
    int counter = 0;
    for (boost::tie(v, v_end) = vertices(g3); v != v_end; ++v) {
      std::cout << "Processor #" << process_id(pg) << ": vertex " << ++counter
                << std::endl;

      out_edges(*v, g3);
      out_degree(*v, g3);
      in_edges(*v, g3);
      in_degree(*v, g3);
    }

    graph_traits<Graph3>::vertices_size_type added_edges = 0;
    if (num_vertices(g3) >= 2) {
      graph_traits<Graph3>::vertex_iterator vi = vertices(g3).first;
      graph_traits<Graph3>::vertex_descriptor u = *vi++;
      graph_traits<Graph3>::vertex_descriptor v = *vi++;
      add_edge(u, v, g3); ++added_edges;
      assert(out_degree(u, g3) == 1);
      assert(in_degree(u, g3) == 1);
      graph_traits<Graph3>::edge_descriptor e = *out_edges(u, g3).first;
      assert(source(e, g3) == u);
      assert(target(e, g3) == v);
      e = *in_edges(u, g3).first;
      assert(source(e, g3) == v);
      assert(target(e, g3) == u);
      assert(out_degree(v, g3) == 1);
      assert(in_degree(v, g3) == 1);
      e = *out_edges(v, g3).first;
      assert(source(e, g3) == v);
      assert(target(e, g3) == u);
      e = *in_edges(v, g3).first;
      assert(source(e, g3) == u);
      assert(target(e, g3) == v);

      assert(*adjacent_vertices(u, g3).first == v);
      assert(*adjacent_vertices(v, g3).first == u);
      std::cout << "Processor #" << process_id(pg) << ": " << num_edges(g3)
                << " edges.\n";
    }

    // Add some remote edges
    for (boost::tie(v, v_end) = vertices(g3); v != v_end; ++v) {
      graph_traits<Graph1>::vertex_descriptor other = *v;
      other.owner = (other.owner + 1) % num_processes(pg);
      other.local = 0;
      add_edge(*v, other, g3); ++added_edges;

      std::cout << "Adding edge from processor " << process_id(pg)
                << " to processor " << other.owner << std::endl;
    }

    synchronize(g3);
    assert(std::distance(edges(g3).first, edges(g3).second) == (ptrdiff_t)added_edges);
    assert(num_edges(g3) == added_edges);

    // Verify the remote edges
    if (num_vertices(g3) >= 2) {
      graph_traits<Graph3>::vertex_iterator vi = vertices(g3).first;
      graph_traits<Graph3>::vertex_descriptor u = *vi++;

      int prior_processor = (process_id(pg) + num_processes(pg) - 1)
        % num_processes(pg);
      const int n = 20;
      graph_traits<Graph3>::vertices_size_type vertices_in_prior = (n / num_processes(pg))
        + (n % num_processes(pg) > prior_processor? 1 : 0);
      if (in_degree(u, g3) != vertices_in_prior + 2) {
        std::cerr << "#" << process_id(pg) << ": " << in_degree(u, g3)
                  << " != " << vertices_in_prior + 2 << std::endl;
      }

      assert(in_degree(u, g3) == vertices_in_prior + 2);
      assert(out_degree(u, g3) == vertices_in_prior + 2);
    }

    local_subgraph<Graph3> local_g3(g3);
    edges(local_g3);
    vertices(local_g3);
    if (num_vertices(local_g3) > 0) {
      out_edges(*vertices(local_g3).first, local_g3);
      in_edges(*vertices(local_g3).first, local_g3);
      adjacent_vertices(*vertices(local_g3).first, local_g3);
      remove_out_edge_if(*vertices(g3).first, never<Graph3>(), g3);
      remove_in_edge_if(*vertices(g3).first, never<Graph3>(), g3);
      if (false) {
        // Removing an edge from two processes concurrently is not yet
        // supported.
        clear_vertex(*vertices(g3).first, g3);
        remove_vertex(*vertices(g3).first, g3);
      }
    }
    remove_edge_if(never<Graph3>(), g3);
  }

  return 0;
}
