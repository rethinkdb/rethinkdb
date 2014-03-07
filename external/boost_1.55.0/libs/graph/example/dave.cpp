//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <iterator>
#include <vector>
#include <list>
// Use boost::queue instead of std::queue because std::queue doesn't
// model Buffer; it has to top() function. -Jeremy
#include <boost/pending/queue.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace std;
using namespace boost;
/*
  This example does a best-first-search (using dijkstra's) and
  simultaneously makes a copy of the graph (assuming the graph is
  connected).

  Example Graph: (p. 90 "Data Structures and Network Algorithms", Tarjan)

              g
            3+ +2
            / 1 \
           e+----f
           |+0 5++
           | \ / |
         10|  d  |12
           |8++\7|
           +/ | +|
           b 4|  c
            \ | +
            6+|/3
              a

  Sample Output:
a --> c d 
b --> a d
c --> f
d --> c e f
e --> b g
f --> e g
g -->
Starting graph:
a(32767); c d
c(32767); f
d(32767); c e f
f(32767); e g
e(32767); b g
g(32767);
b(32767); a d
Result:
a(0); d c
d(4); f e c
c(3); f
f(9); g e
e(4); g b
g(7);
b(14); d a 

*/

typedef property<vertex_color_t, default_color_type, 
         property<vertex_distance_t,int> > VProperty;
typedef int weight_t;
typedef property<edge_weight_t,weight_t> EProperty;

typedef adjacency_list<vecS, vecS, directedS, VProperty, EProperty > Graph;



template <class Tag>
struct endl_printer
  : public boost::base_visitor< endl_printer<Tag> >
{
  typedef Tag event_filter;
  endl_printer(std::ostream& os) : m_os(os) { }
  template <class T, class Graph>
  void operator()(T, Graph&) { m_os << std::endl; }
  std::ostream& m_os;
};
template <class Tag>
endl_printer<Tag> print_endl(std::ostream& os, Tag) {
  return endl_printer<Tag>(os);
}

template <class PA, class Tag>
struct edge_printer
 : public boost::base_visitor< edge_printer<PA, Tag> >
{
  typedef Tag event_filter;

  edge_printer(PA pa, std::ostream& os) : m_pa(pa), m_os(os) { }

  template <class T, class Graph>
  void operator()(T x, Graph& g) {
    m_os << "(" << get(m_pa, source(x, g)) << "," 
         << get(m_pa, target(x, g)) << ") ";
  }
  PA m_pa;
  std::ostream& m_os;
};
template <class PA, class Tag>
edge_printer<PA, Tag>
print_edge(PA pa, std::ostream& os, Tag) {
  return edge_printer<PA, Tag>(pa, os);
}


template <class NewGraph, class Tag>
struct graph_copier 
  : public boost::base_visitor<graph_copier<NewGraph, Tag> >
{
  typedef Tag event_filter;

  graph_copier(NewGraph& graph) : new_g(graph) { }

  template <class Edge, class Graph>
  void operator()(Edge e, Graph& g) {
    add_edge(source(e, g), target(e, g), new_g);
  }
private:
  NewGraph& new_g;
};
template <class NewGraph, class Tag>
inline graph_copier<NewGraph, Tag>
copy_graph(NewGraph& g, Tag) {
  return graph_copier<NewGraph, Tag>(g);
}

template <class Graph, class Name>
void print(Graph& G, Name name)
{
  typename boost::graph_traits<Graph>::vertex_iterator ui, uiend;
  for (boost::tie(ui, uiend) = vertices(G); ui != uiend; ++ui) {
    cout << name[*ui] << " --> ";
    typename boost::graph_traits<Graph>::adjacency_iterator vi, viend;
    for(boost::tie(vi, viend) = adjacent_vertices(*ui, G); vi != viend; ++vi)
      cout << name[*vi] << " ";
    cout << endl;
  }
    
}


int 
main(int , char* [])
{
  // Name and ID numbers for the vertices
  char name[] = "abcdefg";
  enum { a, b, c, d, e, f, g, N};

  Graph G(N);
  boost::property_map<Graph, vertex_index_t>::type 
    vertex_id = get(vertex_index, G);

  std::vector<weight_t> distance(N, (numeric_limits<weight_t>::max)());
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  std::vector<Vertex> parent(N);

  typedef std::pair<int,int> E;

  E edges[] = { E(a,c), E(a,d),
                E(b,a), E(b,d),
                E(c,f),
                E(d,c), E(d,e), E(d,f),
                E(e,b), E(e,g),
                E(f,e), E(f,g) };

  int weight[] = { 3, 4,
                   6, 8,
                   12,
                   7, 0, 5,
                   10, 3,
                   1, 2 };

  for (int i = 0; i < 12; ++i)
    add_edge(edges[i].first, edges[i].second, weight[i], G);

  print(G, name);

  adjacency_list<listS, vecS, directedS, 
    property<vertex_color_t, default_color_type> > G_copy(N);

  cout << "Starting graph:" << endl;

  std::ostream_iterator<int> cout_int(std::cout, " ");
  std::ostream_iterator<char> cout_char(std::cout, " ");

  boost::queue<Vertex> Q;
  boost::breadth_first_search
    (G, vertex(a, G), Q,
     make_bfs_visitor(
     boost::make_list
      (write_property(make_iterator_property_map(name, vertex_id,
                                                name[0]),
                      cout_char, on_examine_vertex()),
       write_property(make_iterator_property_map(distance.begin(),
                                                vertex_id, 
                                                distance[0]), 
                      cout_int, on_examine_vertex()),
       print_edge(make_iterator_property_map(name, vertex_id, 
                                            name[0]),
                  std::cout, on_examine_edge()),
       print_endl(std::cout, on_finish_vertex()))),
     get(vertex_color, G));

  std::cout << "about to call dijkstra's" << std::endl;

  parent[vertex(a, G)] = vertex(a, G);
  boost::dijkstra_shortest_paths
    (G, vertex(a, G), 
     distance_map(make_iterator_property_map(distance.begin(), vertex_id, 
                                             distance[0])).
     predecessor_map(make_iterator_property_map(parent.begin(), vertex_id,
                                                parent[0])).
     visitor(make_dijkstra_visitor(copy_graph(G_copy, on_examine_edge()))));

  cout << endl;
  cout << "Result:" << endl;
  boost::breadth_first_search
    (G, vertex(a, G), 
     visitor(make_bfs_visitor(
     boost::make_list
     (write_property(make_iterator_property_map(name, vertex_id,
                                                name[0]),
                     cout_char, on_examine_vertex()),
      write_property(make_iterator_property_map(distance.begin(),
                                                vertex_id, 
                                                distance[0]), 
                     cout_int, on_examine_vertex()),
      print_edge(make_iterator_property_map(name, vertex_id, 
                                            name[0]),
                 std::cout, on_examine_edge()),
      print_endl(std::cout, on_finish_vertex())))));

  return 0;
}
