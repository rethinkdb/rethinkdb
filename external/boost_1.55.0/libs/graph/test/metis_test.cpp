// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/metis.hpp>
#include <fstream>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;

/* An undirected graph with distance values stored on the vertices. */
typedef adjacency_list<vecS, vecS, undirectedS,
                       property<vertex_distance_t, std::size_t> >
  Graph;

int main(int argc, char* argv[])
{
  // Parse command-line options
  const char* filename = "weighted_graph.gr";
  if (argc > 1) filename = argv[1];

  // Open the METIS input file
  std::ifstream in(filename);
  assert (in.good());
  graph::metis_reader reader(in);

  // Load the graph using the default distribution
  Graph g(reader.begin(), reader.end(),
          reader.num_vertices());

  return 0;
}
