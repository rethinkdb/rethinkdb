// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/kamada_kawai_spring_layout.hpp>
#include <boost/graph/circle_layout.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/point_traits.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>
#include <boost/limits.hpp>
#include <fstream>
#include <string>
using namespace boost;

enum vertex_position_t { vertex_position };
namespace boost { BOOST_INSTALL_PROPERTY(vertex, position); }

typedef square_topology<>::point_type point;

template<typename Graph, typename PositionMap, typename Topology>
void print_graph_layout(const Graph& g, PositionMap position, const Topology& topology)
{
  typedef typename Topology::point_type Point;
  // Find min/max ranges
  Point min_point = position[*vertices(g).first], max_point = min_point;
  BGL_FORALL_VERTICES_T(v, g, Graph) {
    min_point = topology.pointwise_min(min_point, position[v]);
    max_point = topology.pointwise_max(max_point, position[v]);
  }

  for (int y = (int)min_point[1]; y <= (int)max_point[1]; ++y) {
    for (int x = (int)min_point[0]; x <= (int)max_point[0]; ++x) {
      typename graph_traits<Graph>::vertex_iterator vi, vi_end;
      // Find vertex at this position
      typename graph_traits<Graph>::vertices_size_type index = 0;
      for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi, ++index) {
        if ((int)position[*vi][0] == x && (int)position[*vi][1] == y)
          break;
      }

      if (vi == vi_end) std::cout << ' ';
      else std::cout << (char)(index + 'A');
    }
    std::cout << std::endl;
  }
}

template<typename Graph, typename PositionMap>
void dump_graph_layout(std::string name, const Graph& g, PositionMap position)
{
  std::ofstream out((name + ".dot").c_str());
  out << "graph " << name << " {" << std::endl;

  typename graph_traits<Graph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    out << "  n" << get(vertex_index, g, *vi) << "[ pos=\"" 
        << (int)position[*vi][0] + 25 << ", " << (int)position[*vi][1] + 25 
        << "\" ];\n";
  }

  typename graph_traits<Graph>::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
    out << "  n" << get(vertex_index, g, source(*ei, g)) << " -- n"
        << get(vertex_index, g, target(*ei, g)) << ";\n";
  }
  out << "}\n";
}

template<typename Graph>
void 
test_circle_layout(Graph*, typename graph_traits<Graph>::vertices_size_type n)
{
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;

  Graph g(n);

  // Initialize vertex indices
  vertex_iterator vi = vertices(g).first;
  for (vertices_size_type i = 0; i < n; ++i, ++vi) 
    put(vertex_index, g, *vi, i);

  circle_graph_layout(g, get(vertex_position, g), 10.0);

  std::cout << "Regular polygon layout with " << n << " points.\n";
  square_topology<> topology;
  print_graph_layout(g, get(vertex_position, g), topology);
}

struct simple_edge
{
  int first, second;
};

struct kamada_kawai_done 
{
  kamada_kawai_done() : last_delta() {}

  template<typename Graph>
  bool operator()(double delta_p, 
                  typename boost::graph_traits<Graph>::vertex_descriptor /*p*/,
                  const Graph& /*g*/,
                  bool global)
  {
    if (global) {
      double diff = last_delta - delta_p;
      if (diff < 0) diff = -diff;
      last_delta = delta_p;
      return diff < 0.01;
    } else {
      return delta_p < 0.01;
    }
  }

  double last_delta;
};

template<typename Graph>
void
test_triangle(Graph*)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;

  Graph g;
  
  vertex_descriptor u = add_vertex(g); put(vertex_index, g, u, 0);
  vertex_descriptor v = add_vertex(g); put(vertex_index, g, v, 1);
  vertex_descriptor w = add_vertex(g); put(vertex_index, g, w, 2);

  edge_descriptor e1 = add_edge(u, v, g).first; put(edge_weight, g, e1, 1.0);
  edge_descriptor e2 = add_edge(v, w, g).first; put(edge_weight, g, e2, 1.0);
  edge_descriptor e3 = add_edge(w, u, g).first; put(edge_weight, g, e3, 1.0);

  circle_graph_layout(g, get(vertex_position, g), 25.0);

  bool ok = kamada_kawai_spring_layout(g, 
                                       get(vertex_position, g),
                                       get(edge_weight, g),
                                       square_topology<>(50.0),
                                       side_length(50.0));
  BOOST_CHECK(ok);

  std::cout << "Triangle layout (Kamada-Kawai).\n";
  print_graph_layout(g, get(vertex_position, g));
}

template<typename Graph>
void
test_cube(Graph*)
{
  enum {A, B, C, D, E, F, G, H};
  simple_edge cube_edges[12] = {
    {A, E}, {A, B}, {A, D}, {B, F}, {B, C}, {C, D}, {C, G}, {D, H}, 
    {E, H}, {E, F}, {F, G}, {G, H}
  };

  Graph g(&cube_edges[0], &cube_edges[12], 8);
  
  typedef typename graph_traits<Graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  vertex_iterator vi, vi_end;
  int i = 0;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    put(vertex_index, g, *vi, i++);

  edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
    put(edge_weight, g, *ei, 1.0);
    std::cerr << "(" << (char)(get(vertex_index, g, source(*ei, g)) + 'A') 
              << ", " << (char)(get(vertex_index, g, target(*ei, g)) + 'A')
              << ") ";
  }
  std::cerr << std::endl;

  circle_graph_layout(g, get(vertex_position, g), 25.0);

  bool ok = kamada_kawai_spring_layout(g, 
                                       get(vertex_position, g),
                                       get(edge_weight, g),
                                       square_topology<>(50.0),
                                       side_length(50.0),
                                       kamada_kawai_done());
  BOOST_CHECK(ok);

  std::cout << "Cube layout (Kamada-Kawai).\n";
  print_graph_layout(g, get(vertex_position, g), square_topology<>(50.));

  dump_graph_layout("cube", g, get(vertex_position, g));

  minstd_rand gen;
  typedef square_topology<> Topology;
  Topology topology(gen, 50.0);
  std::vector<Topology::point_difference_type> displacements(num_vertices(g));
  rectangle_topology<> rect_top(gen, 0, 0, 50, 50);
  random_graph_layout(g, get(vertex_position, g), rect_top);

  fruchterman_reingold_force_directed_layout
    (g,
     get(vertex_position, g),
     topology,
     square_distance_attractive_force(),
     square_distance_repulsive_force(),
     all_force_pairs(),
     linear_cooling<double>(100),
     make_iterator_property_map(displacements.begin(),
                                get(vertex_index, g),
                                Topology::point_difference_type()));

  std::cout << "Cube layout (Fruchterman-Reingold).\n";
  print_graph_layout(g, get(vertex_position, g), square_topology<>(50.));

  dump_graph_layout("cube-fr", g, get(vertex_position, g));
}

template<typename Graph>
void
test_triangular(Graph*)
{
  enum {A, B, C, D, E, F, G, H, I, J};
  simple_edge triangular_edges[18] = {
    {A, B}, {A, C}, {B, C}, {B, D}, {B, E}, {C, E}, {C, F}, {D, E}, {D, G},
    {D, H}, {E, F}, {E, H}, {E, I}, {F, I}, {F, J}, {G, H}, {H, I}, {I, J}
  };

  Graph g(&triangular_edges[0], &triangular_edges[18], 10);
  
  typedef typename graph_traits<Graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  vertex_iterator vi, vi_end;
  int i = 0;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    put(vertex_index, g, *vi, i++);

  edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
    put(edge_weight, g, *ei, 1.0);
    std::cerr << "(" << (char)(get(vertex_index, g, source(*ei, g)) + 'A') 
              << ", " << (char)(get(vertex_index, g, target(*ei, g)) + 'A')
              << ") ";
  }
  std::cerr << std::endl;

  typedef square_topology<> Topology;
  minstd_rand gen;
  Topology topology(gen, 50.0);
  Topology::point_type origin;
  origin[0] = origin[1] = 50.0;
  Topology::point_difference_type extent;
  extent[0] = extent[1] = 50.0;

  circle_graph_layout(g, get(vertex_position, g), 25.0);

  bool ok = kamada_kawai_spring_layout(g, 
                                       get(vertex_position, g),
                                       get(edge_weight, g),
                                       topology,
                                       side_length(50.0),
                                       kamada_kawai_done());
  BOOST_CHECK(ok);

  std::cout << "Triangular layout (Kamada-Kawai).\n";
  print_graph_layout(g, get(vertex_position, g), square_topology<>(50.));

  dump_graph_layout("triangular-kk", g, get(vertex_position, g));

  rectangle_topology<> rect_top(gen, -25, -25, 25, 25);
  random_graph_layout(g, get(vertex_position, g), rect_top);

  dump_graph_layout("random", g, get(vertex_position, g));

  std::vector<Topology::point_difference_type> displacements(num_vertices(g));
  fruchterman_reingold_force_directed_layout
    (g,
     get(vertex_position, g),
     topology,
     attractive_force(square_distance_attractive_force()).
     cooling(linear_cooling<double>(100)));

  std::cout << "Triangular layout (Fruchterman-Reingold).\n";
  print_graph_layout(g, get(vertex_position, g), square_topology<>(50.));

  dump_graph_layout("triangular-fr", g, get(vertex_position, g));
}

template<typename Graph>
void
test_disconnected(Graph*)
{
  enum {A, B, C, D, E, F, G, H};
  simple_edge triangular_edges[13] = {
    {A, B}, {B, C}, {C, A}, 
    {D, E}, {E, F}, {F, G}, {G, H}, {H, D},
    {D, F}, {F, H}, {H, E}, {E, G}, {G, D}
  };

  Graph g(&triangular_edges[0], &triangular_edges[13], 8);
  
  typedef typename graph_traits<Graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  vertex_iterator vi, vi_end;
  int i = 0;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    put(vertex_index, g, *vi, i++);

  edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
    put(edge_weight, g, *ei, 1.0);
    std::cerr << "(" << (char)(get(vertex_index, g, source(*ei, g)) + 'A') 
              << ", " << (char)(get(vertex_index, g, target(*ei, g)) + 'A')
              << ") ";
  }
  std::cerr << std::endl;

  circle_graph_layout(g, get(vertex_position, g), 25.0);

  bool ok = kamada_kawai_spring_layout(g, 
                                       get(vertex_position, g),
                                       get(edge_weight, g),
                                       square_topology<>(50.0),
                                       side_length(50.0),
                                       kamada_kawai_done());
  BOOST_CHECK(!ok);

  minstd_rand gen;
  rectangle_topology<> rect_top(gen, -25, -25, 25, 25);
  random_graph_layout(g, get(vertex_position, g), rect_top);

  typedef square_topology<> Topology;
  Topology topology(gen, 50.0);
  std::vector<Topology::point_difference_type> displacements(num_vertices(g));
  fruchterman_reingold_force_directed_layout
    (g,
     get(vertex_position, g),
     topology,
     attractive_force(square_distance_attractive_force()).
     cooling(linear_cooling<double>(50)));

  std::cout << "Disconnected layout (Fruchterman-Reingold).\n";
  print_graph_layout(g, get(vertex_position, g), square_topology<>(50.));

  dump_graph_layout("disconnected-fr", g, get(vertex_position, g));
}

int test_main(int, char*[])
{
  typedef adjacency_list<listS, listS, undirectedS, 
                         // Vertex properties
                         property<vertex_index_t, int,
                         property<vertex_position_t, point> >,
                         // Edge properties
                         property<edge_weight_t, double> > Graph;

  test_circle_layout((Graph*)0, 5);
  test_cube((Graph*)0);
  test_triangular((Graph*)0);
  test_disconnected((Graph*)0);
  return 0;
}
