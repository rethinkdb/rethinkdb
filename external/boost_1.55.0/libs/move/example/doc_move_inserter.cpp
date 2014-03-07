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

//[move_inserter_example
#include <boost/container/list.hpp>
#include "movable.hpp"
#include <cassert>
#include <algorithm>

using namespace ::boost::container;

typedef list<movable> list_t;
typedef list_t::iterator l_iterator;

template<class MoveInsertIterator>
void test_move_inserter(list_t &l2, MoveInsertIterator mit)
{
   //Create a list with 10 default constructed objects
   list<movable> l(10);
   assert(!l.begin()->moved());
   l2.clear();

   //Move insert into l2 containers
   std::copy(l.begin(), l.end(), mit);

   //Check size and status
   assert(l2.size() == l.size());
   assert(l.begin()->moved());
   assert(!l2.begin()->moved());
}

int main()
{
   list_t l2;
   test_move_inserter(l2, boost::back_move_inserter(l2));
   test_move_inserter(l2, boost::front_move_inserter(l2));
   test_move_inserter(l2, boost::move_inserter(l2, l2.end()));
   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>
