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
#include <boost/concept/assert.hpp>

int main(int,char*[])
{
  using namespace boost;

  // Check graph concepts againt their archetypes
  typedef default_constructible_archetype<
    sgi_assignable_archetype< equality_comparable_archetype<> > > Vertex;

  typedef incidence_graph_archetype<Vertex, directed_tag, 
    allow_parallel_edge_tag> Graph1;
  BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph1> ));

  typedef adjacency_graph_archetype<Vertex, directed_tag, 
    allow_parallel_edge_tag> Graph2;
  BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph2> ));

  typedef vertex_list_graph_archetype<Vertex, directed_tag, 
    allow_parallel_edge_tag> Graph3;
  BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph3> ));

  BOOST_CONCEPT_ASSERT(( ColorValueConcept<color_value_archetype> ));

  typedef incidence_graph_archetype<Vertex, directed_tag, allow_parallel_edge_tag> G;
  typedef property_graph_archetype<G, vertex_color_t, color_value_archetype>
    Graph4;
  BOOST_CONCEPT_ASSERT(( PropertyGraphConcept<Graph4, Vertex, vertex_color_t> ));

  return 0;
}
