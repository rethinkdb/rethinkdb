/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/
//extract_devices.hpp
#ifndef BOOST_POLYGON_TUTORIAL_EXTRACT_DEVICES_HPP
#define BOOST_POLYGON_TUTORIAL_EXTRACT_DEVICES_HPP
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "connectivity_database.hpp"
#include "device.hpp"

typedef boost::polygon::connectivity_extraction_90<int> connectivity_extraction;
inline std::vector<std::set<int> >
extract_layer(connectivity_extraction& ce, std::vector<std::string>& net_ids,
              connectivity_database& connectivity, polygon_set& layout,
              std::string layer) {
  for(connectivity_database::iterator itr = connectivity.begin(); itr != connectivity.end(); ++itr) {
    net_ids.push_back((*itr).first);
    ce.insert((*itr).second[layer]);
  }
  std::vector<polygon> polygons;
  layout.get_polygons(polygons);
  for(std::size_t i = 0; i < polygons.size(); ++i) {
    ce.insert(polygons[i]);
  }
  std::vector<std::set<int> > graph(polygons.size() + net_ids.size(), std::set<int>());
  ce.extract(graph);
  return graph;
}

inline void extract_device_type(std::vector<device>& devices, connectivity_database& connectivity,
                                polygon_set& layout, std::string type) {
  //recall that P and NDIFF were merged into one DIFF layer in the connectivity database
  //find the two nets on the DIFF layer that interact with each transistor
  //and then find the net on the poly layer that interacts with each transistor
  boost::polygon::connectivity_extraction_90<int> cediff;
  std::vector<std::string> net_ids_diff;
  std::vector<std::set<int> > graph_diff =
    extract_layer(cediff, net_ids_diff, connectivity, layout, "DIFF");
  boost::polygon::connectivity_extraction_90<int> cepoly;
  std::vector<std::string> net_ids_poly;
  std::vector<std::set<int> > graph_poly =
    extract_layer(cepoly, net_ids_poly, connectivity, layout, "POLY");
  std::vector<device> tmp_devices(graph_diff.size() - net_ids_poly.size());
  for(std::size_t i = net_ids_poly.size(); i < graph_diff.size(); ++i) {
    tmp_devices[i - net_ids_diff.size()].type = type;
    tmp_devices[i - net_ids_diff.size()].terminals = std::vector<std::string>(3, std::string());
    std::size_t j = 0;
    for(std::set<int>::iterator itr = graph_diff[i].begin();
        itr != graph_diff[i].end(); ++itr, ++j) {
      if(j == 0) {
        tmp_devices[i - net_ids_diff.size()].terminals[0] = net_ids_diff[*itr];
      } else if(j == 1) {
        tmp_devices[i - net_ids_diff.size()].terminals[2] = net_ids_diff[*itr];
      } else {
        //error, too many diff connections
        tmp_devices[i - net_ids_diff.size()].terminals = std::vector<std::string>(3, std::string());
      }
    }
    j = 0;
    for(std::set<int>::iterator itr = graph_poly[i].begin();
        itr != graph_poly[i].end(); ++itr, ++j) {
      if(j == 0) {
        tmp_devices[i - net_ids_diff.size()].terminals[1] = net_ids_poly[*itr];
      } else {
        //error, too many poly connections
        tmp_devices[i - net_ids_poly.size()].terminals = std::vector<std::string>(3, std::string());
      }
    }
  }

  devices.insert(devices.end(), tmp_devices.begin(), tmp_devices.end());
}

//populates vector of devices based on connectivity and layout data
inline void extract_devices(std::vector<device>& devices, connectivity_database& connectivity,
                            layout_database& layout) {
  using namespace boost::polygon::operators;
  //p-type transistors are gate that interact with p diffusion and nwell
  polygon_set ptransistors = layout["GATE"];
  ptransistors.interact(layout["PDIFF"]);
  ptransistors.interact(layout["NWELL"]);
  //n-type transistors are gate that interact with n diffusion and not nwell
  polygon_set ntransistors = layout["GATE"];
  ntransistors.interact(layout["NDIFF"]);
  polygon_set not_ntransistors = ntransistors;
  not_ntransistors.interact(layout["NWELL"]);
  ntransistors -= not_ntransistors;
  extract_device_type(devices, connectivity, ptransistors, "PTRANS");
  extract_device_type(devices, connectivity, ntransistors, "NTRANS");
}

#endif
