//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#error The vector_as_graph.hpp header requires partial specialization
#endif

#include <vector>
#include <list>
#include <iostream> // needed by graph_utility. -Jeremy
#include <boost/graph/vector_as_graph.hpp>
#include <boost/graph/graph_utility.hpp>

int
main()
{
  enum
  { r, s, t, u, v, w, x, y, N };
  char name[] = "rstuvwxy";
  typedef std::vector < std::list < int > > Graph;
  Graph g(N);
  g[r].push_back(v);
  g[s].push_back(r);
  g[s].push_back(r);
  g[s].push_back(w);
  g[t].push_back(x);
  g[u].push_back(t);
  g[w].push_back(t);
  g[w].push_back(x);
  g[x].push_back(y);
  g[y].push_back(u);
  boost::print_graph(g, name);
  return 0;
}
