/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/
#include "schematic_database.hpp"
#include "layout_pin.hpp"
#include "layout_rectangle.hpp"
#include "connectivity_database.hpp"
#include "compare_schematics.hpp"
#include "extract_devices.hpp"
#include "parse_layout.hpp"
#include "layout_database.hpp"
#include "device.hpp"
#include <string>
#include <fstream>
#include <iostream>

bool compare_files(std::string layout_file, std::string schematic_file) {
  std::ifstream sin(schematic_file.c_str());
  std::ifstream lin(layout_file.c_str());

  std::vector<layout_rectangle> rects;
  std::vector<layout_pin> pins;
  parse_layout(rects, pins, lin);

  schematic_database reference_schematic;
  parse_schematic_database(reference_schematic, sin);

  layout_database layout;
  populate_layout_database(layout, rects);

  connectivity_database connectivity;
  populate_connectivity_database(connectivity, pins, layout);

  schematic_database schematic;
  std::vector<device>& devices = schematic.devices;
  for(std::size_t i = 0; i < pins.size(); ++i) {
    devices.push_back(device());
    devices.back().type = "PIN";
    devices.back().terminals.push_back(pins[i].net);
  }
  extract_devices(devices, connectivity, layout);
  extract_netlist(schematic.nets, devices);

  return compare_schematics(reference_schematic, schematic);
}

int main(int argc, char **argv) {
  if(argc < 3) {
    std::cout << "usage: " << argv[0] << " <layout_file> <schematic_file>" << std::endl;
    return -1;
  }
  bool result = compare_files(argv[1], argv[2]);
  if(result == false) {
    std::cout << "Layout does not match schematic." << std::endl;
    return 1;
  } 
  std::cout << "Layout does match schematic." << std::endl;
  return 0;
}
