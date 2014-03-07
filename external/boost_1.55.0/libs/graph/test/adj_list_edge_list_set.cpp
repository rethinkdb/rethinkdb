// Copyright 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

// Make sure adjacency_list works with EdgeListS=setS
#include <boost/graph/adjacency_list.hpp>
#include <boost/test/minimal.hpp>

using namespace boost;

typedef adjacency_list<vecS, vecS, undirectedS, no_property, no_property,
                       no_property, setS> GraphType;

int test_main(int,char*[])
{
  GraphType g(10);
  add_vertex(g);
  add_edge(0, 5, g);
  return 0;
}
