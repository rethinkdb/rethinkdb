//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/concept_archetype.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_archetypes.hpp>

int main()
{
  using namespace boost;
  typedef default_constructible_archetype< 
    sgi_assignable_archetype<
    equality_comparable_archetype<> > > vertex_t;
  {
    typedef incidence_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag, IncidenceGraph> graph_t;
    graph_t& g = static_object<graph_t>::get();
    read_write_property_map_archetype<vertex_t, color_value_archetype> color;
    depth_first_search(g, color_map(color));
  }
  {
    typedef incidence_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag, IncidenceGraph> graph_t;
    graph_t& g = static_object<graph_t>::get();
    readable_property_map_archetype<vertex_t, std::size_t> v_index;
    depth_first_search(g, vertex_index_map(v_index));
  }
  {
    typedef incidence_graph_archetype<vertex_t, undirected_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, undirected_tag, 
      allow_parallel_edge_tag, IncidenceGraph> Graph;
    typedef property_graph_archetype<Graph, vertex_index_t, std::size_t> 
      graph_t;
    graph_t& g = static_object<graph_t>::get();
    dfs_visitor<> v;
    depth_first_search(g, visitor(v));
  }
  return 0;
}
