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

#if defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)

#include <fstream>
#include <iostream>
#include <boost/interprocess/xsi_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/detail/file_wrapper.hpp>
#include <string>
#include <iostream>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

void remove_shared_memory(const xsi_key &key)
{
   try{
      xsi_shared_memory xsi(open_only, key);
      xsi_shared_memory::remove(xsi.get_shmid());
   }
   catch(interprocess_exception &e){
      if(e.get_error_code() != not_found_error)
         throw;
   }
}

class xsi_shared_memory_remover
{
   public:
   xsi_shared_memory_remover(xsi_shared_memory &xsi_shm)
      : xsi_shm_(xsi_shm)
   {}

   ~xsi_shared_memory_remover()
   {  xsi_shared_memory::remove(xsi_shm_.get_shmid());  }
   private:
   xsi_shared_memory & xsi_shm_;
};

inline std::string get_filename()
{
   std::string ret (ipcdetail::get_temporary_path());
   ret += "/";
   ret += test::get_process_id_name();
   return ret;
}

int main (int argc, char *argv[])
{
   std::string filename(get_filename());
   const char *names[2] = { filename.c_str(), 0 };

   file_mapping::remove(names[0]);
   {  ipcdetail::file_wrapper(create_only, names[0], read_write); }
   xsi_key key(names[0], 1);
   file_mapping::remove(names[0]);
   remove_shared_memory(key);

   unsigned int i;
   try{
      for(i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
      {
         const std::size_t FileSize = 99999*2;
         //Create a file mapping
         xsi_shared_memory mapping (create_only, names[i] ? key : xsi_key(), FileSize);
         xsi_shared_memory_remover rem(mapping);
         try{
            {
               //Partial mapping should fail fox XSI shared memory
               bool thrown = false;
               try{
                  mapped_region region2(mapping, read_write, FileSize/2, FileSize - FileSize/2, 0);
               }
               catch(...){
                  thrown = true;
               }
               if(thrown == false){
                  return 1;
               }
               //Create a mapped region
               mapped_region region (mapping, read_write, 0, FileSize, 0);

               //Fill two regions with a pattern
               unsigned char *filler = static_cast<unsigned char*>(region.get_address());
               for(std::size_t i = 0; i < FileSize; ++i){
                  *filler++ = static_cast<unsigned char>(i);
               }
            }

            //Now check the pattern mapping a single read only mapped_region
            {
               //Create a single region, mapping all the file
               mapped_region region (mapping, read_only);

               //Check pattern
               unsigned char *pattern = static_cast<unsigned char*>(region.get_address());
               for(std::size_t i = 0; i < FileSize; ++i, ++pattern){
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
      }
   }
   catch(std::exception &exc){
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

#endif   //BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS

#include <boost/interprocess/detail/config_end.hpp>
