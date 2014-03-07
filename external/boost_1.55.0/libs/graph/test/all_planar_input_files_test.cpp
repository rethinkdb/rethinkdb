//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

/*

This test looks in the directory "planar_input_graphs" for any files
of the form *.dimacs. Each such file is used to create an input graph
and test the input graph for planarity. If the graph is planar, a
straight line drawing is generated and verified. If the graph isn't
planar, a kuratowski subgraph is isolated and verified.

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
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  std::vector<vertex_t> vertices_by_index;
  
  std::ifstream in(filename.c_str());
  
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
          g = Graph(boost::lexical_cast<std::size_t>(v[2].c_str()));
          std::copy(vertices(g).first, 
                    vertices(g).second, 
                    std::back_inserter(vertices_by_index)
                    );
        }
      else if (v[0] == "e")
        {
          add_edge(vertices_by_index
                     [boost::lexical_cast<std::size_t>(v[1].c_str())], 
                   vertices_by_index
                     [boost::lexical_cast<std::size_t>(v[2].c_str())], 
                   g);
        }
    }
}






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
      std::cout << "Not planar. ";
      BOOST_REQUIRE(is_kuratowski_subgraph(g, 
                                           kuratowski_edges.begin(), 
                                           kuratowski_edges.end()
                                           )
                    );

      return 0;
    }

  // If we get this far, we have a connected planar graph.
  make_biconnected_planar(g, perm, get(edge_index, g), edge_updater);

  // Compute the planar embedding of the (now) biconnected planar graph
  BOOST_CHECK (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                            boyer_myrvold_params::embedding = 
                                              perm
                                            )
               );

  // If we get this far, we have a biconnected planar graph
  make_maximal_planar(g, perm, get(vertex_index,g), get(edge_index,g), 
                      edge_updater
                      );
  
  // Now the graph is triangulated - we can compute the final planar embedding
  BOOST_CHECK (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                            boyer_myrvold_params::embedding = 
                                              perm
                                            )
               );

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
  
  std::cout << "Planar. ";
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

    std::cout << "Testing " << dir_itr->path().leaf() << "... ";
    BOOST_REQUIRE (test_graph(dir_itr->path().string()) == 0);

    std::cout << std::endl;
  }

  return 0;

}
