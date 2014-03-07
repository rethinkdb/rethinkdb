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
#include <boost/graph/edge_list.hpp>
#include <boost/concept/assert.hpp>
#include <cstddef>
#include <iterator>

int main(int,char*[])
{
    // Check edge_list
    {
        using namespace boost;
        
        typedef std::pair<int,int> E;
    
        typedef edge_list<E*,E,std::ptrdiff_t,std::random_access_iterator_tag> EdgeList;
    
        typedef graph_traits<EdgeList>::edge_descriptor Edge;
    
        BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<EdgeList> ));
    
        BOOST_CONCEPT_ASSERT(( ReadablePropertyGraphConcept<EdgeList, Edge, 
            edge_index_t> ));
    }
    return 0;
}
