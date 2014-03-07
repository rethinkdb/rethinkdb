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
#include <ios> //std::streamoff
#include <fstream>   //std::ofstream, std::ifstream
#include <iostream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <memory>    //std::auto_ptr
#include <stdexcept> //std::exception
#include <cstddef>   //std::size_t
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

inline std::string get_filename()
{
   std::string ret (ipcdetail::get_temporary_path());
   ret += "/";
   ret += test::get_process_id_name();
   return ret;
}

file_mapping get_file_mapping()
{
   file_mapping f;
   return file_mapping(boost::move(f));
}

int main ()
{
   try{
      const std::size_t FileSize = 99999*2;
      {
         //Create file with given size
         std::ofstream file(get_filename().c_str(), std::ios::binary | std::ios::trunc);
         file.seekp(static_cast<std::streamoff>(FileSize-1));
         file.write("", 1);
      }

      {
         //Create a file mapping
         file_mapping mapping(get_filename().c_str(), read_write);
         //Create two mapped regions, one half of the file each
         mapped_region region (mapping
                              ,read_write
                              ,0
                              ,FileSize/2
                              );

         mapped_region region2(mapping
                              ,read_write
                              ,FileSize/2
                              ,FileSize - FileSize/2
                              );

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

      //See if the pattern is correct in the file
      {
         //Open the file
         std::ifstream file(get_filename().c_str(), std::ios::binary);

         //Create a memory buffer
         std::auto_ptr<unsigned char> memory (new unsigned char [FileSize/2 +1]);

         //Fill buffer
         file.read(static_cast<char*>(static_cast<void*>(memory.get()))
                  , FileSize/2);

         unsigned char *checker = memory.get();
         //Check pattern
         for(std::size_t i = 0
            ;i < FileSize/2
            ;++i){
            if(*checker++ != static_cast<unsigned char>(i)){
               return 1;
            }
         }

         //Fill buffer
         file.read(static_cast<char*>(static_cast<void*>(memory.get()))
                  , FileSize - FileSize/2);

         checker = memory.get();
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
         //Create a file mapping
         file_mapping mapping(get_filename().c_str(), read_only);

         //Create a single regions, mapping all the file
         mapped_region region (mapping
                              ,read_only
                              );

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
      {
         //Now test move semantics
         file_mapping mapping(get_filename().c_str(), read_only);
         file_mapping move_ctor(boost::move(mapping));
         file_mapping move_assign;
         move_assign = boost::move(move_ctor);
         mapping.swap(move_assign);
         file_mapping ret(get_file_mapping());
      }
   }
   catch(std::exception &exc){
      file_mapping::remove(get_filename().c_str());
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
      throw;
   }
   file_mapping::remove(get_filename().c_str());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
