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
#ifndef BOOST_MOVE_TEST_COPYMOVABLE_HPP
#define BOOST_MOVE_TEST_COPYMOVABLE_HPP

#include <boost/move/detail/config_begin.hpp>

//[movable_definition 
//header file "copy_movable.hpp"
#include <boost/move/core.hpp>

//A copy_movable class
class copy_movable
{
   BOOST_COPYABLE_AND_MOVABLE(copy_movable)
   int value_;

   public:
   copy_movable() : value_(1){}

   //Move constructor and assignment
   copy_movable(BOOST_RV_REF(copy_movable) m)
   {  value_ = m.value_;   m.value_ = 0;  }

   copy_movable(const copy_movable &m)
   {  value_ = m.value_;   }

   copy_movable & operator=(BOOST_RV_REF(copy_movable) m)
   {  value_ = m.value_;   m.value_ = 0;  return *this;  }

   copy_movable & operator=(BOOST_COPY_ASSIGN_REF(copy_movable) m)
   {  value_ = m.value_;   return *this;  }

   bool moved() const //Observer
   {  return value_ == 0; }
};

//]

#include <boost/move/detail/config_end.hpp>

#endif //BOOST_MOVE_TEST_COPYMOVABLE_HPP
