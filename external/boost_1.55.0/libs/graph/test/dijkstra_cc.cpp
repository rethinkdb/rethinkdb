//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graph_archetypes.hpp>

typedef boost::default_constructible_archetype<
  boost::sgi_assignable_archetype<> > dist_value;

// So generate_infinity works...
namespace std {
  template <>
  class numeric_limits<dist_value> {
  public:
    static dist_value max BOOST_PREVENT_MACRO_SUBSTITUTION () {
      return boost::static_object<dist_value>::get(); 
    }
  };
}

dist_value abs(const dist_value& x) { return x; }
std::size_t abs(std::size_t x) { return x; }

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
    vertex_t s;
    typedef graph_traits<graph_t>::edge_descriptor edge_t;
    readable_property_map_archetype<edge_t, std::size_t> weight;
    readable_property_map_archetype<vertex_t, int> index;
    read_write_property_map_archetype<vertex_t, std::size_t> distance;
    dijkstra_shortest_paths(g, s, 
                            vertex_index_map(index).
                            weight_map(weight).
                            distance_map(distance));

    dijkstra_shortest_paths_no_color_map(g, s, 
                                         vertex_index_map(index).
                                         weight_map(weight).
                                         distance_map(distance));
  }
  {
    typedef incidence_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag, IncidenceGraph> Graph;
    vertex_t s;
    typedef graph_traits<Graph>::edge_descriptor edge_t;
    readable_property_map_archetype<edge_t, std::size_t> weight;
    typedef property_graph_archetype<Graph, vertex_index_t, std::size_t> 
      graph_t;
    graph_t& g = static_object<graph_t>::get();
    read_write_property_map_archetype<vertex_t, vertex_t> pred;
    dijkstra_shortest_paths(g, s,
                            predecessor_map(pred).
                            weight_map(weight));

    dijkstra_shortest_paths_no_color_map(g, s,
                                         predecessor_map(pred).
                                         weight_map(weight));
  }
  {
    typedef incidence_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag, IncidenceGraph> Graph;
    vertex_t s;
    typedef property_graph_archetype<Graph, edge_weight_t, std::size_t> 
      graph_t;
    graph_t& g = static_object<graph_t>::get();
    read_write_property_map_archetype<vertex_t, vertex_t> pred;
    readable_property_map_archetype<vertex_t, int> index;
    dijkstra_shortest_paths(g, s,
                            predecessor_map(pred).
                            vertex_index_map(index));

     dijkstra_shortest_paths_no_color_map(g, s,
                                          predecessor_map(pred).
                                          vertex_index_map(index));
  }
  {
    typedef incidence_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag> IncidenceGraph;
    typedef vertex_list_graph_archetype<vertex_t, directed_tag, 
      allow_parallel_edge_tag, IncidenceGraph> graph_t;
    graph_t& g = static_object<graph_t>::get();
    vertex_t s;
    typedef graph_traits<graph_t>::edge_descriptor edge_t;
    readable_property_map_archetype<edge_t, dist_value> weight;
    readable_property_map_archetype<vertex_t, int> index;
    read_write_property_map_archetype<vertex_t, color_value_archetype> color;
    read_write_property_map_archetype<vertex_t, dist_value> distance;
    typedef binary_function_archetype<dist_value, dist_value, dist_value> 
      Combine;
    Combine combine = static_object<Combine>::get();
    typedef binary_predicate_archetype<dist_value, dist_value>
      Compare;
    Compare compare = static_object<Compare>::get();
    dijkstra_visitor<> vis;

    dijkstra_shortest_paths(g, s, color_map(color).
                            vertex_index_map(index).
                            weight_map(weight).
                            distance_map(distance).
                            distance_combine(combine).
                            distance_compare(compare).
                            visitor(vis));

    dijkstra_shortest_paths_no_color_map(g, s,
                                         vertex_index_map(index).
                                         weight_map(weight).
                                         distance_map(distance).
                                         distance_combine(combine).
                                         distance_compare(compare).
                                         visitor(vis));
  }
  return 0;
}
