// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2010, Geodan, Amsterdam, the Netherlands
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



wxWidgets World Mapper example

It will show a basic wxWidgets window, displaying world countries, highlighting the country under
the mouse, and indicating position of the mouse in latitude/longitude and in pixels.


To compile this program:

Install wxWidgets (if not done before)

Using Linux/gcc
   - check if installation is OK, http://wiki.wxwidgets.org/Installing_and_configuring_under_Ubuntu
   - compile using e.g. gcc -o x04_wxwidgets -I../../../.. x04_wxwidgets_world_mapper.cpp `wx-config --cxxflags` `wx-config --libs`
   
