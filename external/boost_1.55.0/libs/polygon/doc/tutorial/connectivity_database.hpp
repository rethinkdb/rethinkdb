/*
  Copyright 2010 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
//connectivity_database.hpp
#ifndef BOOST_POLYGON_TUTORIAL_CONNECTIVITY_DATABASE_HPP
#define BOOST_POLYGON_TUTORIAL_CONNECTIVITY_DATABASE_HPP
#include <boost/polygon/polygon.hpp>
#include <map>
#include <sstream>
#include "layout_database.hpp"
#include "layout_pin.hpp"

typedef std::map<std::string, layout_database > connectivity_database;

//map layout pin data type to boost::polygon::rectangle_concept
namespace boost { namespace polygon{
  template <>
  struct rectangle_traits<layout_pin> {
    typedef int coordinate_type;
    typedef interval_data<int> interval_type;
    static inline interval_type get(const layout_pin& pin, orientation_2d orient) {
      if(orient == HORIZONTAL)
        return interval_type(pin.xl, pin.xh);
      return interval_type(pin.yl, pin.yh);
    }
  };

  template <>
  struct geometry_concept<layout_pin> { typedef rectangle_concept type; };
}}

typedef boost::polygon::polygon_90_data<int> polygon;
typedef boost::polygon::polygon_90_set_data<int> polygon_set;

inline void populate_connected_component
(connectivity_database& connectivity, std::vector<polygon>& polygons, 
 std::vector<int> polygon_color, std::vector<std::set<int> >& graph, 
 std::size_t node_id, std::size_t polygon_id_offset, std::string& net, 
 std::vector<std::string>& net_ids, std::string net_prefix,
 std::string& layout_layer) {
  if(polygon_color[node_id] == 1)
    return;
  polygon_color[node_id] = 1;
  if(node_id < polygon_id_offset && net_ids[node_id] != net) {
    //merge nets in connectivity database
    //if one of the nets is internal net merge it into the other
    std::string net1 = net_ids[node_id];
    std::string net2 = net;
    if(net.compare(0, net_prefix.length(), net_prefix) == 0) {
      net = net1;
      std::swap(net1, net2);
    } else {
      net_ids[node_id] = net;
    }
    connectivity_database::iterator itr = connectivity.find(net1);
    if(itr != connectivity.end()) {
      for(layout_database::iterator itr2 = (*itr).second.begin();
          itr2 != (*itr).second.end(); ++itr2) {
        connectivity[net2][(*itr2).first].insert((*itr2).second);
      }
      connectivity.erase(itr);
    }
  }
  if(node_id >= polygon_id_offset)
    connectivity[net][layout_layer].insert(polygons[node_id - polygon_id_offset]);
  for(std::set<int>::iterator itr = graph[node_id].begin();
      itr != graph[node_id].end(); ++itr) {
    populate_connected_component(connectivity, polygons, polygon_color, graph, 
                                 *itr, polygon_id_offset, net, net_ids, net_prefix, layout_layer);
  }
}

inline void connect_layout_to_layer(connectivity_database& connectivity, polygon_set& layout, std::string layout_layer, std::string layer, std::string net_prefix, int& net_suffix) {
  if(layout_layer.empty())
    return;
  boost::polygon::connectivity_extraction_90<int> ce;
  std::vector<std::string> net_ids;
  for(connectivity_database::iterator itr = connectivity.begin(); itr != connectivity.end(); ++itr) {
    net_ids.push_back((*itr).first);
    ce.insert((*itr).second[layer]);
  }
  std::vector<polygon> polygons;
  layout.get_polygons(polygons);
  std::size_t polygon_id_offset = net_ids.size();
  for(std::size_t i = 0; i < polygons.size(); ++i) {
    ce.insert(polygons[i]);
  }
  std::vector<std::set<int> > graph(polygons.size() + net_ids.size(), std::set<int>());
  ce.extract(graph);
  std::vector<int> polygon_color(polygons.size() + net_ids.size(), 0);
  //for each net in net_ids populate connected component with net
  for(std::size_t node_id = 0; node_id < net_ids.size(); ++node_id) {
    populate_connected_component(connectivity, polygons, polygon_color, graph, node_id, 
                                 polygon_id_offset, net_ids[node_id], net_ids, 
                                 net_prefix, layout_layer);
  }
  //for each polygon_color that is zero populate connected compontent with net_prefix + net_suffix++
  for(std::size_t i = 0; i < polygons.size(); ++i) {
    if(polygon_color[i + polygon_id_offset] == 0) {
      std::stringstream ss(std::stringstream::in | std::stringstream::out);
      ss << net_prefix << net_suffix++;
      std::string internal_net; 
      ss >> internal_net;
      populate_connected_component(connectivity, polygons, polygon_color, graph, 
                                   i + polygon_id_offset, 
                                   polygon_id_offset, internal_net, net_ids, 
                                   net_prefix, layout_layer);
    }
  }
}

//given a layout_database we populate a connectivity database
inline void populate_connectivity_database(connectivity_database& connectivity, std::vector<layout_pin>& pins, layout_database& layout) {
  using namespace boost::polygon;
  using namespace boost::polygon::operators;
  for(std::size_t i = 0; i < pins.size(); ++i) {
    connectivity[pins[i].net][pins[i].layer].insert(pins[i]);
  }
  int internal_net_suffix = 0;
  //connect metal1 layout to pins which were on metal1
  connect_layout_to_layer(connectivity, layout["METAL1"], "METAL1", 
                          "METAL1", "__internal_net_", internal_net_suffix);
  //connect via0 layout to metal1
  connect_layout_to_layer(connectivity, layout["VIA0"], "VIA0", 
                          "METAL1", "__internal_net_", internal_net_suffix);
  //poly needs to have gates subtracted from it to prevent shorting through transistors
  polygon_set poly_not_gate = layout["POLY"] - layout["GATE"];
  //connect poly minus gate to via0
  connect_layout_to_layer(connectivity, poly_not_gate, "POLY", 
                          "VIA0", "__internal_net_", internal_net_suffix);
  //we don't want to short signals through transistors so we subtract the gate regions
  //from the diffusions
  polygon_set diff_not_gate = (layout["PDIFF"] + layout["NDIFF"]) - layout["GATE"];
  //connect diffusion minus gate to poly
  //Note that I made up the DIFF layer name for combined P and NDIFF
  connect_layout_to_layer(connectivity, diff_not_gate, "DIFF", 
                          "POLY", "__internal_net_", internal_net_suffix);
  //connect gate to poly to make connections through gates on poly
  connect_layout_to_layer(connectivity, layout["GATE"], "GATE", 
                          "POLY", "__internal_net_", internal_net_suffix);
  //now we have traced connectivity of the layout down to the transistor level
  //any polygons not connected to pins have been assigned internal net names
}

#endif
