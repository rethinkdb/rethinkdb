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

//[move_algorithms_example
#include "movable.hpp"
#include <boost/move/algorithm.hpp>
#include <cassert>
#include <boost/aligned_storage.hpp>

int main()
{
   const std::size_t ArraySize = 10;
   movable movable_array[ArraySize];
   movable movable_array2[ArraySize];
   //move
   boost::move(&movable_array2[0], &movable_array2[ArraySize], &movable_array[0]);
   assert(movable_array2[0].moved());
   assert(!movable_array[0].moved());

   //move backward
   boost::move_backward(&movable_array[0], &movable_array[ArraySize], &movable_array2[ArraySize]);
   assert(movable_array[0].moved());
   assert(!movable_array2[0].moved());

   //uninitialized_move
   boost::aligned_storage< sizeof(movable)*ArraySize
                         , boost::alignment_of<movable>::value>::type storage;
   movable *raw_movable = static_cast<movable*>(static_cast<void*>(&storage));
   boost::uninitialized_move(&movable_array2[0], &movable_array2[ArraySize], raw_movable);
   assert(movable_array2[0].moved());
   assert(!raw_movable[0].moved());
   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>
