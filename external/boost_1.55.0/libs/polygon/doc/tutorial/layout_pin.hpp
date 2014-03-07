/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/

//layout_pin.hpp
#ifndef BOOST_POLYGON_TUTORIAL_LAYOUT_PIN_HPP
#define BOOST_POLYGON_TUTORIAL_LAYOUT_PIN_HPP
#include <string>
#include <iostream>

struct layout_pin {
  int xl, yl, xh, yh;
  std::string layer;
  std::string net;
};

inline std::ostream& operator << (std::ostream& o, const layout_pin& r)
{
  o << r.xl << " " << r.xh << " " << r.yl << " " << r.yh << " " << r.layer << " " << r.net;
  return o;
}

inline std::istream& operator >> (std::istream& i, layout_pin& r)
{
  i >> r.xl >> r.xh >> r.yl >> r.yh >> r.layer >> r.net; 
  return i;
}

#endif
