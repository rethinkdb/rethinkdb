//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/range/irange.hpp>
#include <boost/pending/indirect_cmp.hpp>

#include <iostream>

using namespace boost;
template < typename TimeMap > class dfs_time_visitor:public default_dfs_visitor {
  typedef typename property_traits < TimeMap >::value_type T;
public:
  dfs_time_visitor(TimeMap dmap, TimeMap fmap, T & t)
:  m_dtimemap(dmap), m_ftimemap(fmap), m_time(t) {
  }
  template < typename Vertex, typename Graph >
    void discover_vertex(Vertex u, const Graph & g) const
  {
    put(m_dtimemap, u, m_time++);
  }
  template < typename Vertex, typename Graph >
    void finish_vertex(Vertex u, const Graph & g) const
  {
    put(m_ftimemap, u, m_time++);
  }
  TimeMap m_dtimemap;
  TimeMap m_ftimemap;
  T & m_time;
};


int
main()
{
  // Select the graph type we wish to use
  typedef adjacency_list < vecS, vecS, directedS > graph_t;
  typedef graph_traits < graph_t >::vertices_size_type size_type;
  // Set up the vertex names
  enum
  { u, v, w, x, y, z, N };
  char name[] = { 'u', 'v', 'w', 'x', 'y', 'z' };
  // Specify the edges in the graph
  typedef std::pair < int, int >E;
  E edge_array[] = { E(u, v), E(u, x), E(x, v), E(y, x),
    E(v, y), E(w, y), E(w, z), E(z, z)
  };
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  graph_t g(N);  
  for (std::size_t j = 0; j < sizeof(edge_array) / sizeof(E); ++j)
    add_edge(edge_array[j].first, edge_array[j].second, g);
#else
  graph_t g(edge_array, edge_array + sizeof(edge_array) / sizeof(E), N);
#endif

  // discover time and finish time properties
  std::vector < size_type > dtime(num_vertices(g));
  std::vector < size_type > ftime(num_vertices(g));
  typedef
    iterator_property_map<std::vector<size_type>::iterator,
                          property_map<graph_t, vertex_index_t>::const_type>
    time_pm_type;
  time_pm_type dtime_pm(dtime.begin(), get(vertex_index, g));
  time_pm_type ftime_pm(ftime.begin(), get(vertex_index, g));
  size_type t = 0;
  dfs_time_visitor < time_pm_type >vis(dtime_pm, ftime_pm, t);

  depth_first_search(g, visitor(vis));

  // use std::sort to order the vertices by their discover time
  std::vector < size_type > discover_order(N);
  integer_range < size_type > r(0, N);
  std::copy(r.begin(), r.end(), discover_order.begin());
  std::sort(discover_order.begin(), discover_order.end(),
            indirect_cmp < time_pm_type, std::less < size_type > >(dtime_pm));
  std::cout << "order of discovery: ";
  int i;
  for (i = 0; i < N; ++i)
    std::cout << name[discover_order[i]] << " ";

  std::vector < size_type > finish_order(N);
  std::copy(r.begin(), r.end(), finish_order.begin());
  std::sort(finish_order.begin(), finish_order.end(),
            indirect_cmp < time_pm_type, std::less < size_type > >(ftime_pm));
  std::cout << std::endl << "order of finish: ";
  for (i = 0; i < N; ++i)
    std::cout << name[finish_order[i]] << " ";
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
