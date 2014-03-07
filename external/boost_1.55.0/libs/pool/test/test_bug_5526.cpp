/* Copyright (C) 2011 John Maddock
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

// Test of bug #2656 (https://svn.boost.org/trac/boost/ticket/2526)

#include <boost/pool/pool.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/assert.hpp>

struct bad
{
   bad()
   {
      buf = static_cast<int*>(boost::singleton_pool<int, sizeof(int)>::malloc());
      *buf = 0x1234;
   }
   ~bad()
   {
      BOOST_ASSERT(*buf == 0x1234);
      boost::singleton_pool<int, sizeof(int)>::free(buf);
   }
   int* buf;
};

std::auto_ptr<bad> aptr;

int main() 
{
   aptr.reset(new bad());
   return 0;
}
