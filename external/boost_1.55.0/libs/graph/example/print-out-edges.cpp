//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

template < typename Graph, typename VertexNamePropertyMap > void
read_graph_file(std::istream & graph_in, std::istream & name_in,
                Graph & g, VertexNamePropertyMap name_map)
{
  typedef typename graph_traits < Graph >::vertices_size_type size_type;
  size_type n_vertices;
  typename graph_traits < Graph >::vertex_descriptor u;
  typename property_traits < VertexNamePropertyMap >::value_type name;

  graph_in >> n_vertices;       // read in number of vertices
  for (size_type i = 0; i < n_vertices; ++i) {  // Add n vertices to the graph
    u = add_vertex(g);
    name_in >> name;
    put(name_map, u, name);     // ** Attach name property to vertex u **
  }
  size_type src, targ;
  while (graph_in >> src)       // Read in edges
    if (graph_in >> targ)
      add_edge(src, targ, g);   // add an edge to the graph
    else
      break;
}

template < typename Graph, typename VertexNameMap > void
output_out_edges(std::ostream & out, const Graph & g,
                 typename graph_traits < Graph >::vertex_descriptor u,
                 VertexNameMap name_map)
{
  typename graph_traits < Graph >::out_edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
    out << get(name_map, source(*ei, g)) << " -> "
      << get(name_map, target(*ei, g)) << std::endl;
}

template < typename NameMap > class name_equals_t {
public:
  name_equals_t(const std::string & n, NameMap map)
  : m_name(n), m_name_map(map)
  {
  }
  template < typename Vertex > bool operator()(Vertex u) const
  {
    return get(m_name_map, u) == m_name;
  }
private:
    std::string m_name;
  NameMap m_name_map;
};

// object generator function
template < typename NameMap >
  inline name_equals_t < NameMap >
name_equals(const std::string & str, NameMap name)
{
  return name_equals_t < NameMap > (str, name);
}


int
main()
{
  typedef adjacency_list < listS,       // Store out-edges of each vertex in a std::list
    vecS,                       // Store vertex set in a std::vector
    directedS,                  // The graph is directed
    property < vertex_name_t, std::string >     // Add a vertex property
   >graph_type;

  graph_type g;                 // use default constructor to create empty graph
  const char* dep_file_name = "makefile-dependencies.dat";
  const char* target_file_name = "makefile-target-names.dat";
  std::ifstream file_in(dep_file_name), name_in(target_file_name);
  if (!file_in) {
    std::cerr << "** Error: could not open file " << dep_file_name
      << std::endl;
    return -1;
  }
  if (!name_in) {
    std::cerr << "** Error: could not open file " << target_file_name
      << std::endl;
    return -1;
  }
  // Obtain internal property map from the graph
  property_map < graph_type, vertex_name_t >::type name_map =
    get(vertex_name, g);
  read_graph_file(file_in, name_in, g, name_map);

  graph_traits < graph_type >::vertex_iterator i, end;
  boost::tie(i, end) = vertices(g);
  typedef property_map < graph_type, vertex_name_t >::type name_map_t;
  name_equals_t < name_map_t > predicate("dax.h", get(vertex_name, g));
  i = std::find_if(i, end, predicate);
  output_out_edges(std::cout, g, *i, get(vertex_name, g));

  assert(num_vertices(g) == 15);
  assert(num_edges(g) == 19);
  return 0;
}
