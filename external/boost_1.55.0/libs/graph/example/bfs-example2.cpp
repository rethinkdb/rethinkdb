//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/range/irange.hpp>
#include <boost/property_map/property_map.hpp>

#include <iostream>

using namespace boost;
template < typename TimeMap > class bfs_time_visitor:public default_bfs_visitor {
  typedef typename property_traits < TimeMap >::value_type T;
public:
  bfs_time_visitor(TimeMap tmap, T & t):m_timemap(tmap), m_time(t) { }
  template < typename Vertex, typename Graph >
  void discover_vertex(Vertex u, const Graph & g) const
  {
    put(m_timemap, u, m_time++);
  }
  TimeMap m_timemap;
  T & m_time;
};


struct VertexProps {
  boost::default_color_type color;
  std::size_t discover_time;
  unsigned int index;
};

int
main()
{
  using namespace boost;
  // Select the graph type we wish to use
  typedef adjacency_list < listS, listS, undirectedS,
    VertexProps> graph_t;
  // Set up the vertex IDs and names
  enum { r, s, t, u, v, w, x, y, N };
  const char *name = "rstuvwxy";
  // Specify the edges in the graph
  typedef std::pair < int, int >E;
  E edge_array[] = { E(r, s), E(r, v), E(s, w), E(w, r), E(w, t),
    E(w, x), E(x, t), E(t, u), E(x, y), E(u, y)
  };
  // Create the graph object
  const int n_edges = sizeof(edge_array) / sizeof(E);
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ has trouble with the edge iterator constructor
  graph_t g;
  std::vector<graph_traits<graph_t>::vertex_descriptor> verts;
  for (std::size_t i = 0; i < N; ++i)
    verts.push_back(add_vertex(g));
  for (std::size_t j = 0; j < n_edges; ++j)
    add_edge(verts[edge_array[j].first], verts[edge_array[j].second], g);
#else
  typedef graph_traits<graph_t>::vertices_size_type v_size_t;
  graph_t g(edge_array, edge_array + n_edges, v_size_t(N));
#endif

  // Typedefs
  typedef graph_traits<graph_t>::vertices_size_type Size;

  Size time = 0;
  typedef property_map<graph_t, std::size_t VertexProps::*>::type dtime_map_t;
  dtime_map_t dtime_map = get(&VertexProps::discover_time, g);
  bfs_time_visitor < dtime_map_t > vis(dtime_map, time);
  breadth_first_search(g, vertex(s, g), color_map(get(&VertexProps::color, g)).
    visitor(vis));

  // a vector to hold the discover time property for each vertex
  std::vector < Size > dtime(num_vertices(g));
  typedef
    iterator_property_map<std::vector<Size>::iterator,
                          property_map<graph_t, unsigned int VertexProps::*>::type>
    dtime_pm_type;
  graph_traits<graph_t>::vertex_iterator vi, vi_end;
  std::size_t c = 0;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi, ++c) {
    dtime[c] = dtime_map[*vi];
    put(&VertexProps::index, g, *vi, c);
  }
  dtime_pm_type dtime_pm(dtime.begin(), get(&VertexProps::index, g));

  // Use std::sort to order the vertices by their discover time
  std::vector<graph_traits<graph_t>::vertices_size_type > discover_order(N);
  integer_range < int >range(0, N);
  std::copy(range.begin(), range.end(), discover_order.begin());
  std::sort(discover_order.begin(), discover_order.end(),
            make_indirect_cmp(
              std::less<Size>(),
              make_iterator_property_map(
                dtime.begin(),
                typed_identity_property_map<std::size_t>())));

  std::cout << "order of discovery: ";
  for (int i = 0; i < N; ++i)
    std::cout << name[discover_order[i]] << " ";
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
