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
//[doc_named_alloc
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <cstddef>
#include <cassert>
#include <utility>
//<-
#include "../test/get_process_id_name.hpp"
//->

int main(int argc, char *argv[])
{
   using namespace boost::interprocess;
   typedef std::pair<double, int> MyType;

   if(argc == 1){  //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
      //<-
      #if 1
         shm_remove() { shared_memory_object::remove(test::get_process_id_name()); }
         ~shm_remove(){ shared_memory_object::remove(test::get_process_id_name()); }
      #else
      //->
         shm_remove() { shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
      //<-
      #endif
      //->
      } remover;
      //<-
      (void)remover;
      //->

      //Construct managed shared memory
      //<-
      #if 1
      managed_shared_memory segment(create_only, test::get_process_id_name(), 65536);
      #else
      //->
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);
      //<-
      #endif
      //->

      //Create an object of MyType initialized to {0.0, 0}
      MyType *instance = segment.construct<MyType>
         ("MyType instance")  //name of the object
         (0.0, 0);            //ctor first argument

      //Create an array of 10 elements of MyType initialized to {0.0, 0}
      MyType *array = segment.construct<MyType>
         ("MyType array")     //name of the object
         [10]                 //number of elements
         (0.0, 0);            //Same two ctor arguments for all objects

      //Create an array of 3 elements of MyType initializing each one
      //to a different value {0.0, 0}, {1.0, 1}, {2.0, 2}...
      float float_initializer[3] = { 0.0, 1.0, 2.0 };
      int   int_initializer[3]   = { 0, 1, 2 };

      MyType *array_it = segment.construct_it<MyType>
         ("MyType array from it")   //name of the object
         [3]                        //number of elements
         ( &float_initializer[0]    //Iterator for the 1st ctor argument
         , &int_initializer[0]);    //Iterator for the 2nd ctor argument

      //Launch child process
      std::string s(argv[0]); s += " child ";
      //<-
      s += test::get_process_id_name();
      //->
      if(0 != std::system(s.c_str()))
         return 1;

      //<-
      (void)instance;
      (void)array;
      (void)array_it;
      //->

      //Check child has destroyed all objects
      if(segment.find<MyType>("MyType array").first ||
         segment.find<MyType>("MyType instance").first ||
         segment.find<MyType>("MyType array from it").first)
         return 1;
   }
   else{
      //Open managed shared memory
      //<-
      #if 1
      managed_shared_memory segment(open_only, argv[2]);
      #else
      //->
      managed_shared_memory segment(open_only, "MySharedMemory");
      //<-
      #endif
      //->

      std::pair<MyType*, managed_shared_memory::size_type> res;

      //Find the array
      res = segment.find<MyType> ("MyType array");
      //Length should be 10
      if(res.second != 10) return 1;

      //Find the object
      res = segment.find<MyType> ("MyType instance");
      //Length should be 1
      if(res.second != 1) return 1;

      //Find the array constructed from iterators
      res = segment.find<MyType> ("MyType array from it");
      //Length should be 3
      if(res.second != 3) return 1;

      //We're done, delete all the objects
      segment.destroy<MyType>("MyType array");
      segment.destroy<MyType>("MyType instance");
      segment.destroy<MyType>("MyType array from it");
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
