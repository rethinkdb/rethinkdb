// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2011, Geodan, Amsterdam, the Netherlands
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

Qt World Mapper example

It will show a basic Qt Widget, displaying world countries

To compile this program:

Install Qt (if not done before)
   
Using Linux/gcc
   - install Qt with sudo apt-get install libqt4-dev
   - run qmake -project
   - edit the generated file "with_external_libs.pro"  and delete all lines but the x06_qt_world_mapper.cpp
   - run qmake
   - edit the generated Makefile, if necessary, and add -I../../../.. to include Boost and Boost.Geometry
   - run make

