// Copyright 2009 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nicholas Edmonds
//           Andrew Lumsdaine

#include <boost/random.hpp>
#include <boost/test/minimal.hpp>

#include <boost/graph/rmat_graph_generator.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/graph/ssca_graph_generator.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/mesh_graph_generator.hpp>

#include <boost/graph/adjacency_list.hpp>

using namespace boost;

int test_main(int argc, char** argv) {

  typedef rand48 RandomGenerator;

  typedef adjacency_list<vecS, vecS, directedS> Graph;

  RandomGenerator gen;

  size_t N = 100;
  size_t M = 1000;
  double p = 0.05;

  // Test Erdos-Renyi generator
  {
    erdos_renyi_iterator<RandomGenerator, Graph> start(gen, N, p);
    erdos_renyi_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  {
    sorted_erdos_renyi_iterator<RandomGenerator, Graph> start(gen, N, p);
    sorted_erdos_renyi_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  // Test Small World generator
  {
    small_world_iterator<RandomGenerator, Graph> start(gen, N, M, p);
    small_world_iterator<RandomGenerator, Graph> end;
    
    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  // Test SSCA generator
  {
    ssca_iterator<RandomGenerator, Graph> start(gen, N, 5, 0.5, 5, p);
    ssca_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  // Test Mesh generator
  {
    mesh_iterator<Graph> start(N, N);
    mesh_iterator<Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  // Test R-MAT generator
  double a = 0.57, b = 0.19, c = 0.19, d = 0.05;

  {
    rmat_iterator<RandomGenerator, Graph> start(gen, N, M, a, b, c, d);
    rmat_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  {
    unique_rmat_iterator<RandomGenerator, Graph> start(gen, N, M, a, b, c, d);
    unique_rmat_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  {
    sorted_unique_rmat_iterator<RandomGenerator, Graph> start(gen, N, M, a, b, c, d);
    sorted_unique_rmat_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  {
    sorted_unique_rmat_iterator<RandomGenerator, Graph> start(gen, N, M, a, b, c, d, true);
    sorted_unique_rmat_iterator<RandomGenerator, Graph> end;

    while (start != end) ++start;

    BOOST_CHECK(start == end);
  }

  return 0;
}
