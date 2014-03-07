//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <iostream>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <cstddef>
#include <exception>

using namespace boost::interprocess;

int main ()
{
   try{
      const std::size_t MemSize = 99999*2;
      {
         //Now check anonymous mapping
         mapped_region region(anonymous_shared_memory(MemSize));

         //Write pattern
         unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < MemSize
            ;++i, ++pattern){
            *pattern = static_cast<unsigned char>(i);
         }

         //Check pattern
         pattern = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < MemSize
            ;++i, ++pattern){
            if(*pattern != static_cast<unsigned char>(i)){
               return 1;
            }
         }
      }
   }
   catch(std::exception &exc){
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
      return 1;
   }
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
