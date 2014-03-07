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
#include <boost/interprocess/detail/os_file_functions.hpp>
//[doc_managed_copy_on_write
#include <boost/interprocess/managed_mapped_file.hpp>
#include <fstream> //std::fstream
#include <iterator>//std::distance

//<-
#include "../test/get_process_id_name.hpp"
//->

int main()
{
   using namespace boost::interprocess;

   //Define file names
   //<-
   #if 1
   const char *ManagedFile  = 0;
   const char *ManagedFile2 = 0;
   std::string managed_file_name(boost::interprocess::ipcdetail::get_temporary_path());
   managed_file_name += "/"; managed_file_name += test::get_process_id_name();
   ManagedFile = managed_file_name.c_str();
   std::string managed_file2_name(boost::interprocess::ipcdetail::get_temporary_path());
   managed_file2_name += "/"; managed_file2_name += test::get_process_id_name();  managed_file2_name += "_2";
   ManagedFile2 = managed_file2_name.c_str();
   #else
   //->
   const char *ManagedFile  = "MyManagedFile";
   const char *ManagedFile2 = "MyManagedFile2";
   //<-
   #endif
   //->

   //Try to erase any previous managed segment with the same name
   file_mapping::remove(ManagedFile);
   file_mapping::remove(ManagedFile2);
   remove_file_on_destroy destroyer1(ManagedFile);
   remove_file_on_destroy destroyer2(ManagedFile2);

   {
      //Create an named integer in a managed mapped file
      managed_mapped_file managed_file(create_only, ManagedFile, 65536);
      managed_file.construct<int>("MyInt")(0u);

      //Now create a copy on write version
      managed_mapped_file managed_file_cow(open_copy_on_write, ManagedFile);

      //Erase the int and create a new one
      if(!managed_file_cow.destroy<int>("MyInt"))
         throw int(0);
      managed_file_cow.construct<int>("MyInt2");

      //Check changes
      if(managed_file_cow.find<int>("MyInt").first && !managed_file_cow.find<int>("MyInt2").first)
         throw int(0);

      //Check the original is intact
      if(!managed_file.find<int>("MyInt").first && managed_file.find<int>("MyInt2").first)
         throw int(0);

      {  //Dump the modified copy on write segment to a file
         std::fstream file(ManagedFile2, std::ios_base::out | std::ios_base::binary);
         if(!file)
            throw int(0);
		 file.write(static_cast<const char *>(managed_file_cow.get_address()), (std::streamsize)managed_file_cow.get_size());
      }

      //Now open the modified file and test changes
      managed_mapped_file managed_file_cow2(open_only, ManagedFile2);
      if(managed_file_cow2.find<int>("MyInt").first && !managed_file_cow2.find<int>("MyInt2").first)
         throw int(0);
   }
   {
      //Now create a read-only version
      managed_mapped_file managed_file_ro(open_read_only, ManagedFile);

      //Check the original is intact
      if(!managed_file_ro.find<int>("MyInt").first && managed_file_ro.find<int>("MyInt2").first)
         throw int(0);

      //Check the number of named objects using the iterators
      if(std::distance(managed_file_ro.named_begin(),  managed_file_ro.named_end())  != 1 &&
         std::distance(managed_file_ro.unique_begin(), managed_file_ro.unique_end()) != 0 )
         throw int(0);
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
