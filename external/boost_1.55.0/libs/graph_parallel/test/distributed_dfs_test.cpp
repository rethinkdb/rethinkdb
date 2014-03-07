//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee,
// Copyright 2004 Douglas Gregor
// Copyright (C) 2006-2008 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/test/minimal.hpp>
#include <iostream>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;
using boost::graph::distributed::mpi_process_group;

// Set up the vertex names
enum vertex_id_t { u, v, w, x, y, z, N };
char vertex_names[] = { 'u', 'v', 'w', 'x', 'y', 'z' };

template<typename Vertex, typename Graph>
char get_vertex_name(Vertex v,  const Graph& g)
{
  return vertex_names[g.distribution().global(owner(v), local(v))];
}

struct printing_dfs_visitor
{
  template<typename Vertex, typename Graph>
  void initialize_vertex(Vertex v, const Graph& g)
  {
    vertex_event("initialize_vertex", v, g);
  }

  template<typename Vertex, typename Graph>
  void start_vertex(Vertex v, const Graph& g)
  {
    vertex_event("start_vertex", v, g);
  }

  template<typename Vertex, typename Graph>
  void discover_vertex(Vertex v, const Graph& g)
  {
    vertex_event("discover_vertex", v, g);
  }

  template<typename Edge, typename Graph>
  void examine_edge(Edge e, const Graph& g)
  {
    edge_event("examine_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void tree_edge(Edge e, const Graph& g)
  {
    edge_event("tree_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void back_edge(Edge e, const Graph& g)
  {
    edge_event("back_edge", e, g);
  }

  template<typename Edge, typename Graph>
  void forward_or_cross_edge(Edge e, const Graph& g)
  {
    edge_event("forward_or_cross_edge", e, g);
  }

  template<typename Vertex, typename Graph>
  void finish_vertex(Vertex v, const Graph& g)
  {
    vertex_event("finish_vertex", v, g);
  }

private:
  template<typename Vertex, typename Graph>
  void vertex_event(const char* name, Vertex v, const Graph& g)
  {
    std::cerr << "#" << process_id(g.process_group()) << ": " << name << "("
              << get_vertex_name(v, g) << ": " << local(v) << "@" << owner(v)
              << ")\n";
  }

  template<typename Edge, typename Graph>
  void edge_event(const char* name, Edge e, const Graph& g)
  {
    std::cerr << "#" << process_id(g.process_group()) << ": " << name << "("
              << get_vertex_name(source(e, g), g) << ": "
              << local(source(e, g)) << "@" << owner(source(e, g)) << ", "
              << get_vertex_name(target(e, g), g) << ": "
              << local(target(e, g)) << "@" << owner(target(e, g)) << ")\n";
  }

};

void
test_distributed_dfs()
{
  typedef adjacency_list<listS,
                         distributedS<mpi_process_group, vecS>,
                         undirectedS,
                         // Vertex properties
                         property<vertex_color_t, default_color_type> >
    Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  // Specify the edges in the graph
  typedef std::pair<int, int> E;
  E edge_array[] = { E(u, v), E(u, w), E(u, x), E(x, v), E(y, x),
                     E(v, y), E(w, y), E(w, z), E(z, z) };
  Graph g(edge_array, edge_array + sizeof(edge_array) / sizeof(E), N);

  std::vector<vertex_descriptor> parent(num_vertices(g));
  std::vector<vertex_descriptor> explore(num_vertices(g));

  boost::graph::tsin_depth_first_visit
    (g,
     vertex(u, g),
     printing_dfs_visitor(),
     get(vertex_color, g),
     make_iterator_property_map(parent.begin(), get(vertex_index, g)),
     make_iterator_property_map(explore.begin(), get(vertex_index, g)),
     get(vertex_index, g));

#if 0
  std::size_t correct_parents1[N] = {u, u, y, y, v, w};
  std::size_t correct_parents2[N] = {u, u, y, v, x, w};
#endif

  for (std::size_t i = 0; i < N; ++i) {
    vertex_descriptor v = vertex(i, g);
    if (owner(v) == process_id(g.process_group())) {
      std::cout  << "parent(" << get_vertex_name(v, g) << ") = "
                 << get_vertex_name(parent[v.local], g) << std::endl;

    }
  }

  if (false) {
    depth_first_visit(g, vertex(u, g), printing_dfs_visitor());
  }
}

int
test_main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  test_distributed_dfs();
  return 0;
}
