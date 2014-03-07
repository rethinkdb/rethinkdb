/* Copyright (C) 2011 Kwan Ting Chan
 * Based from bug report submitted by Xiaohan Wang
 * 
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

// Test of bug #4960 (https://svn.boost.org/trac/boost/ticket/4960)

#include <boost/pool/pool_alloc.hpp>
#include <vector>
#include <iostream>

typedef std::vector<int, boost::pool_allocator<int> > EventVector;
typedef std::vector<EventVector, boost::pool_allocator<EventVector> > IndexVector;

int main() 
{
  IndexVector iv;
  int limit = 100;
  for (int i = 0; i < limit; ++i) 
  {
    iv.push_back(EventVector());
    for(int j = 0; j < limit; ++j)
      iv.back().push_back(j);
  }

   boost::pool<boost::default_user_allocator_new_delete> po(4);
   for(int i = 0; i < limit; ++i)
   {
      void* p = po.ordered_malloc(0);
      po.ordered_free(p, 0);
   }

  boost::pool_allocator<int> al;
  for(int i = 0; i < limit; ++i)
  {
     int* p = al.allocate(0);
     al.deallocate(p, 0);
  }

  std::cout << "it works\n";
  return 0;
}
