// Boost.Graph library isomorphism test

// Copyright (C) 2001 Douglas Gregor (gregod@cs.rpi.edu)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org
//
// Revision History:
//
// 29 Nov 2001    Jeremy Siek
//      Changed to use Boost.Random.
// 29 Nov 2001    Doug Gregor
//      Initial checkin.

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/isomorphism.hpp>
//#include "isomorphism-v3.hpp"
#include <boost/property_map/property_map.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace boost;


enum { a, b, c, d, e, f, g, h };
enum { _1, _2, _3, _4, _5, _6, _7, _8 };

void test_isomorphism() 
{
  typedef adjacency_list<vecS, vecS, bidirectionalS> GraphA;
  typedef adjacency_list<vecS, vecS, bidirectionalS> GraphB;

  char a_names[] = "abcdefgh";
  char b_names[] = "12345678";

  GraphA Ga(8);
  add_edge(a, d, Ga);
  add_edge(a, h, Ga);
  add_edge(b, c, Ga);
  add_edge(b, e, Ga);
  add_edge(c, f, Ga);
  add_edge(d, a, Ga);
  add_edge(d, h, Ga);
  add_edge(e, b, Ga);
  add_edge(f, b, Ga);
  add_edge(f, e, Ga);
  add_edge(g, d, Ga);
  add_edge(g, f, Ga);
  add_edge(h, c, Ga);
  add_edge(h, g, Ga);

  GraphB Gb(8);
  add_edge(_1, _6, Gb);
  add_edge(_2, _1, Gb);
  add_edge(_2, _5, Gb);
  add_edge(_3, _2, Gb);
  add_edge(_3, _4, Gb);
  add_edge(_4, _2, Gb);
  add_edge(_4, _3, Gb);
  add_edge(_5, _4, Gb);
  add_edge(_5, _6, Gb);
  add_edge(_6, _7, Gb);
  add_edge(_6, _8, Gb);
  add_edge(_7, _8, Gb);
  add_edge(_8, _1, Gb);
  add_edge(_8, _7, Gb);
  

  std::vector<std::size_t> in_degree_A(num_vertices(Ga));
  boost::detail::compute_in_degree(Ga, &in_degree_A[0]);

  std::vector<std::size_t> in_degree_B(num_vertices(Gb));
  boost::detail::compute_in_degree(Gb, &in_degree_B[0]);

  degree_vertex_invariant<std::size_t*, GraphA> 
    invariantA(&in_degree_A[0], Ga);
  degree_vertex_invariant<std::size_t*, GraphB> 
    invariantB(&in_degree_B[0], Gb);

  std::vector<graph_traits<GraphB>::vertex_descriptor> f(num_vertices(Ga));

  bool ret = isomorphism(Ga, Gb, &f[0], invariantA, invariantB, 
                         (invariantB.max)(),
                         get(vertex_index, Ga), get(vertex_index, Gb));
  assert(ret == true);

  for (std::size_t i = 0; i < num_vertices(Ga); ++i)
    std::cout << "f(" << a_names[i] << ")=" << b_names[f[i]] << std::endl;
  
  BOOST_TEST(verify_isomorphism(Ga, Gb, &f[0]));
}

int test_main(int, char* [])
{
  test_isomorphism();
  return boost::report_errors();
}
