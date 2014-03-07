// Copyright 2004 The Trustees of Indiana University.

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

void test_bidirectional_graph()
{
  typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>,
                         bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor Vertex;

  typedef std::pair<std::size_t, std::size_t> E;
  E edges[3] = { E(0,3), E(1,4), E(2,5) };
  
  Graph g(&edges[0], &edges[0] + 3, 6);
  assert(num_processes(g.process_group()) == 2);

  if (process_id(g.process_group()) == 0)
    std::cerr << "Testing bidirectional graph edge removal...";
  synchronize(g.process_group());

  graph_traits<Graph>::vertex_iterator vi = vertices(g).first;
  Vertex u = *vi++;
  Vertex v = *vi++;
  Vertex w = *vi++;
  BOOST_CHECK(vi == vertices(g).second);
  
  BOOST_CHECK((process_id(g.process_group()) == 0 && out_degree(u, g) == 1)
             || (process_id(g.process_group()) == 1 && in_degree(u, g) == 1));
  
  BOOST_CHECK((process_id(g.process_group()) == 0 && out_degree(v, g) == 1)
             || (process_id(g.process_group()) == 1 && in_degree(v, g) == 1));

  BOOST_CHECK((process_id(g.process_group()) == 0 && out_degree(w, g) == 1)
             || (process_id(g.process_group()) == 1 && in_degree(w, g) == 1));

  // Remove edges
  if (process_id(g.process_group()) == 0) {
    remove_edge(*out_edges(u, g).first, g);
    remove_edge(*out_edges(w, g).first, g);
  } else {
    remove_edge(*in_edges(v, g).first, g);
    remove_edge(*in_edges(w, g).first, g);
  }
  
  synchronize(g);

  BOOST_CHECK(num_edges(g) == 0);
  BOOST_CHECK(out_degree(u, g) == 0);
  BOOST_CHECK(out_degree(v, g) == 0);
  BOOST_CHECK(out_degree(w, g) == 0);
  BOOST_CHECK(in_degree(u, g) == 0);
  BOOST_CHECK(in_degree(v, g) == 0);
  BOOST_CHECK(in_degree(w, g) == 0);

  if (process_id(g.process_group()) == 0) std::cerr << "OK.\n";
}

void test_undirected_graph()
{
  typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>,
                         undirectedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor Vertex;
  typedef graph_traits<Graph>::edge_descriptor Edge;

  typedef std::pair<std::size_t, std::size_t> E;
  E the_edges[3] = { E(0,3), E(1,4), E(2,5) };
  
  Graph g(&the_edges[0], &the_edges[0] + 3, 6);
  assert(num_processes(g.process_group()) == 2);

  if (process_id(g.process_group()) == 0)
    std::cerr << "Testing undirected graph edge removal...";
  synchronize(g.process_group());

  graph_traits<Graph>::vertex_iterator vi = vertices(g).first;
  Vertex u = *vi++;
  Vertex v = *vi++;
  Vertex w = *vi++;
  BOOST_CHECK(vi == vertices(g).second);  
  BOOST_CHECK(degree(u, g) == 1);
  BOOST_CHECK(degree(v, g) == 1);
  BOOST_CHECK(degree(w, g) == 1);
  
  // Remove edges
  if (process_id(g.process_group()) == 0) {
    BOOST_CHECK(num_edges(g) == 3);
    remove_edge(*out_edges(u, g).first, g);
    remove_edge(*out_edges(w, g).first, g);
  } else {
    BOOST_CHECK(num_edges(g) == 0);
    remove_edge(*in_edges(v, g).first, g);
    remove_edge(*in_edges(w, g).first, g);
  }
  
  synchronize(g);

  if (num_edges(g) > 0) {
    Edge e = *edges(g).first;
    std::cerr << "#" << process_id(g.process_group()) << ": extra edge! ("
              << local(source(e, g)) << "@" << owner(source(e, g))
              << ", " 
              << local(target(e, g)) << "@" << owner(target(e, g))
              << ")\n";
  }

  BOOST_CHECK(num_edges(g) == 0);
  BOOST_CHECK(degree(u, g) == 0);
  BOOST_CHECK(degree(v, g) == 0);
  BOOST_CHECK(degree(w, g) == 0);
  if (process_id(g.process_group()) == 0) std::cerr << "OK.\n";
}

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);
  test_bidirectional_graph();
  test_undirected_graph();
  return 0;
}
