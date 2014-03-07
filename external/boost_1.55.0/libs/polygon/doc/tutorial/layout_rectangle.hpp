/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/

//layout_rectangle.hpp
#ifndef BOOST_POLYGON_TUTORIAL_LAYOUT_RECTANGLE_HPP
#define BOOST_POLYGON_TUTORIAL_LAYOUT_RECTANGLE_HPP
#include <string>
#include <iostream>

struct layout_rectangle {
  int xl, yl, xh, yh;
  std::string layer;
};

inline std::ostream& operator << (std::ostream& o, const layout_rectangle& r)
{
  o << r.xl << " " << r.xh << " " << r.yl << " " << r.yh << " " << r.layer;
  return o;
}

inline std::istream& operator >> (std::istream& i, layout_rectangle& r)
{
  i >> r.xl >> r.xh >> r.yl >> r.yh >> r.layer; 
  return i;
}

#endif
