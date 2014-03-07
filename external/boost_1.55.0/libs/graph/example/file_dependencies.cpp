//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// Some small modifications are done by Alexander Holler

/*

  Paul Moore's request:

  As an example of a practical problem which is not restricted to graph
  "experts", consider file dependencies. It's basically graph construction,
  plus topological sort, but it might make a nice "tutorial" example. Build a
  dependency graph of files, then use the algorithms to do things like

  1. Produce a full recompilation order (topological sort, by modified date)
  2. Produce a "parallel" recompilation order (same as above, but group files
  which can be built in parallel)
  3. Change analysis (if I change file x, which others need recompiling)
  4. Dependency changes (if I add a dependency between file x and file y, what
  are the effects)

*/

#include <boost/config.hpp> // put this first to suppress some VC++ warnings

#include <iostream>
#include <iterator>
#include <algorithm>
#include <time.h>

#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>

using namespace std;
using namespace boost;

enum files_e { dax_h, yow_h, boz_h, zow_h, foo_cpp, 
               foo_o, bar_cpp, bar_o, libfoobar_a,
               zig_cpp, zig_o, zag_cpp, zag_o, 
                 libzigzag_a, killerapp, N };
const char* name[] = { "dax.h", "yow.h", "boz.h", "zow.h", "foo.cpp",
                       "foo.o", "bar.cpp", "bar.o", "libfoobar.a",
                       "zig.cpp", "zig.o", "zag.cpp", "zag.o",
                       "libzigzag.a", "killerapp" };


struct print_visitor : public bfs_visitor<> {
  template <class Vertex, class Graph>
  void discover_vertex(Vertex v, Graph&) {
    cout << name[v] << " ";
  }
};


struct cycle_detector : public dfs_visitor<>
{
  cycle_detector(bool& has_cycle) 
    : m_has_cycle(has_cycle) { }

  template <class Edge, class Graph>
  void back_edge(Edge, Graph&) { m_has_cycle = true; }
protected:
  bool& m_has_cycle;
};




int main(int,char*[])
{

  typedef pair<int,int> Edge;
  Edge used_by[] = {
    Edge(dax_h, foo_cpp), Edge(dax_h, bar_cpp), Edge(dax_h, yow_h),
    Edge(yow_h, bar_cpp), Edge(yow_h, zag_cpp),
    Edge(boz_h, bar_cpp), Edge(boz_h, zig_cpp), Edge(boz_h, zag_cpp),
    Edge(zow_h, foo_cpp), 
    Edge(foo_cpp, foo_o),
    Edge(foo_o, libfoobar_a),
    Edge(bar_cpp, bar_o),
    Edge(bar_o, libfoobar_a),
    Edge(libfoobar_a, libzigzag_a),
    Edge(zig_cpp, zig_o),
    Edge(zig_o, libzigzag_a),
    Edge(zag_cpp, zag_o),
    Edge(zag_o, libzigzag_a),
    Edge(libzigzag_a, killerapp)
  };
  const std::size_t nedges = sizeof(used_by)/sizeof(Edge);

  typedef adjacency_list<vecS, vecS, bidirectionalS> Graph;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle the iterator constructor
  Graph g(N);
  for (std::size_t j = 0; j < nedges; ++j) {
    graph_traits<Graph>::edge_descriptor e; bool inserted;
    boost::tie(e, inserted) = add_edge(used_by[j].first, used_by[j].second, g);
  }
#else
  Graph g(used_by, used_by + nedges, N);
#endif
  typedef graph_traits<Graph>::vertex_descriptor Vertex;

  // Determine ordering for a full recompilation
  // and the order with files that can be compiled in parallel
  {
    typedef list<Vertex> MakeOrder;
    MakeOrder::iterator i;
    MakeOrder make_order;

    topological_sort(g, std::front_inserter(make_order));
    cout << "make ordering: ";
    for (i = make_order.begin();
         i != make_order.end(); ++i) 
      cout << name[*i] << " ";
  
    cout << endl << endl;

    // Parallel compilation ordering
    std::vector<int> time(N, 0);
    for (i = make_order.begin(); i != make_order.end(); ++i) {    
      // Walk through the in_edges an calculate the maximum time.
      if (in_degree (*i, g) > 0) {
        Graph::in_edge_iterator j, j_end;
        int maxdist=0;
        // Through the order from topological sort, we are sure that every 
        // time we are using here is already initialized.
        for (boost::tie(j, j_end) = in_edges(*i, g); j != j_end; ++j)
          maxdist=(std::max)(time[source(*j, g)], maxdist);
        time[*i]=maxdist+1;
      }
    }

    cout << "parallel make ordering, " << endl
         << "vertices with same group number can be made in parallel" << endl;
    {
      graph_traits<Graph>::vertex_iterator i, iend;
      for (boost::tie(i,iend) = vertices(g); i != iend; ++i)
        cout << "time_slot[" << name[*i] << "] = " << time[*i] << endl;
    }

  }
  cout << endl;

  // if I change yow.h what files need to be re-made?
  {
    cout << "A change to yow.h will cause what to be re-made?" << endl;
    print_visitor vis;
    breadth_first_search(g, vertex(yow_h, g), visitor(vis));
    cout << endl;
  }
  cout << endl;

  // are there any cycles in the graph?
  {
    bool has_cycle = false;
    cycle_detector vis(has_cycle);
    depth_first_search(g, visitor(vis));
    cout << "The graph has a cycle? " << has_cycle << endl;
  }
  cout << endl;

  // add a dependency going from bar.cpp to dax.h
  {
    cout << "adding edge bar_cpp -> dax_h" << endl;
    add_edge(bar_cpp, dax_h, g);
  }
  cout << endl;

  // are there any cycles in the graph?
  {
    bool has_cycle = false;
    cycle_detector vis(has_cycle);
    depth_first_search(g, visitor(vis));
    cout << "The graph has a cycle now? " << has_cycle << endl;
  }

  return 0;
}
