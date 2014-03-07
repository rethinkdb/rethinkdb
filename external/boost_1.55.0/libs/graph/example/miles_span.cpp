//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// Sample output:
//
//  The graph miles(100,0,0,0,0,10,0) has 405 edges,
//   and its minimum spanning tree has length 14467.
//

#include <boost/config.hpp>
#include <string.h>
#include <stdio.h>
#include <boost/graph/stanford_graph.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>

// A visitor class for accumulating the total length of the minimum
// spanning tree. The Distance template parameter is for a
// PropertyMap.
template <class Distance>
struct total_length_visitor : public boost::dijkstra_visitor<> {
  typedef typename boost::property_traits<Distance>::value_type D;
  total_length_visitor(D& len, Distance d)
    : _total_length(len), _distance(d) { }
  template <class Vertex, class Graph>
  inline void finish_vertex(Vertex s, Graph& g) {
    _total_length += boost::get(_distance, s); 
  }
  D& _total_length;
  Distance _distance;
};

int main(int argc, char* argv[])
{
  using namespace boost;
  Graph* g;

  unsigned long n = 100;
  unsigned long n_weight = 0;
  unsigned long w_weight = 0;
  unsigned long p_weight = 0;
  unsigned long d = 10;
  long s = 0;
  unsigned long r = 1;
  char* file_name = NULL;

  while(--argc){
    if(sscanf(argv[argc],"-n%lu",&n)==1);
    else if(sscanf(argv[argc],"-N%lu",&n_weight)==1);
    else if(sscanf(argv[argc],"-W%lu",&w_weight)==1);
    else if(sscanf(argv[argc],"-P%lu",&p_weight)==1);
    else if(sscanf(argv[argc],"-d%lu",&d)==1);
    else if(sscanf(argv[argc],"-r%lu",&r)==1);
    else if(sscanf(argv[argc],"-s%ld",&s)==1);
    else if(strcmp(argv[argc],"-v")==0) verbose = 1;
    else if(strncmp(argv[argc],"-g",2)==0) file_name = argv[argc]+2;
    else{
      fprintf(stderr,
              "Usage: %s [-nN][-dN][-rN][-sN][-NN][-WN][-PN][-v][-gfoo]\n",
              argv[0]);
      return -2;
    }
  }
  if (file_name) r = 1;

  while (r--) {
    if (file_name)
      g = restore_graph(file_name);
    else
      g = miles(n,n_weight,w_weight,p_weight,0L,d,s);

    if(g == NULL || g->n <= 1) {
      fprintf(stderr,"Sorry, can't create the graph! (error code %ld)\n",
              panic_code);
      return-1;
    }

   printf("The graph %s has %ld edges,\n", g->id, g->m / 2);

   long sp_length = 0;

   // Use the "z" utility field for distance.
   typedef property_map<Graph*, z_property<long> >::type Distance;
   Distance d = get(z_property<long>(), g);
   // Use the "w" property for parent
   typedef property_map<Graph*, w_property<Vertex*> >::type Parent;
   Parent p = get(w_property<Vertex*>(), g);
   total_length_visitor<Distance> length_vis(sp_length, d);

   prim_minimum_spanning_tree(g, p,
                              distance_map(get(z_property<long>(), g)).
                              weight_map(get(edge_length_t(), g)). 
                              // Use the "y" utility field for color
                              color_map(get(y_property<long>(), g)).
                              visitor(length_vis));

   printf("  and its minimum spanning tree has length %ld.\n", sp_length);

   gb_recycle(g);
   s++;
 }
  return 0;
}
