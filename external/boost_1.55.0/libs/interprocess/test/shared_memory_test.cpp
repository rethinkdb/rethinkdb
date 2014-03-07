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
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "named_creation_template.hpp"
#include <cstring>   //for strcmp, memset
#include <iostream>  //for cout
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

static const std::size_t ShmSize = 1000;
static const char *      ShmName = test::get_process_id_name();

struct eraser
{
   ~eraser()
   {
      shared_memory_object::remove(ShmName);
   }
};

typedef ipcdetail::managed_open_or_create_impl<shared_memory_object, 0, true, false> shared_memory;

//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class shared_memory_creation_test_wrapper
   : public eraser
   , public shared_memory
{

   public:
   shared_memory_creation_test_wrapper(create_only_t)
      :  shared_memory(create_only, ShmName, ShmSize, read_write, 0, permissions())
   {}

   shared_memory_creation_test_wrapper(open_only_t)
      :  shared_memory(open_only, ShmName, read_write, 0)
   {}

   shared_memory_creation_test_wrapper(open_or_create_t)
      :  shared_memory(open_or_create, ShmName, ShmSize, read_write, 0, permissions())
   {}
};


int main ()
{
   try{
      shared_memory_object::remove(ShmName);
      test::test_named_creation<shared_memory_creation_test_wrapper>();

      //Create and get name, size and address
      {
         shared_memory_object::remove(ShmName);
         shared_memory shm1(create_only, ShmName, ShmSize, read_write, 0, permissions());

         //Overwrite all memory
         std::memset(shm1.get_user_address(), 0, shm1.get_user_size());

         //Now test move semantics
         shared_memory move_ctor(boost::move(shm1));
         shared_memory move_assign;
         move_assign = boost::move(move_ctor);
      }
   }
   catch(std::exception &ex){
      shared_memory_object::remove(ShmName);
      std::cout << ex.what() << std::endl;
      return 1;
   }
   shared_memory_object::remove(ShmName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
