// Boost.Geometry.Index varray
// Unit Test

// Copyright (c) 2009 Ion Gaztanaga
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_TEST_MOVABLE_HPP
#define BOOST_GEOMETRY_INDEX_TEST_MOVABLE_HPP

//[movable_definition 
//header file "movable.hpp"
#include <boost/move/move.hpp>

//A movable class
class movable
{
    BOOST_MOVABLE_BUT_NOT_COPYABLE(movable)
    int value_;
    
public:
    movable() : value_(1){}
    
    //Move constructor and assignment
    movable(BOOST_RV_REF(movable) m)
    {  value_ = m.value_;   m.value_ = 0;  }
    
    movable & operator=(BOOST_RV_REF(movable) m)
    {  value_ = m.value_;   m.value_ = 0;  return *this;  }
    
    bool moved() const //Observer
    {  return value_ == 0; }
};


class copy_movable
{
    BOOST_COPYABLE_AND_MOVABLE(copy_movable)
    size_t value_;
    
public:
    copy_movable(size_t value = 1) : value_(value){}
    
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
    
    bool operator==(const copy_movable& m) const
    {  return value_ == m.value_; }
};

struct copy_movable_wrapper
{
   copy_movable cm;
};

copy_movable produce()
{  return copy_movable();  }

namespace boost{

template<>
struct has_nothrow_move<movable>
{
   static const bool value = true;
};

template<>
struct has_nothrow_move<copy_movable>
{
   static const bool value = true;
};

}  //namespace boost{
//]

#endif //BOOST_GEOMETRY_INDEX_TEST_MOVABLE_HPP
