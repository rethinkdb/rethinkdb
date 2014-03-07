// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/isomorphism.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/graph/graph_utility.hpp>

/*
  Sample output:
  isomorphic? 1
  f: 9 10 11 0 1 3 2 4 6 8 7 5 
 */

int
main()
{
  using namespace boost;
  
  const int n = 12;

  typedef adjacency_list<vecS, listS, undirectedS,
    property<vertex_index_t, int> > graph_t;
  graph_t g1(n), g2(n);

  std::vector<graph_traits<graph_t>::vertex_descriptor> v1(n), v2(n);

  property_map<graph_t, vertex_index_t>::type 
    v1_index_map = get(vertex_index, g1),
    v2_index_map = get(vertex_index, g2);

  graph_traits<graph_t>::vertex_iterator i, end;
  int id = 0;
  for (boost::tie(i, end) = vertices(g1); i != end; ++i, ++id) {
    put(v1_index_map, *i, id);
    v1[id] = *i;
  }
  id = 0;
  for (boost::tie(i, end) = vertices(g2); i != end; ++i, ++id) {
    put(v2_index_map, *i, id);
    v2[id] = *i;
  }
  add_edge(v1[0], v1[1], g1); add_edge(v1[1], v1[2], g1); 
  add_edge(v1[0], v1[2], g1);
  add_edge(v1[3], v1[4], g1);  add_edge(v1[4], v1[5], g1);
  add_edge(v1[5], v1[6], g1);  add_edge(v1[6], v1[3], g1);
  add_edge(v1[7], v1[8], g1);  add_edge(v1[8], v1[9], g1);
  add_edge(v1[9], v1[10], g1);
  add_edge(v1[10], v1[11], g1);  add_edge(v1[11], v1[7], g1);

  add_edge(v2[9], v2[10], g2);  add_edge(v2[10], v2[11], g2);  
  add_edge(v2[11], v2[9], g2);
  add_edge(v2[0], v2[1], g2);  add_edge(v2[1], v2[3], g2); 
  add_edge(v2[3], v2[2], g2);  add_edge(v2[2], v2[0], g2);
  add_edge(v2[4], v2[5], g2); add_edge(v2[5], v2[7], g2); 
  add_edge(v2[7], v2[8], g2);
  add_edge(v2[8], v2[6], g2); add_edge(v2[6], v2[4], g2);

  std::vector<graph_traits<graph_t>::vertex_descriptor> f(n);

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  bool ret = isomorphism
    (g1, g2, make_iterator_property_map(f.begin(), v1_index_map, f[0]),
     degree_vertex_invariant(), get(vertex_index, g1), get(vertex_index, g2));
#else
  bool ret = isomorphism
    (g1, g2, isomorphism_map
     (make_iterator_property_map(f.begin(), v1_index_map, f[0])));
#endif
  std::cout << "isomorphic? " << ret << std::endl;

  std::cout << "f: ";
  for (std::size_t v = 0; v != f.size(); ++v)
    std::cout << get(get(vertex_index, g2), f[v]) << " ";
  std::cout << std::endl;
  
  return 0;
}
