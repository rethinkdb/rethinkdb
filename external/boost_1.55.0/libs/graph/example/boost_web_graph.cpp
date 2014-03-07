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
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <boost/pending/stringtok.hpp>
#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>


template <class Distance>
class calc_distance_visitor : public boost::bfs_visitor<>
{
public:
  calc_distance_visitor(Distance d) : distance(d) { }

  template <class Graph>
  void tree_edge(typename boost::graph_traits<Graph>::edge_descriptor e,
                 Graph& g)
  {
    typename boost::graph_traits<Graph>::vertex_descriptor u, v;
    u = boost::source(e, g);
    v = boost::target(e, g);
    distance[v] = distance[u] + 1;
  }
private:
  Distance distance;
};


template <class VertexNameMap, class DistanceMap>
class print_tree_visitor : public boost::dfs_visitor<>
{
public:
  print_tree_visitor(VertexNameMap n, DistanceMap d) : name(n), distance(d) { }
  template <class Graph>
  void 
  discover_vertex(typename boost::graph_traits<Graph>::vertex_descriptor v,
            Graph&)
  {
    typedef typename boost::property_traits<DistanceMap>::value_type Dist;
    // indentation based on depth
    for (Dist i = 0; i < distance[v]; ++i)
      std::cout << "  ";
    std::cout << name[v] << std::endl;
  }

  template <class Graph>
  void tree_edge(typename boost::graph_traits<Graph>::edge_descriptor e,
                 Graph& g)
  {
    distance[boost::target(e, g)] = distance[boost::source(e, g)] + 1;
  }  

private:
  VertexNameMap name;
  DistanceMap distance;
};

int
main()
{
  using namespace boost;

  std::ifstream datafile("./boost_web.dat");
  if (!datafile) {
    std::cerr << "No ./boost_web.dat file" << std::endl;
    return -1;
  }

  //===========================================================================
  // Declare the graph type and object, and some property maps.

  typedef adjacency_list<vecS, vecS, directedS, 
    property<vertex_name_t, std::string, 
      property<vertex_color_t, default_color_type> >,
    property<edge_name_t, std::string, property<edge_weight_t, int> >
  > Graph;

  typedef graph_traits<Graph> Traits;
  typedef Traits::vertex_descriptor Vertex;
  typedef Traits::edge_descriptor Edge;

  typedef std::map<std::string, Vertex> NameVertexMap;
  NameVertexMap name2vertex;
  Graph g;

  typedef property_map<Graph, vertex_name_t>::type NameMap;
  NameMap node_name  = get(vertex_name, g);
  property_map<Graph, edge_name_t>::type link_name = get(edge_name, g);

  //===========================================================================
  // Read the data file and construct the graph.
  
  std::string line;
  while (std::getline(datafile,line)) {

    std::list<std::string> line_toks;
    boost::stringtok(line_toks, line, "|");

    NameVertexMap::iterator pos; 
    bool inserted;
    Vertex u, v;

    std::list<std::string>::iterator i = line_toks.begin();

    boost::tie(pos, inserted) = name2vertex.insert(std::make_pair(*i, Vertex()));
    if (inserted) {
      u = add_vertex(g);
      put(node_name, u, *i);
      pos->second = u;
    } else
      u = pos->second;
    ++i;

    std::string hyperlink_name = *i++;
      
    boost::tie(pos, inserted) = name2vertex.insert(std::make_pair(*i, Vertex()));
    if (inserted) {
      v = add_vertex(g);
      put(node_name, v, *i);
      pos->second = v;
    } else
      v = pos->second;

    Edge e;
    boost::tie(e, inserted) = add_edge(u, v, g);
    if (inserted) {
      put(link_name, e, hyperlink_name);
    }
  }

  //===========================================================================
  // Calculate the diameter of the graph.

  typedef Traits::vertices_size_type size_type;
  typedef std::vector<size_type> IntVector;
  // Create N x N matrix for storing the shortest distances
  // between each vertex. Initialize all distances to zero.
  std::vector<IntVector> d_matrix(num_vertices(g),
                                  IntVector(num_vertices(g), 0));

  size_type i;
  for (i = 0; i < num_vertices(g); ++i) {
    calc_distance_visitor<size_type*> vis(&d_matrix[i][0]);
    Traits::vertex_descriptor src = vertices(g).first[i];
    breadth_first_search(g, src, boost::visitor(vis));
  }

  size_type diameter = 0;
  BOOST_USING_STD_MAX();
  for (i = 0; i < num_vertices(g); ++i)
    diameter = max BOOST_PREVENT_MACRO_SUBSTITUTION(diameter, *std::max_element(d_matrix[i].begin(), 
                                                    d_matrix[i].end()));
  
  std::cout << "The diameter of the boost web-site graph is " << diameter
            << std::endl << std::endl;

  std::cout << "Number of clicks from the home page: " << std::endl;
  Traits::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    std::cout << d_matrix[0][*vi] << "\t" << node_name[*vi] << std::endl;
  std::cout << std::endl;
  
  //===========================================================================
  // Print out the breadth-first search tree starting at the home page

  // Create storage for a mapping from vertices to their parents
  std::vector<Traits::vertex_descriptor> parent(num_vertices(g));
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    parent[*vi] = *vi;

  // Do a BFS starting at the home page, recording the parent of each
  // vertex (where parent is with respect to the search tree).
  Traits::vertex_descriptor src = vertices(g).first[0];
  breadth_first_search
    (g, src, 
     boost::visitor(make_bfs_visitor(record_predecessors(&parent[0],
                                                         on_tree_edge()))));

  // Add all the search tree edges into a new graph
  Graph search_tree(num_vertices(g));
  boost::tie(vi, vi_end) = vertices(g);
  ++vi;
  for (; vi != vi_end; ++vi)
    add_edge(parent[*vi], *vi, search_tree);

  std::cout << "The breadth-first search tree:" << std::endl;

  // Print out the search tree. We use DFS because it visits
  // the tree nodes in the order that we want to print out:
  // a directory-structure like format.
  std::vector<size_type> dfs_distances(num_vertices(g), 0);
  print_tree_visitor<NameMap, size_type*>
    tree_printer(node_name, &dfs_distances[0]);
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    get(vertex_color, g)[*vi] = white_color;
  depth_first_visit(search_tree, src, tree_printer, get(vertex_color, g));
  
  return EXIT_SUCCESS;
}
