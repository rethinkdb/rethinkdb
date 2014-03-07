//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <boost/concept/assert.hpp>
#include <vector>
#include <list>

// THIS FILE MUST PRECEDE ALL OTHER BOOST GRAPH FILES
// Due to ADL nastiness involving the vertices() function
#include <boost/graph/vector_as_graph.hpp>
// THIS FILE MUST PRECEDE ALL OTHER BOOST GRAPH FILES

#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_archetypes.hpp>

int main(int,char*[])
{
  using namespace boost;
  // Check "vector as graph"
  {
    typedef std::vector< std::list<int> > Graph;
    BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
    BOOST_CONCEPT_ASSERT(( AdjacencyGraphConcept<Graph> ));
  }
  return 0;
}
