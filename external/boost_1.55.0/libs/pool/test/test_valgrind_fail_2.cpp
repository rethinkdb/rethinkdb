/* Copyright (C) 2011 John Maddock
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

// Test of bug #2656 (https://svn.boost.org/trac/boost/ticket/2656)

#include <boost/pool/pool.hpp>

int main() 
{
   boost::pool<> p(sizeof(int));
   int* ptr = static_cast<int*>((p.malloc)());
   *ptr = 0;
   (p.free)(ptr);
   *ptr = 2; // write to freed memory
   return 0;
}
