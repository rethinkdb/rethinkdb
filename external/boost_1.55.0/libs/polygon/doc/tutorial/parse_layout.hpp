/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/

//parse_layout.hpp
#ifndef BOOST_POLYGON_TUTORIAL_PARSE_LAYOUT_HPP
#define BOOST_POLYGON_TUTORIAL_PARSE_LAYOUT_HPP
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "layout_rectangle.hpp"
#include "layout_pin.hpp"

//populates vectors of layout rectangles and pins
inline void parse_layout(std::vector<layout_rectangle>& rects, std::vector<layout_pin>& pins, 
                  std::ifstream& sin) {
  while(!sin.eof()) {
    std::string type_id;
    sin >> type_id;
    if(type_id == "Rectangle") {
      layout_rectangle rect;
      sin >> rect;
      rects.push_back(rect);
    } else if (type_id == "Pin") {
      layout_pin pin;
      sin >> pin;
      pins.push_back(pin);
    } else if (type_id == "") {
      break;
    }
  }
}

#endif
