//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.


#include "abi_test.hpp"


char abi_test::virtual_one()const
{ 
   return c; 
}

boost::int32_t abi_test::virtual_two()const
{ 
   return i; 
}

abi_test::abi_test()
{
   c = 0x12;
   i = 0x5678;
}

