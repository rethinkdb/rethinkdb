//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace boost;

namespace std
{
  template < typename T >
    std::istream & operator >> (std::istream & in, std::pair < T, T > &p)
  {
    in >> p.first >> p.second;
    return in;
  }
}

typedef adjacency_list < listS, // Store out-edges of each vertex in a std::list
  vecS,                         // Store vertex set in a std::vector
  directedS                     // The file dependency graph is directed
> file_dep_graph;

typedef graph_traits < file_dep_graph >::vertex_descriptor vertex_t;
typedef graph_traits < file_dep_graph >::edge_descriptor edge_t;

bool
has_cycle_dfs(const file_dep_graph & g, vertex_t u,
              default_color_type * color)
{
  color[u] = gray_color;
  graph_traits < file_dep_graph >::adjacency_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
    if (color[*vi] == white_color) {
      if (has_cycle_dfs(g, *vi, color))
        return true;            // cycle detected, return immediately
    } else if (color[*vi] == gray_color)        // *vi is an ancestor!
      return true;
  color[u] = black_color;
  return false;
}

bool
has_cycle(const file_dep_graph & g)
{
  std::vector < default_color_type > color(num_vertices(g), white_color);
  graph_traits < file_dep_graph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (color[*vi] == white_color)
      if (has_cycle_dfs(g, *vi, &color[0]))
        return true;
  return false;
}


int
main()
{
  std::ifstream file_in("makefile-dependencies.dat");
  typedef graph_traits < file_dep_graph >::vertices_size_type size_type;
  size_type n_vertices;
  file_in >> n_vertices;        // read in number of vertices
  std::istream_iterator < std::pair < size_type,
    size_type > > input_begin(file_in), input_end;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ has trouble with the edge iterator constructor
  file_dep_graph g(n_vertices);
  while (input_begin != input_end) {
    size_type i, j;
    boost::tie(i, j) = *input_begin++;
    add_edge(i, j, g);
  }
#else
  file_dep_graph g(input_begin, input_end, n_vertices);
#endif

  std::vector < std::string > name(num_vertices(g));
  std::ifstream name_in("makefile-target-names.dat");
  graph_traits < file_dep_graph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    name_in >> name[*vi];

  assert(has_cycle(g) == false);
  return 0;
}
