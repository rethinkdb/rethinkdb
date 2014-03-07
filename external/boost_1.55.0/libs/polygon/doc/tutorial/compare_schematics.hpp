/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/
//compare_schematics.hpp
#ifndef BOOST_POLYGON_TUTORIAL_COMPARE_SCHEMATICS_HPP
#define BOOST_POLYGON_TUTORIAL_COMPARE_SCHEMATICS_HPP
#include <string>
#include "schematic_database.hpp"

bool compare_connectivity(std::string& ref_net, std::string& net,
                          schematic_database& reference_schematic,
                          schematic_database& schematic,
                          std::vector<std::size_t>& reference_to_internal_device_map,
                          std::size_t node_id) {
  std::set<std::size_t>& ref_nodes = reference_schematic.nets[ref_net];
  std::set<std::size_t>& nodes = schematic.nets[net];
  for(std::set<std::size_t>::iterator itr = ref_nodes.begin();
      itr != ref_nodes.end() && *itr < node_id; ++itr) {
    if(nodes.find(reference_to_internal_device_map[*itr]) == nodes.end())
      return false;
  }
  return true;
}

bool compare_schematics_recursive
(schematic_database& reference_schematic,
 schematic_database& schematic,
 std::vector<std::size_t>& reference_to_internal_device_map,
 std::set<std::size_t>& assigned_devices, std::size_t node_id){
  //do check of equivalence up to this node
  for(std::size_t i = 0; i < node_id; ++i) {
    for(std::size_t j = 0; j < reference_schematic.devices[i].terminals.size(); ++j) {
      device& rd = reference_schematic.devices[i];
      device& xd = schematic.devices[reference_to_internal_device_map[i]];
      if(rd.type == "PIN") {
        if(rd.terminals[j] != xd.terminals[j])
          return false;
      } else {
        //connectivity must be the same
        if(j == 1) {
          //gate has to be the same net
          if(!compare_connectivity(rd.terminals[1], xd.terminals[1], reference_schematic, schematic,
                                   reference_to_internal_device_map, node_id))
            return false;
        } else {
          //order of nets in source and drain is not important so check both ways and accept either
          if(!compare_connectivity(rd.terminals[j], xd.terminals[0], reference_schematic, schematic,
                                   reference_to_internal_device_map, node_id) &&
             !compare_connectivity(rd.terminals[j], xd.terminals[2], reference_schematic, schematic,
                                   reference_to_internal_device_map, node_id))
            return false;
        }
      }
    }
  }
  if(node_id >= reference_schematic.devices.size())
    return true; //final success
  
  //recurse into subsequent nodes
  for(std::size_t i = 0; i < schematic.devices.size(); ++i) {
    if(reference_schematic.devices[node_id].type !=
       schematic.devices[i].type)
      continue; //skip dissimilar devices
    //avoid multi-assignment of devices
    if(assigned_devices.find(i) == assigned_devices.end()) {
      reference_to_internal_device_map[node_id] = i;
      std::set<std::size_t>::iterator itr = assigned_devices.insert(assigned_devices.end(), i);
      if(compare_schematics_recursive(reference_schematic, schematic,
                                      reference_to_internal_device_map,
                                      assigned_devices, node_id + 1))
        return true;
      assigned_devices.erase(itr);
    }
  }
  //could not find match between schematics
  return false;
}

//this is a trivial brute force comparison algorithm because comparing
//schematics does not require the use of Boost.Polygon and doing it more
//optimally does not add to the tutorial
inline bool compare_schematics(schematic_database& reference_schematic,
                               schematic_database& schematic) {
  std::vector<std::size_t> 
    reference_to_internal_device_map(reference_schematic.devices.size(), 0);
  std::set<std::size_t> assigned_devices;
  return compare_schematics_recursive(reference_schematic, schematic, 
                                      reference_to_internal_device_map,
                                      assigned_devices, 0);
}

#endif
