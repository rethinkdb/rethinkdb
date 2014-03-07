//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/leda_graph.hpp>
#include <boost/concept/assert.hpp>

int
main()
{
  using namespace boost;
  typedef leda::GRAPH<int, int> Graph;
  BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
  BOOST_CONCEPT_ASSERT(( BidirectionalGraphConcept< Graph> ));
  BOOST_CONCEPT_ASSERT(( VertexMutableGraphConcept< Graph> ));
  BOOST_CONCEPT_ASSERT(( EdgeMutableGraphConcept<Graph> ));
  return EXIT_SUCCESS;
}
