//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_archetypes.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/concept/assert.hpp>
#include <string>

int main(int,char*[])
{
  using namespace boost;
  // Check const reverse_graph
  {
    typedef adjacency_list< vecS, vecS, bidirectionalS, 
      property<vertex_color_t, int>,
      property<edge_weight_t, int>,
      property<graph_name_t, std::string>
    > AdjList;
    typedef reverse_graph<AdjList> Graph;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef graph_traits<Graph>::edge_descriptor Edge;
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, Vertex, vertex_color_t> ));
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, Edge, edge_weight_t> ));
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, Edge, edge_underlying_t> ));
    AdjList g;
    Graph gr(g);
    get_property(gr, graph_name_t());
  }
  // Check non-const reverse_graph
  {
    typedef adjacency_list< vecS, vecS, bidirectionalS, 
      property<vertex_color_t, int>,
      property<edge_weight_t, int>,
      property<graph_name_t, std::string>
    > AdjList;
    typedef reverse_graph<AdjList,AdjList&> Graph;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef graph_traits<Graph>::edge_descriptor Edge;
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Vertex, vertex_color_t> ));
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Edge, edge_weight_t> ));
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, Edge, edge_underlying_t> ));
    AdjList g;
    Graph gr(g);
    get_property(gr, graph_name_t());
    set_property(gr, graph_name_t(), "foo");
  }
  return 0;
}
