//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

struct vertex_info_t { };
struct edge_info_t { };
namespace boost {
  BOOST_INSTALL_PROPERTY(vertex, info);
  BOOST_INSTALL_PROPERTY(edge, info);
};

typedef property<vertex_info_t, double> vertex_properties;
typedef property<edge_info_t, double> edge_properties;

typedef adjacency_list<vecS, vecS, bidirectionalS,
vertex_properties, edge_properties> graph_t;

double& foo_1(graph_t& x)
{
  property_map<graph_t, vertex_info_t>::type pmap
    = get(vertex_info_t(), x);
  return pmap[vertex(0, x)];
}

const double& foo_2(graph_t const & x)
{
  property_map<graph_t, vertex_info_t>::const_type pmap
    = get(vertex_info_t(), x);
  return pmap[vertex(0, x)];
}

double& bar_1(graph_t& x)
{
  property_map<graph_t, edge_info_t>::type pmap
    = get(edge_info_t(), x);
  return pmap[edge(vertex(0, x), vertex(1, x), x).first];
}

const double& bar_2(graph_t const & x)
{
  property_map<graph_t, edge_info_t>::const_type pmap
    = get(edge_info_t(), x);
  return pmap[edge(vertex(0, x), vertex(1, x), x).first];
}
      
int
main()
{
  return 0;
}
