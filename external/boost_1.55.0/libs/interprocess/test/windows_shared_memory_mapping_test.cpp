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
#include <boost/interprocess/detail/workaround.hpp>

#ifdef BOOST_INTERPROCESS_WINDOWS

#include <fstream>
#include <iostream>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

int main ()
{
   try{
      const char *names[2] = { test::get_process_id_name(), 0 };
      for(unsigned int i_name = 0; i_name < sizeof(names)/sizeof(names[0]); ++i_name)
      {
         const std::size_t FileSize = 99999*2;
         //Create a file mapping
         windows_shared_memory mapping
            (create_only, names[i_name], read_write, FileSize);

         {

            //Create two mapped regions, one half of the file each
            mapped_region region (mapping
                                 ,read_write
                                 ,0
                                 ,FileSize/2
                                 ,0);

            mapped_region region2(mapping
                                 ,read_write
                                 ,FileSize/2
                                 ,FileSize - FileSize/2
                                 ,0);

            //Fill two regions with a pattern
            unsigned char *filler = static_cast<unsigned char*>(region.get_address());
            for(std::size_t i = 0
               ;i < FileSize/2
               ;++i){
               *filler++ = static_cast<unsigned char>(i);
            }

            filler = static_cast<unsigned char*>(region2.get_address());
            for(std::size_t i = FileSize/2
               ;i < FileSize
               ;++i){
               *filler++ = static_cast<unsigned char>(i);
            }
            if(!region.flush(0, 0, false)){
               return 1;
            }

            if(!region2.flush(0, 0, true)){
               return 1;
            }
         }

         //See if the pattern is correct in the file using two mapped regions
         {
            mapped_region region (mapping, read_only, 0, FileSize/2, 0);
            mapped_region region2(mapping, read_only, FileSize/2, FileSize - FileSize/2, 0);

            unsigned char *checker = static_cast<unsigned char*>(region.get_address());
            //Check pattern
            for(std::size_t i = 0
               ;i < FileSize/2
               ;++i){
               if(*checker++ != static_cast<unsigned char>(i)){
                  return 1;
               }
            }

            //Check second half
            checker = static_cast<unsigned char *>(region2.get_address());

            //Check pattern
            for(std::size_t i = FileSize/2
               ;i < FileSize
               ;++i){
               if(*checker++ != static_cast<unsigned char>(i)){
                  return 1;
               }
            }
         }

         //Now check the pattern mapping a single read only mapped_region
         {
            //Create a single regions, mapping all the file
            mapped_region region (mapping, read_only);

            //Check pattern
            unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
            for(std::size_t i = 0
               ;i < FileSize
               ;++i, ++pattern){
               if(*pattern != static_cast<unsigned char>(i)){
                  return 1;
               }
            }
         }
      }
   }
   catch(std::exception &exc){
      //shared_memory_object::remove(test::get_process_id_name());
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
      return 1;
   }

   return 0;
}

#else

int main()
{
   return 0;
}

#endif

#include <boost/interprocess/detail/config_end.hpp>
