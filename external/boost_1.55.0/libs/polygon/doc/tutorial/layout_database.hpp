/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/

//layout_database.hpp
#ifndef BOOST_POLYGON_TUTORIAL_LAYOUT_DATABASE_HPP
#define BOOST_POLYGON_TUTORIAL_LAYOUT_DATABASE_HPP
#include <boost/polygon/polygon.hpp>
#include <map>
#include "layout_rectangle.hpp"

typedef std::map<std::string, boost::polygon::polygon_90_set_data<int> > layout_database;

//map the layout rectangle data type to the boost::polygon::rectangle_concept
namespace boost { namespace polygon{
  template <>
  struct rectangle_traits<layout_rectangle> {
    typedef int coordinate_type;
    typedef interval_data<int> interval_type;
    static inline interval_type get(const layout_rectangle& rectangle, orientation_2d orient) {
      if(orient == HORIZONTAL)
        return interval_type(rectangle.xl, rectangle.xh);
      return interval_type(rectangle.yl, rectangle.yh);
    }
  };

  template <>
  struct geometry_concept<layout_rectangle> { typedef rectangle_concept type; };
}}

//insert layout rectangles into a layout database
inline void populate_layout_database(layout_database& layout, std::vector<layout_rectangle>& rects) {
  for(std::size_t i = 0; i < rects.size(); ++i) {
    layout[rects[i].layer].insert(rects[i]);
  }
}
#endif
