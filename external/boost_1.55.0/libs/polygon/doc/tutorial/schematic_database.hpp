/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/

//schematic_database.hpp
#ifndef BOOST_POLYGON_TUTORIAL_SCHEMATIC_DATABASE_HPP
#define BOOST_POLYGON_TUTORIAL_SCHEMATIC_DATABASE_HPP
#include <string>
#include <fstream>
#include <map>
#include <set>
#include "device.hpp"

struct schematic_database{
  std::vector<device> devices;
  std::map<std::string, std::set<std::size_t> > nets;
};

//given a vector of devices populate the map of net name to set of device index
inline void extract_netlist(std::map<std::string, std::set<std::size_t> >& nets,
                            std::vector<device>& devices) {
  for(std::size_t i = 0; i < devices.size(); ++i) {
    for(std::size_t j = 0; j < devices[i].terminals.size(); ++j) {
      //create association between net name and device id
      nets[devices[i].terminals[j]].insert(nets[devices[i].terminals[j]].end(), i);
    }
  }
}

inline void parse_schematic_database(schematic_database& schematic,
                                     std::ifstream& sin) {
  std::vector<device>& devices = schematic.devices;
  while(!sin.eof()) {
    std::string type_id;
    sin >> type_id;
    if(type_id == "Device") {
      device d;
      sin >> d;
      devices.push_back(d);
    } else if (type_id == "Pin") {
      std::string net;
      sin >> net;
      device d;
      d.type = "PIN";
      d.terminals.push_back(net);
      devices.push_back(d);
    } else if (type_id == "") {
      break;
    }
  }
  extract_netlist(schematic.nets, devices);
}

#endif
