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

void
topo_sort_dfs(const file_dep_graph & g, vertex_t u, vertex_t * &topo_order,
              int *mark)
{
  mark[u] = 1;                  // 1 means visited, 0 means not yet visited
  graph_traits < file_dep_graph >::adjacency_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
    if (mark[*vi] == 0)
      topo_sort_dfs(g, *vi, topo_order, mark);

  *--topo_order = u;
}

void
topo_sort(const file_dep_graph & g, vertex_t * topo_order)
{
  std::vector < int >mark(num_vertices(g), 0);
  graph_traits < file_dep_graph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (mark[*vi] == 0)
      topo_sort_dfs(g, *vi, topo_order, &mark[0]);
}


int
main()
{
  std::ifstream file_in("makefile-dependencies.dat");
  typedef graph_traits < file_dep_graph >::vertices_size_type size_type;
  size_type n_vertices;
  file_in >> n_vertices;        // read in number of vertices
  std::istream_iterator < std::pair < size_type, size_type > > 
    input_begin(file_in), input_end;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle the iterator constructor
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

  std::vector < vertex_t > order(num_vertices(g));
  topo_sort(g, &order[0] + num_vertices(g));
  for (size_type i = 0; i < num_vertices(g); ++i)
    std::cout << name[order[i]] << std::endl;

  return 0;
}
