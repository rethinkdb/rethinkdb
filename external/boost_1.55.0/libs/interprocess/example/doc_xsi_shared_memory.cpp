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

#if defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

//[doc_xsi_shared_memory
#include <boost/interprocess/xsi_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace boost::interprocess;

void remove_old_shared_memory(const xsi_key &key)
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

int main(int argc, char *argv[])
{
   if(argc == 1){  //Parent process
      //Build XSI key (ftok based)
      xsi_key key(argv[0], 1);

      remove_old_shared_memory(key);

      //Create a shared memory object.
      xsi_shared_memory shm (create_only, key, 1000);

      //Remove shared memory on destruction
      struct shm_remove
      {
         int shmid_;
         shm_remove(int shmid) : shmid_(shmid){}
         ~shm_remove(){ xsi_shared_memory::remove(shmid_); }
      } remover(shm.get_shmid());

      //Map the whole shared memory in this process
      mapped_region region(shm, read_write);

      //Write all the memory to 1
      std::memset(region.get_address(), 1, region.get_size());

      //Launch child process
      std::string s(argv[0]); s += " child ";
      if(0 != std::system(s.c_str()))
         return 1;
   }
   else{
      //Build XSI key (ftok based)
      xsi_key key(argv[0], 1);

      //Create a shared memory object.
      xsi_shared_memory shm (open_only, key);

      //Map the whole shared memory in this process
      mapped_region region(shm, read_only);

      //Check that memory was initialized to 1
      char *mem = static_cast<char*>(region.get_address());
      for(std::size_t i = 0; i < region.get_size(); ++i)
         if(*mem++ != 1)
            return 1;   //Error checking memory
   }
   return 0;
}
//]

#else //BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS

int main()
{
   return 0;
}

#endif //BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS

#include <boost/interprocess/detail/config_end.hpp>
