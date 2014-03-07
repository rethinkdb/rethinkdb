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
#include <unistd.h>
#include <sys/stat.h>
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


int
main()
{
  typedef adjacency_list < listS,       // Store out-edges of each vertex in a std::list
    vecS,                       // Store vertex set in a std::vector
    directedS,                  // The graph is directed
    property < vertex_name_t, std::string >     // Add a vertex property
   >graph_type;

  graph_type g;                 // use default constructor to create empty graph
  std::ifstream file_in("makefile-dependencies.dat"),
    name_in("makefile-target-names.dat");
  if (!file_in) {
    std::cerr << "** Error: could not open file makefile-target-names.dat"
      << std::endl;
    exit(-1);
  }
  // Obtain internal property map from the graph
  property_map < graph_type, vertex_name_t >::type name_map =
    get(vertex_name, g);
  read_graph_file(file_in, name_in, g, name_map);

  // Create storage for last modified times
  std::vector < time_t > last_mod_vec(num_vertices(g));
  // Create nickname for the property map type
  typedef iterator_property_map < std::vector < time_t >::iterator,
    property_map < graph_type, vertex_index_t >::type, time_t, time_t&> iter_map_t;
  // Create last modified time property map
  iter_map_t mod_time_map(last_mod_vec.begin(), get(vertex_index, g));

  property_map < graph_type, vertex_name_t >::type name = get(vertex_name, g);
  struct stat stat_buf;
  graph_traits < graph_type >::vertex_descriptor u;
  typedef graph_traits < graph_type >::vertex_iterator vertex_iter_t;
  std::pair < vertex_iter_t, vertex_iter_t > p;
  for (p = vertices(g); p.first != p.second; ++p.first) {
    u = *p.first;
    if (stat(name[u].c_str(), &stat_buf) != 0)
      std::cerr << "error in stat() for file " << name[u] << std::endl;
    put(mod_time_map, u, stat_buf.st_mtime);
  }

  for (p = vertices(g); p.first != p.second; ++p.first) {
    std::cout << name[*p.first] << " was last modified at "
      << ctime(&mod_time_map[*p.first]);
  }
  assert(num_vertices(g) == 15);
  assert(num_edges(g) == 19);
  return 0;
}
