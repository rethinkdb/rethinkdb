//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cstddef>
#include <cstdio>

//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;
typedef list<int, allocator<int, managed_mapped_file::segment_manager> >
   MyList;

int main ()
{
   //Define file names
   //<-
   #if 1
   std::string file(boost::interprocess::ipcdetail::get_temporary_path());
   file += "/"; file += test::get_process_id_name();
   const char *FileName = file.c_str();
   #else
   //->
   const char *FileName       = "file_mapping";
   //<-
   #endif
   //->

   const std::size_t FileSize = 1000;
   file_mapping::remove(FileName);

   try{
      MyList::size_type old_size = 0;
      managed_mapped_file::handle_t list_handle;
      {
         managed_mapped_file mfile_memory(create_only, FileName, FileSize);
         MyList *mylist = mfile_memory.construct<MyList>("MyList")
                              (mfile_memory.get_segment_manager());

         //Obtain handle, that identifies the list in the buffer
         list_handle = mfile_memory.get_handle_from_address(mylist);

         //Fill list until there is no more room in the file
         try{
            while(1) {
               mylist->insert(mylist->begin(), 0);
            }
         }
         catch(const bad_alloc &){
            //mapped file is full
         }
         //Let's obtain the size of the list
         old_size = mylist->size();
      }
      //To make the list bigger, let's increase the mapped file
      //in FileSize bytes more.
      managed_mapped_file::grow(FileName, FileSize*2);

      {
         managed_mapped_file mfile_memory(open_only, FileName);


         //If mapping address has changed, the old pointer is invalid,
         //so use previously obtained handle to find the new pointer.
         MyList *mylist = static_cast<MyList *>
                           (mfile_memory.get_address_from_handle(list_handle));

         //Fill list until there is no more room in the file
         try{
            while(1) {
               mylist->insert(mylist->begin(), 0);
            }
         }
         catch(const bad_alloc &){
            //mapped file is full
         }

         //Let's obtain the new size of the list
         MyList::size_type new_size = mylist->size();

         assert(new_size > old_size);

         //Destroy list
         mfile_memory.destroy_ptr(mylist);
      }
   }
   catch(...){
      file_mapping::remove(FileName);
      throw;
   }
   file_mapping::remove(FileName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
