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
#include <fstream>
#include <iostream>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

shared_memory_object get_shared_memory_mapping()
{
   shared_memory_object sh;
   return shared_memory_object(boost::move(sh));
}

int main ()
{
   std::string process_id = test::get_process_id_name();
   std::string process_id2(process_id);
   process_id2 += "_2";
   try{
      const std::size_t FileSize = 99999*4;
      {
         //Remove shared memory
         shared_memory_object::remove(process_id.c_str());
         shared_memory_object::remove(process_id2.c_str());

         //Create shared memory and file mapping
         shared_memory_object mapping(create_only, process_id.c_str(), read_write);
         mapping.truncate(FileSize);
      }

      {
         //Create a file mapping
         shared_memory_object mapping(open_only, process_id.c_str(), read_write);

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
         //Create a file mapping
         shared_memory_object mapping(open_only, process_id.c_str(), read_write);
         mapped_region region(mapping, read_write, 0, FileSize/2, 0);
         mapped_region region2(mapping, read_write, FileSize/2, FileSize - FileSize/2, 0);

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
         //Create a file mapping
         shared_memory_object mapping(open_only, process_id.c_str(), read_only);

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
         //Now shrink
         const std::size_t original_region_size = region.get_size();
         if(!region.shrink_by(region.get_size()/2, false) || region.get_size() != original_region_size/2){
            return 1;
         }
         const std::size_t shrunk_region_size = region.get_size();
         if(!region.shrink_by(region.get_size()/2, true) || region.get_size() != shrunk_region_size/2){
            return 1;
         }

         //Now advise
         #if defined(POSIX_MADV_NORMAL) || defined(MADV_NORMAL)
         std::cout << "Advice normal" << std::endl;
         if(!region.advise(mapped_region::advice_normal)){
            return 1;
         }
         #endif

         #if defined(POSIX_MADV_SEQUENTIAL) || defined(MADV_SEQUENTIAL)
         std::cout << "Advice sequential" << std::endl;
         if(!region.advise(mapped_region::advice_sequential)){
            return 1;
         }
         #endif

         #if defined(POSIX_MADV_RANDOM) || defined(MADV_RANDOM)
         std::cout << "Advice random" << std::endl;
         if(!region.advise(mapped_region::advice_random)){
            return 1;
         }
         #endif

         #if defined(POSIX_MADV_WILLNEED) || defined(MADV_WILLNEED)
         std::cout << "Advice 'will need'" << std::endl;
         if(!region.advise(mapped_region::advice_willneed)){
            return 1;
         }
         #endif

         #if defined(POSIX_MADV_DONTNEED) || (defined(MADV_DONTNEED) && defined(BOOST_INTERPROCESS_MADV_DONTNEED_HAS_NONDESTRUCTIVE_SEMANTICS))
         std::cout << "Advice 'dont't need'" << std::endl;
         if(!region.advise(mapped_region::advice_dontneed)){
            return 1;
         }
         #endif

      }
      {
         //Check for busy address space
         shared_memory_object mapping(open_only, process_id.c_str(), read_only);
         mapped_region region (mapping, read_only);
         shared_memory_object mapping2(create_only, process_id2.c_str(), read_write);
         mapping2.truncate(FileSize);
         try{
            mapped_region region2 (mapping2, read_only, 0, FileSize, region.get_address());
         }
         catch(interprocess_exception &e){
            shared_memory_object::remove(process_id2.c_str());
            if(e.get_error_code() != busy_error){
               throw e;
            }
         }
         catch(std::exception &){
            shared_memory_object::remove(process_id2.c_str());
            throw;
         }
         shared_memory_object::remove(process_id2.c_str());
      }
      {
         //Now check anonymous mapping
         mapped_region region(anonymous_shared_memory(FileSize));

         //Write pattern
         unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
         for(std::size_t i = 0
            ;i < FileSize
            ;++i, ++pattern){
            *pattern = static_cast<unsigned char>(i);
         }

         //Check pattern
         pattern = static_cast<unsigned char*>(region.get_address());
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
         shared_memory_object mapping(open_only, process_id.c_str(), read_write);
         shared_memory_object move_ctor(boost::move(mapping));
         shared_memory_object move_assign;
         move_assign = boost::move(move_ctor);
         shared_memory_object ret(get_shared_memory_mapping());
      }
   }
   catch(std::exception &exc){
      shared_memory_object::remove(process_id.c_str());
      shared_memory_object::remove(process_id2.c_str());
      std::cout << "Unhandled exception: " << exc.what() << std::endl;
      return 1;
   }
   shared_memory_object::remove(process_id.c_str());
   return 0;
}
