//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//          Doug Gregor, D. Kevin McGrath
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <vector>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/king_ordering.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/bandwidth.hpp>

/*
  Sample Output
  original bandwidth: 8
  Reverse Cuthill-McKee ordering starting at: 6
    8 3 0 9 2 5 1 4 7 6 
    bandwidth: 4
  Reverse Cuthill-McKee ordering starting at: 0
    9 1 4 6 7 2 8 5 3 0 
    bandwidth: 4
  Reverse Cuthill-McKee ordering:
    0 8 5 7 3 6 4 2 1 9 
    bandwidth: 4
 */
int main(int , char* [])
{
  using namespace boost;
  using namespace std;
  typedef adjacency_list<vecS, vecS, undirectedS, 
     property<vertex_color_t, default_color_type,
       property<vertex_degree_t,int> > > Graph;
  typedef graph_traits<Graph>::vertex_descriptor Vertex;
  typedef graph_traits<Graph>::vertices_size_type size_type;

  typedef std::pair<std::size_t, std::size_t> Pair;
  Pair edges[14] = { Pair(0,3), //a-d
                     Pair(0,5),  //a-f
                     Pair(1,2),  //b-c
                     Pair(1,4),  //b-e
                     Pair(1,6),  //b-g
                     Pair(1,9),  //b-j
                     Pair(2,3),  //c-d
                     Pair(2,4),  //c-e
                     Pair(3,5),  //d-f
                     Pair(3,8),  //d-i
                     Pair(4,6),  //e-g
                     Pair(5,6),  //f-g
                     Pair(5,7),  //f-h
                     Pair(6,7) }; //g-h 
  
  Graph G(10);
  for (int i = 0; i < 14; ++i)
    add_edge(edges[i].first, edges[i].second, G);

  graph_traits<Graph>::vertex_iterator ui, ui_end;

  property_map<Graph,vertex_degree_t>::type deg = get(vertex_degree, G);
  for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
    deg[*ui] = degree(*ui, G);

  property_map<Graph, vertex_index_t>::type
    index_map = get(vertex_index, G);

  std::cout << "original bandwidth: " << bandwidth(G) << std::endl;

  std::vector<Vertex> inv_perm(num_vertices(G));
  std::vector<size_type> perm(num_vertices(G));
  {
    Vertex s = vertex(6, G);
    //king_ordering
    king_ordering(G, s, inv_perm.rbegin(), get(vertex_color, G), 
                  get(vertex_degree, G), get(vertex_index, G));
    cout << "King ordering starting at: " << s << endl;
    cout << "  ";    
    for (std::vector<Vertex>::const_iterator i = inv_perm.begin();
         i != inv_perm.end(); ++i)
      cout << index_map[*i] << " ";
    cout << endl;

    for (size_type c = 0; c != inv_perm.size(); ++c)
      perm[index_map[inv_perm[c]]] = c;
    std::cout << "  bandwidth: " 
              << bandwidth(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
  }
  {
    Vertex s = vertex(0, G);
    //king_ordering
    king_ordering(G, s, inv_perm.rbegin(), get(vertex_color, G),
                  get(vertex_degree, G), get(vertex_index, G));
    cout << "King ordering starting at: " << s << endl;
    cout << "  ";
    for (std::vector<Vertex>::const_iterator i=inv_perm.begin();
       i != inv_perm.end(); ++i)
      cout << index_map[*i] << " ";
    cout << endl;

    for (size_type c = 0; c != inv_perm.size(); ++c)
      perm[index_map[inv_perm[c]]] = c;
    std::cout << "  bandwidth: " 
              << bandwidth(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
  }

  {
    //king_ordering
    king_ordering(G, inv_perm.rbegin());
    
    cout << "King ordering:" << endl;
    cout << "  ";
    for (std::vector<Vertex>::const_iterator i=inv_perm.begin();
       i != inv_perm.end(); ++i)
      cout << index_map[*i] << " ";
    cout << endl;

    for (size_type c = 0; c != inv_perm.size(); ++c)
      perm[index_map[inv_perm[c]]] = c;
    std::cout << "  bandwidth: " 
              << bandwidth(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
  }
  return 0;
}
