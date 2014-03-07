//=======================================================================
// Copyright 2001 University of Notre Dame.
// Authors: Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <stdio.h>
#include <iostream>
#include <boost/graph/stanford_graph.hpp>
#include <boost/graph/strong_components.hpp>

#define specs(v) \
 (filename ? index_map[v] : v->cat_no) << " " << v->name

int main(int argc, char* argv[])
{
  using namespace boost;
  Graph* g;
  typedef graph_traits<Graph*>::vertex_descriptor vertex_t;
  unsigned long n = 0;
  unsigned long d = 0;
  unsigned long p = 0;
  long s = 0;
  char* filename = NULL;
  int c, i;

  while (--argc) {
    if (sscanf(argv[argc], "-n%lu", &n) == 1);
    else if (sscanf(argv[argc], "-d%lu", &d) == 1);
    else if (sscanf(argv[argc], "-p%lu", &p) == 1);
    else if (sscanf(argv[argc], "-s%ld", &s) == 1);
    else if (strncmp(argv[argc], "-g", 2) == 0)
      filename = argv[argc] + 2;
    else {
      fprintf(stderr, "Usage: %s [-nN][-dN][-pN][-sN][-gfoo]\n", argv[0]);
      return -2;
    }
  }

  g = (filename ? restore_graph(filename) : roget(n, d, p, s));
  if (g == NULL) {
    fprintf(stderr, "Sorry, can't create the graph! (error code %ld)\n",
            panic_code);
    return -1;
  }
  printf("Reachability analysis of %s\n\n", g->id);

  // - The root map corresponds to what Knuth calls the "min" field.
  // - The discover time map is the "rank" field
  // - Knuth uses the rank field for double duty, to record the
  //   discover time, and to keep track of which vertices have
  //   been visited. The BGL strong_components() function needs
  //   a separate field for marking colors, so we use the w field.

  std::vector<int> comp(num_vertices(g));
  property_map<Graph*, vertex_index_t>::type 
    index_map = get(vertex_index, g);

  property_map<Graph*, v_property<vertex_t> >::type 
    root = get(v_property<vertex_t>(), g);

  int num_comp = strong_components
    (g, make_iterator_property_map(comp.begin(), index_map),
     root_map(root).
     discover_time_map(get(z_property<long>(), g)).
     color_map(get(w_property<long>(), g)));

  std::vector< std::vector<vertex_t> > strong_comp(num_comp);

  // First add representative vertices to each component's list
  graph_traits<Graph*>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (root[*vi] == *vi)
      strong_comp[comp[index_map[*vi]]].push_back(*vi);

  // Then add the other vertices of the component
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (root[*vi] != *vi)
      strong_comp[comp[index_map[*vi]]].push_back(*vi);

  // We do not print out the "from" and "to" information as Knuth
  // does because we no longer have easy access to that information
  // from outside the algorithm.

  for (c = 0; c < num_comp; ++c) {
    vertex_t v = strong_comp[c].front();
    std::cout << "Strong component `" << specs(v) << "'";
    if (strong_comp[c].size() > 1) {
      std::cout << " also includes:\n";
      for (i = 1; i < strong_comp[c].size(); ++i)
        std::cout << " " << specs(strong_comp[c][i]) << std::endl;
    } else
      std::cout << std::endl;
  }

  // Next we print out the "component graph" or "condensation", that
  // is, we consider each component to be a vertex in a new graph
  // where there is an edge connecting one component to another if there
  // is one or more edges connecting any of the vertices from the
  // first component to any of the vertices in the second. We use the
  // name of the representative vertex as the name of the component.

  printf("\nLinks between components:\n");

  // This array provides an efficient way to check if we've already
  // created a link from the current component to the component
  // of the target vertex.
  std::vector<int> mark(num_comp, (std::numeric_limits<int>::max)());

  // We go in reverse order just to mimic the output ordering in
  // Knuth's version.
  for (c = num_comp - 1; c >= 0; --c) {
    vertex_t u = strong_comp[c][0];
    for (i = 0; i < strong_comp[c].size(); ++i) {
      vertex_t v = strong_comp[c][i];
      graph_traits<Graph*>::out_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) = out_edges(v, g); ei != ei_end; ++ei) {
        vertex_t x = target(*ei, g);
        int comp_x = comp[index_map[x]];
        if (comp_x != c && mark[comp_x] != c) {
          mark[comp_x] = c;
          vertex_t w = strong_comp[comp_x][0];
          std::cout << specs(u) << " -> " << specs(w)
                    << " (e.g., " << specs(v) << " -> " << specs(x) << ")\n";
        } // if
      } // for
    } // for
  } // for
}
