//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

/*

This test is almost identical to all_planar_input_files_test.cpp
except that parallel edges and loops are added to the graphs as
they are read in.

This test needs to be linked against Boost.Filesystem.

*/

#define BOOST_FILESYSTEM_VERSION 3

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>


#include <boost/property_map/property_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/test/minimal.hpp>


#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/planar_canonical_ordering.hpp>
#include <boost/graph/make_connected.hpp>
#include <boost/graph/make_biconnected_planar.hpp>
#include <boost/graph/make_maximal_planar.hpp>
#include <boost/graph/is_straight_line_drawing.hpp>
#include <boost/graph/is_kuratowski_subgraph.hpp>
#include <boost/graph/chrobak_payne_drawing.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/planar_detail/add_edge_visitors.hpp>






using namespace boost;

struct coord_t
{
  std::size_t x;
  std::size_t y;
};



template <typename Graph>
void read_dimacs(Graph& g, const std::string& filename)
{

  // every <vertex_stride>th vertex has a self-loop
  int vertex_stride = 5; 

  // on vertices with self loops, there are between 1 and 
  // <max_loop_multiplicity> loops
  int max_loop_multiplicity = 6;

  // every <edge_stride>th edge is a parallel edge
  int edge_stride = 7;

  // parallel edges come in groups of 2 to <max_edge_multiplicity> + 1
  int max_edge_multiplicity = 5;
  
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  std::vector<vertex_t> vertices_by_index;
  
  std::ifstream in(filename.c_str());
  
  long num_edges_added = 0;
  long num_parallel_edges = 0;

  while (!in.eof())
    {
      
      char buffer[256];
      in.getline(buffer, 256);
      std::string s(buffer);
      
      if (s.size() == 0)
        continue;
      
      std::vector<std::string> v;
      split(v, buffer, is_any_of(" \t\n"));
      
      if (v[0] == "p")
        {
          //v[1] == "edge"
          long num_vertices = boost::lexical_cast<long>(v[2].c_str());
          g = Graph(num_vertices);
          
          
          vertex_iterator_t vi, vi_end;
          long count = 0;
          long mult_count = 0;
          for(boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            {
              if (count % vertex_stride == 0)
                {
                  for(int i = 0; 
                      i < (mult_count % max_loop_multiplicity) + 1;
                      ++i
                      )
                    {
                      add_edge(*vi, *vi, g);
                    }
                  ++mult_count;
                }
              ++count;
            }
          
          std::copy(vertices(g).first, 
                    vertices(g).second, 
                    std::back_inserter(vertices_by_index)
                    );
        }
      else if (v[0] == "e")
        {
          add_edge(vertices_by_index[boost::lexical_cast<long>(v[1].c_str())], 
                   vertices_by_index[boost::lexical_cast<long>(v[2].c_str())], 
                   g);

          if (num_edges_added % edge_stride == 0)
            {
              for(int i = 0;
                  i < (num_parallel_edges % max_edge_multiplicity) + 1;
                  ++i
                  )
                {
                  add_edge(vertices_by_index
                             [boost::lexical_cast<long>(v[1].c_str())], 
                           vertices_by_index
                             [boost::lexical_cast<long>(v[2].c_str())], 
                           g);
                }
              ++num_parallel_edges;
            }
          ++num_edges_added;

        }
    }
}




struct face_counter : planar_face_traversal_visitor
{

  face_counter() : m_num_faces(0) {}
  
  void begin_face() { ++m_num_faces; }

  long num_faces() { return m_num_faces; }

private:
  
  long m_num_faces;

};






int test_graph(const std::string& dimacs_filename)
{

  typedef adjacency_list<listS,
                         vecS,
                         undirectedS,
                         property<vertex_index_t, int>,
                         property<edge_index_t, int> > graph;

  typedef graph_traits<graph>::edge_descriptor edge_t;
  typedef graph_traits<graph>::edge_iterator edge_iterator_t;
  typedef graph_traits<graph>::vertex_iterator vertex_iterator_t;
  typedef graph_traits<graph>::edges_size_type e_size_t;
  typedef graph_traits<graph>::vertex_descriptor vertex_t;
  typedef edge_index_update_visitor<property_map<graph, edge_index_t>::type> 
    edge_visitor_t;

  vertex_iterator_t vi, vi_end;
  edge_iterator_t ei, ei_end;

  graph g;
  read_dimacs(g, dimacs_filename);

  // Initialize the interior edge index
  property_map<graph, edge_index_t>::type e_index = get(edge_index, g);
  e_size_t edge_count = 0;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);

  // Initialize the interior vertex index - not needed if the vertices
  // are stored with a vecS
  /*
  property_map<graph, vertex_index_t>::type v_index = get(vertex_index, g);
  v_size_t vertex_count = 0;
  for(boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    put(v_index, *vi, vertex_count++);
  */

  // This edge_updater will automatically update the interior edge
  // index of the graph as edges are created.
  edge_visitor_t edge_updater(get(edge_index, g), num_edges(g));

  // The input graph may not be maximal planar, but the Chrobak-Payne straight
  // line drawing needs a maximal planar graph as input. So, we make a copy of
  // the original graph here, then add edges to the graph to make it maximal
  // planar. When we're done creating a drawing of the maximal planar graph,
  // we can use the same mapping of vertices to points on the grid to embed the
  // original, non-maximal graph.
  graph g_copy(g);

  // Add edges to make g connected, if it isn't already
  make_connected(g, get(vertex_index, g), edge_updater);

  std::vector<graph_traits<graph>::edge_descriptor> kuratowski_edges;

  typedef std::vector< std::vector<edge_t> > edge_permutation_storage_t;
  typedef boost::iterator_property_map
    < edge_permutation_storage_t::iterator, 
      property_map<graph, vertex_index_t>::type 
    >
    edge_permutation_t;

  edge_permutation_storage_t edge_permutation_storage(num_vertices(g));
  edge_permutation_t perm(edge_permutation_storage.begin(), 
                          get(vertex_index,g)
                          );

  // Test for planarity, computing the planar embedding or the kuratowski 
  // subgraph.
  if (!boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                    boyer_myrvold_params::embedding = perm,
                                    boyer_myrvold_params::kuratowski_subgraph 
                                    = std::back_inserter(kuratowski_edges)
                                    )
      )
    {
      std::cerr << "Not planar. ";
      BOOST_REQUIRE(is_kuratowski_subgraph
                    (g, kuratowski_edges.begin(), kuratowski_edges.end())
                    );

      return 0;
    }

  // If we get this far, we have a connected planar graph.
  make_biconnected_planar(g, perm, get(edge_index, g), edge_updater);

  // Compute the planar embedding of the (now) biconnected planar graph
  BOOST_CHECK (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                            boyer_myrvold_params::embedding 
                                              = perm
                                            )
               );

  // If we get this far, we have a biconnected planar graph
  make_maximal_planar(g, perm, get(vertex_index,g), get(edge_index,g), 
                      edge_updater);
  
  // Now the graph is triangulated - we can compute the final planar embedding
  BOOST_CHECK (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                            boyer_myrvold_params::embedding 
                                              = perm
                                            )
               );

  // Make sure Euler's formula holds
  face_counter vis;
  planar_face_traversal(g, perm, vis, get(edge_index, g));

  BOOST_CHECK(num_vertices(g) - num_edges(g) + vis.num_faces() == 2);

  // Compute a planar canonical ordering of the vertices
  std::vector<vertex_t> ordering;
  planar_canonical_ordering(g, perm, std::back_inserter(ordering));

  BOOST_CHECK(ordering.size() == num_vertices(g));

  typedef std::vector< coord_t > drawing_storage_t;
  typedef boost::iterator_property_map
    < drawing_storage_t::iterator, property_map<graph, vertex_index_t>::type >
    drawing_map_t;

  drawing_storage_t drawing_vector(num_vertices(g));
  drawing_map_t drawing(drawing_vector.begin(), get(vertex_index,g));

  // Compute a straight line drawing
  chrobak_payne_straight_line_drawing(g, 
                                      perm, 
                                      ordering.begin(),
                                      ordering.end(),
                                      drawing
                                      );
  
  std::cerr << "Planar. ";
  BOOST_REQUIRE (is_straight_line_drawing(g, drawing));

  return 0;
}







int test_main(int argc, char* argv[])
{

  std::string input_directory_str = "planar_input_graphs";
  if (argc > 1)
    {
      input_directory_str = std::string(argv[1]);
    }

  std::cout << "Reading planar input files from " << input_directory_str
            << std::endl;

  filesystem::path input_directory = 
    filesystem::system_complete(filesystem::path(input_directory_str));
  const std::string dimacs_extension = ".dimacs";

  filesystem::directory_iterator dir_end;
  for( filesystem::directory_iterator dir_itr(input_directory);
       dir_itr != dir_end; ++dir_itr)
  { 

    if (dir_itr->path().extension() != dimacs_extension)
      continue;

    std::cerr << "Testing " << dir_itr->path().leaf() << "... ";
    BOOST_REQUIRE (test_graph(dir_itr->path().string()) == 0);

    std::cerr << std::endl;
  }

  return 0;

}
