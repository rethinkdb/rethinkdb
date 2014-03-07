/*
  Copyright 2010 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/

//device.hpp
#ifndef BOOST_POLYGON_TUTORIAL_DEVICE_HPP
#define BOOST_POLYGON_TUTORIAL_DEVICE_HPP
#include <string>
#include <vector>
#include <iostream>

struct device {
  std::string type;
  std::vector<std::string> terminals;
};

inline std::ostream& operator << (std::ostream& o, const device& r)
{
  o << r.type << " ";
  for(std::size_t i = 0; i < r.terminals.size(); ++i) {
    o << r.terminals[i] << " ";
  }
  return o;
}

inline std::istream& operator >> (std::istream& i, device& r)
{
  i >> r.type; 
  r.terminals = std::vector<std::string>(3, std::string());
  i >> r.terminals[0] >> r.terminals[1] >> r.terminals[2];
  return i;
}

#endif
