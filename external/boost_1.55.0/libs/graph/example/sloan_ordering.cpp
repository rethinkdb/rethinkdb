//
//=======================================================================
// Copyright 2002 Marc Wintermantel (wintermantel@imes.mavt.ethz.ch)
// ETH Zurich, Center of Structure Technologies (www.imes.ethz.ch/st)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//


#include <boost/config.hpp>
#include <vector>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/sloan_ordering.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/bandwidth.hpp>
#include <boost/graph/profile.hpp>
#include <boost/graph/wavefront.hpp>


using std::cout;
using std::endl;

/*
  Sample Output
  #####################################
  ### First light of sloan-ordering ###
  #####################################

  original bandwidth: 8
  original profile: 42
  original max_wavefront: 7
  original aver_wavefront: 4.2
  original rms_wavefront: 4.58258

  Starting vertex: 0
  Pseudoperipheral vertex: 9
  Pseudoperipheral radius: 4

  Sloan ordering starting at: 0
    0 8 3 7 5 2 4 6 1 9
    bandwidth: 4
    profile: 28
    max_wavefront: 4
    aver_wavefront: 2.8
    rms_wavefront: 2.93258

  Sloan ordering without a start-vertex:
    8 0 3 7 5 2 4 6 1 9
    bandwidth: 4
    profile: 27
    max_wavefront: 4
    aver_wavefront: 2.7
    rms_wavefront: 2.84605

  ###############################
  ### sloan-ordering finished ###
  ###############################
*/

int main(int , char* [])
{
  cout << endl;  
  cout << "#####################################" << endl; 
  cout << "### First light of sloan-ordering ###" << endl;
  cout << "#####################################" << endl << endl;

  using namespace boost;
  using namespace std;
 

  //Defining the graph type 
  typedef adjacency_list<
    setS, 
    vecS, 
    undirectedS, 
    property<
    vertex_color_t, 
    default_color_type,
    property<
    vertex_degree_t,
    int,
    property<
    vertex_priority_t,
    double > > > > Graph;
  
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
 
  
  //Creating a graph and adding the edges from above into it
  Graph G(10);
  for (int i = 0; i < 14; ++i)
    add_edge(edges[i].first, edges[i].second, G);

  //Creating two iterators over the vertices
  graph_traits<Graph>::vertex_iterator ui, ui_end;

  //Creating a property_map with the degrees of the degrees of each vertex
  property_map<Graph,vertex_degree_t>::type deg = get(vertex_degree, G);
  for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
    deg[*ui] = degree(*ui, G);

  //Creating a property_map for the indices of a vertex
  property_map<Graph, vertex_index_t>::type index_map = get(vertex_index, G);

  std::cout << "original bandwidth: " << bandwidth(G) << std::endl;
  std::cout << "original profile: " << profile(G) << std::endl;
  std::cout << "original max_wavefront: " << max_wavefront(G) << std::endl;
  std::cout << "original aver_wavefront: " << aver_wavefront(G) << std::endl;
  std::cout << "original rms_wavefront: " << rms_wavefront(G) << std::endl;
  

  //Creating a vector of vertices  
  std::vector<Vertex> sloan_order(num_vertices(G));
  //Creating a vector of size_type  
  std::vector<size_type> perm(num_vertices(G));

  {
    
    //Setting the start node
    Vertex s = vertex(0, G);
    int ecc;   //defining a variable for the pseudoperipheral radius
    
    //Calculating the pseudoeperipheral node and radius
    Vertex e = pseudo_peripheral_pair(G, s, ecc, get(vertex_color, G), get(vertex_degree, G) );

    cout << endl;
    cout << "Starting vertex: " << s << endl;
    cout << "Pseudoperipheral vertex: " << e << endl;
    cout << "Pseudoperipheral radius: " << ecc << endl << endl;



    //Sloan ordering
    sloan_ordering(G, s, e, sloan_order.begin(), get(vertex_color, G), 
                           get(vertex_degree, G), get(vertex_priority, G));
    
    cout << "Sloan ordering starting at: " << s << endl;
    cout << "  ";    
    
    for (std::vector<Vertex>::const_iterator i = sloan_order.begin();
         i != sloan_order.end(); ++i)
      cout << index_map[*i] << " ";
    cout << endl;

    for (size_type c = 0; c != sloan_order.size(); ++c)
      perm[index_map[sloan_order[c]]] = c;
    std::cout << "  bandwidth: " 
              << bandwidth(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
    std::cout << "  profile: " 
              << profile(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
    std::cout << "  max_wavefront: " 
              << max_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
    std::cout << "  aver_wavefront: " 
              << aver_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
    std::cout << "  rms_wavefront: " 
              << rms_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
              << std::endl;
  }
  



    /////////////////////////////////////////////////
    //Version including finding a good starting point
    /////////////////////////////////////////////////
   
    {
      //sloan_ordering
      sloan_ordering(G, sloan_order.begin(), 
                        get(vertex_color, G),
                        make_degree_map(G), 
                        get(vertex_priority, G) );
      
      cout << endl << "Sloan ordering without a start-vertex:" << endl;
      cout << "  ";
      for (std::vector<Vertex>::const_iterator i=sloan_order.begin();
           i != sloan_order.end(); ++i)
        cout << index_map[*i] << " ";
      cout << endl;
      
      for (size_type c = 0; c != sloan_order.size(); ++c)
        perm[index_map[sloan_order[c]]] = c;
      std::cout << "  bandwidth: " 
                << bandwidth(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
                << std::endl;
      std::cout << "  profile: " 
                << profile(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
                << std::endl;
      std::cout << "  max_wavefront: " 
                << max_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
                << std::endl;
      std::cout << "  aver_wavefront: " 
                << aver_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
                << std::endl;
      std::cout << "  rms_wavefront: " 
                << rms_wavefront(G, make_iterator_property_map(&perm[0], index_map, perm[0]))
                << std::endl;
    }
  

  
  cout << endl;
  cout << "###############################" << endl;
  cout << "### sloan-ordering finished ###" << endl;
  cout << "###############################" << endl << endl;
  return 0;

}
