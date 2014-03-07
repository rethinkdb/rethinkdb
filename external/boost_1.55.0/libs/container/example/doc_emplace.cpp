//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
//[doc_emplace
#include <boost/container/list.hpp>
#include <cassert>

//Non-copyable and non-movable class
class non_copy_movable
{
   non_copy_movable(const non_copy_movable &);
   non_copy_movable& operator=(const non_copy_movable &);

   public:
   non_copy_movable(int = 0) {}
};

int main ()
{
   using namespace boost::container;

   //Store non-copyable and non-movable objects in a list
   list<non_copy_movable> l;
   non_copy_movable ncm;

   //A new element will be built calling non_copy_movable(int) contructor
   l.emplace(l.begin(), 0);
   assert(l.size() == 1);

   //A new element will be built calling the default constructor
   l.emplace(l.begin());
   assert(l.size() == 2);
   return 0;
}
//]
#include <boost/container/detail/config_end.hpp>
