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
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/concept/assert.hpp>

int main(int,char*[])
{
  using namespace boost;
  // Check adjacency_matrix without properties
  {
    typedef adjacency_matrix<directedS> Graph;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( MutableGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyMatrixConcept<Graph> ));
  }
  {
    typedef adjacency_matrix<undirectedS> Graph;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( MutableGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyMatrixConcept<Graph> ));
  }
  // Check adjacency_matrix with properties
  {
    typedef adjacency_matrix<directedS, 
      property<vertex_color_t, int>,
      property<edge_weight_t, float> > Graph;
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef graph_traits<Graph>::edge_descriptor Edge;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyMatrixConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( VertexMutablePropertyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeMutablePropertyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, 
      Vertex, vertex_index_t> ));
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Vertex, vertex_color_t> ));
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Edge, edge_weight_t> ));
  }
  {
    typedef adjacency_matrix<undirectedS, 
      property<vertex_color_t, int>,
      property<edge_weight_t, float> > Graph;
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef graph_traits<Graph>::edge_descriptor Edge;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyMatrixConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( VertexMutablePropertyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( EdgeMutablePropertyGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<Graph, 
      Vertex, vertex_index_t> ));
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Vertex, vertex_color_t> ));
    BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph, Edge, edge_weight_t> ));
  }
  return 0;
}
