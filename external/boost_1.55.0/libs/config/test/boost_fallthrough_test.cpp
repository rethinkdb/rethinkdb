//  Use, modification and distribution are subject to the  
//  Boost Software License, Version 1.0. (See accompanying file  
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt) 

#include <boost/config.hpp> 

int test(int n) 
{ 
   switch (n) 
   { 
   case 0: 
      n++; 
      BOOST_FALLTHROUGH; 
   case 1: 
      n++; 
      break; 
   } 
   return n; 
}

