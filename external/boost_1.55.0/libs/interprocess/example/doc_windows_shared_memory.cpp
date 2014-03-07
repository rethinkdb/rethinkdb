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

#if defined(BOOST_INTERPROCESS_WINDOWS) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

//[doc_windows_shared_memory
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
//<-
#include "../test/get_process_id_name.hpp"
//->

int main(int argc, char *argv[])
{
   using namespace boost::interprocess;

   if(argc == 1){  //Parent process
      //Create a native windows shared memory object.
      //<-
      #if 1
      windows_shared_memory shm (create_only, test::get_process_id_name(), read_write, 1000);
      #else
      //->
      windows_shared_memory shm (create_only, "MySharedMemory", read_write, 1000);
      //<-
      #endif
      //->

      //Map the whole shared memory in this process
      mapped_region region(shm, read_write);

      //Write all the memory to 1
      std::memset(region.get_address(), 1, region.get_size());

      //Launch child process
      std::string s(argv[0]); s += " child ";
      //<-
      s += test::get_process_id_name();
      //->
      if(0 != std::system(s.c_str()))
         return 1;
      //windows_shared_memory is destroyed when the last attached process dies...
   }
   else{
      //Open already created shared memory object.
      //<-
      #if 1
      windows_shared_memory shm (open_only, argv[2], read_only);
      #else
      //->
      windows_shared_memory shm (open_only, "MySharedMemory", read_only);
      //<-
      #endif
      //->

      //Map the whole shared memory in this process
      mapped_region region(shm, read_only);

      //Check that memory was initialized to 1
      char *mem = static_cast<char*>(region.get_address());
      for(std::size_t i = 0; i < region.get_size(); ++i)
         if(*mem++ != 1)
            return 1;   //Error checking memory
      return 0;
   }
   return 0;
}
//]

#else //BOOST_INTERPROCESS_WINDOWS

int main()
{
   return 0;
}

#endif //BOOST_INTERPROCESS_WINDOWS

#include <boost/interprocess/detail/config_end.hpp>
