//=======================================================================
// Copyright 2008 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/property_map/property_map.hpp>
#include <boost/test/minimal.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/is_straight_line_drawing.hpp>

#include <vector>

using namespace boost;

struct coord_t 
{
  std::size_t x;
  std::size_t y;
};


int test_main(int, char*[]) 
{
  typedef adjacency_list< vecS, vecS, undirectedS, 
                          property<vertex_index_t, int> 
                         > graph_t;

  typedef std::vector< coord_t > drawing_storage_t;

  typedef boost::iterator_property_map 
      < drawing_storage_t::iterator,
        property_map<graph_t, vertex_index_t>::type
       > drawing_t;

  graph_t g(4);
  add_edge(0,1,g);
  add_edge(2,3,g);

  drawing_storage_t drawing_storage(num_vertices(g));
  drawing_t drawing(drawing_storage.begin(), get(vertex_index,g));

  // two perpendicular lines that intersect at (1,1)
  drawing[0].x = 1; drawing[0].y = 0;
  drawing[1].x = 1; drawing[1].y = 2;
  drawing[2].x = 0; drawing[2].y = 1;
  drawing[3].x = 2; drawing[3].y = 1;

  BOOST_REQUIRE(!is_straight_line_drawing(g,drawing));

  // two parallel horizontal lines
  drawing[0].x = 0; drawing[0].y = 0;
  drawing[1].x = 2; drawing[1].y = 0;

  BOOST_REQUIRE(is_straight_line_drawing(g,drawing));
  
  // two parallel vertical lines
  drawing[0].x = 0; drawing[0].y = 0;
  drawing[1].x = 0; drawing[1].y = 2;
  drawing[2].x = 1; drawing[2].y = 0;
  drawing[3].x = 1; drawing[3].y = 2;

  BOOST_REQUIRE(is_straight_line_drawing(g,drawing));

  // two lines that intersect at (1,1)
  drawing[0].x = 0; drawing[0].y = 0;
  drawing[1].x = 2; drawing[1].y = 2;
  drawing[2].x = 0; drawing[2].y = 2;
  drawing[3].x = 2; drawing[3].y = 0;

  BOOST_REQUIRE(!is_straight_line_drawing(g,drawing));
  
  // K_4 arranged in a diamond pattern, so that edges intersect
  g = graph_t(4); 
  add_edge(0,1,g);  
  add_edge(0,2,g);  
  add_edge(0,3,g);
  add_edge(1,2,g);  
  add_edge(1,3,g);
  add_edge(2,3,g);    
  
  drawing_storage = drawing_storage_t(num_vertices(g));
  drawing = drawing_t(drawing_storage.begin(), get(vertex_index,g));

  drawing[0].x = 1; drawing[0].y = 2;
  drawing[1].x = 2; drawing[1].y = 1;
  drawing[2].x = 1; drawing[2].y = 0;
  drawing[3].x = 0; drawing[3].y = 1;

  BOOST_REQUIRE(!is_straight_line_drawing(g, drawing));

  // K_4 arranged so that no edges intersect
  drawing[0].x = 0; drawing[0].y = 0;
  drawing[1].x = 1; drawing[1].y = 1;
  drawing[2].x = 1; drawing[2].y = 2;
  drawing[3].x = 2; drawing[3].y = 0;

  BOOST_REQUIRE(is_straight_line_drawing(g, drawing));

  // a slightly more complicated example - edges (0,1) and (4,5)
  // intersect
  g = graph_t(8); 
  add_edge(0,1,g);  
  add_edge(2,3,g);  
  add_edge(4,5,g);
  add_edge(6,7,g);  
  
  drawing_storage = drawing_storage_t(num_vertices(g));
  drawing = drawing_t(drawing_storage.begin(), get(vertex_index,g));

  drawing[0].x = 1; drawing[0].y = 1;
  drawing[1].x = 5; drawing[1].y = 4;
  drawing[2].x = 2; drawing[2].y = 5;
  drawing[3].x = 4; drawing[3].y = 4;
  drawing[4].x = 3; drawing[4].y = 4;
  drawing[5].x = 3; drawing[5].y = 2;
  drawing[6].x = 4; drawing[6].y = 2;
  drawing[7].x = 1; drawing[7].y = 1;

  BOOST_REQUIRE(!is_straight_line_drawing(g, drawing));
  
  // form a graph consisting of a bunch of parallel vertical edges,
  // then place an edge at various positions to intersect edges
  g = graph_t(22);
  for(int i = 0; i < 11; ++i)
    add_edge(2*i,2*i+1,g);

  drawing_storage = drawing_storage_t(num_vertices(g));
  drawing = drawing_t(drawing_storage.begin(), get(vertex_index,g));

  for(int i = 0; i < 10; ++i)
    {
      drawing[2*i].x = i; drawing[2*i].y = 0;
      drawing[2*i+1].x = i; drawing[2*i+1].y = 10;
    }
  
  // put the final edge as a horizontal edge intersecting one other edge
  drawing[20].x = 5; drawing[20].y = 5;
  drawing[21].x = 7; drawing[21].y = 5;

  BOOST_REQUIRE(!is_straight_line_drawing(g, drawing));

  // make the final edge a diagonal intersecting multiple edges
  drawing[20].x = 2; drawing[20].y = 4;
  drawing[21].x = 9; drawing[21].y = 7;

  BOOST_REQUIRE(!is_straight_line_drawing(g, drawing));

  // reverse the slope
  drawing[20].x = 2; drawing[20].y = 7;
  drawing[21].x = 9; drawing[21].y = 4;

  BOOST_REQUIRE(!is_straight_line_drawing(g, drawing));

  return 0;
}

