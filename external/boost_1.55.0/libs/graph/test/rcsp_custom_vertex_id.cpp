//=======================================================================
// Copyright (c) 2013 Alberto Santini
// Author: Alberto Santini <alberto@santini.in>
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/r_c_shortest_paths.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <vector>
#include <memory>

using std::vector;
using std::allocator;
using namespace boost;

class VertexProperty {
public:
  int property1;
  int property2;
  int id;
  VertexProperty() {}
  VertexProperty(const int property1, const int property2) : property1(property1), property2(property2) {}
};

class EdgeProperty {
public:
  int cost;
  int id;
  EdgeProperty() {}
  EdgeProperty(const int cost) : cost(cost) {}
};

typedef adjacency_list<listS, listS, bidirectionalS, VertexProperty, EdgeProperty> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

class ResourceCont {
public:
  int res;
  ResourceCont(const int res = 5) : res(res) {}
  bool operator==(const ResourceCont& rc) const { return (res == rc.res); }
  bool operator<(const ResourceCont& rc) const { return (res > rc.res); }
};

class LabelExt {
public:
  bool operator()(const Graph& g, ResourceCont& rc, const ResourceCont& old_rc, Edge e) const {
    rc.res = old_rc.res - g[e].cost;
    return (rc.res > 0);
  }
};

class LabelDom {
public:
  bool operator()(const ResourceCont& rc1, const ResourceCont& rc2) const {
    return (rc1 == rc2 || rc1 < rc2);
  }
};

int main() {
  VertexProperty vp1(1, 1);
  VertexProperty vp2(5, 9);
  VertexProperty vp3(4, 3);
  EdgeProperty e12(1);
  EdgeProperty e23(2);
        
  Graph g;
        
  Vertex v1 = add_vertex(g); g[v1] = vp1;
  Vertex v2 = add_vertex(g); g[v2] = vp2;
  Vertex v3 = add_vertex(g); g[v3] = vp3;
  
  add_edge(v1, v2, e12, g);
  add_edge(v2, v3, e23, g);
    
  int index = 0;
  BGL_FORALL_VERTICES(v, g, Graph) {
    g[v].id = index++;
  }
  index = 0;
  BGL_FORALL_EDGES(e, g, Graph) {
    g[e].id = index++;
  }
  
  typedef vector<vector<Edge> > OptPath;
  typedef vector<ResourceCont> ParetoOpt;
  
  OptPath op;
  ParetoOpt ol;
  
  r_c_shortest_paths( g,
                      get(&VertexProperty::id, g),
                      get(&EdgeProperty::id, g),
                      v1,
                      v2,
                      op,
                      ol,
                      ResourceCont(5),
                      LabelExt(),
                      LabelDom(),
                      allocator<r_c_shortest_paths_label<Graph, ResourceCont> >(),
                      default_r_c_shortest_paths_visitor());
  
  return 0;
}
