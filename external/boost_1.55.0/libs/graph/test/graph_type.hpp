//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/adjacency_list.hpp>
typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, boost::property<vertex_id_t, std::size_t>, boost::property<edge_id_t, std::size_t> > Graph;
typedef boost::property<vertex_id_t, std::size_t> VertexId;
typedef boost::property<edge_id_t, std::size_t> EdgeID;
