/* Copyright (C) 2011 John Maddock
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

// Test of bug #2656 (https://svn.boost.org/trac/boost/ticket/2656)

#include <boost/pool/pool.hpp>
#include <iostream>
#include <iomanip>

static const int magic_value = 0x12345678;

int main() 
{
   boost::pool<> p(sizeof(int));
   int* ptr = static_cast<int*>((p.malloc)());
   std::cout << *ptr << std::endl; // uninitialized read
   return 0;
}
