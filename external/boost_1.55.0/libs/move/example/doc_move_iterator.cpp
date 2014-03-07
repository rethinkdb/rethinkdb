//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
//[move_iterator_example
#include <boost/container/vector.hpp>
#include "movable.hpp"
#include <cassert>

int main()
{
   using namespace ::boost::container;

   //Create a vector with 10 default constructed objects
   vector<movable> v(10);
   assert(!v[0].moved());

   //Move construct all elements in v into v2
   vector<movable> v2( boost::make_move_iterator(v.begin())
                     , boost::make_move_iterator(v.end()));
   assert(v[0].moved());
   assert(!v2[0].moved());

   //Now move assign all elements from in v2 back into v
   v.assign( boost::make_move_iterator(v2.begin())
           , boost::make_move_iterator(v2.end()));
   assert(v2[0].moved());
   assert(!v[0].moved());

   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>
