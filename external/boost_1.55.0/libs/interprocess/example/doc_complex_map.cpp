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
//[doc_complex_map
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

//Typedefs of allocators and containers
typedef managed_shared_memory::segment_manager                       segment_manager_t;
typedef allocator<void, segment_manager_t>                           void_allocator;
typedef allocator<int, segment_manager_t>                            int_allocator;
typedef vector<int, int_allocator>                                   int_vector;
typedef allocator<int_vector, segment_manager_t>                     int_vector_allocator;
typedef vector<int_vector, int_vector_allocator>                     int_vector_vector;
typedef allocator<char, segment_manager_t>                           char_allocator;
typedef basic_string<char, std::char_traits<char>, char_allocator>   char_string;

class complex_data
{
   int               id_;
   char_string       char_string_;
   int_vector_vector int_vector_vector_;

   public:
   //Since void_allocator is convertible to any other allocator<T>, we can simplify
   //the initialization taking just one allocator for all inner containers.
   complex_data(int id, const char *name, const void_allocator &void_alloc)
      : id_(id), char_string_(name, void_alloc), int_vector_vector_(void_alloc)
   {}
   //Other members...
};

//Definition of the map holding a string as key and complex_data as mapped type
typedef std::pair<const char_string, complex_data>                      map_value_type;
typedef std::pair<char_string, complex_data>                            movable_to_map_value_type;
typedef allocator<map_value_type, segment_manager_t>                    map_value_type_allocator;
typedef map< char_string, complex_data
           , std::less<char_string>, map_value_type_allocator>          complex_map_type;

int main ()
{
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

   //Create shared memory
   //<-
   #if 1
   managed_shared_memory segment(create_only,test::get_process_id_name(), 65536);
   #else
   //->
   managed_shared_memory segment(create_only,"MySharedMemory", 65536);
   //<-
   #endif
   //->

   //An allocator convertible to any allocator<T, segment_manager_t> type
   void_allocator alloc_inst (segment.get_segment_manager());

   //Construct the shared memory map and fill it
   complex_map_type *mymap = segment.construct<complex_map_type>
      //(object name), (first ctor parameter, second ctor parameter)
         ("MyMap")(std::less<char_string>(), alloc_inst);

   for(int i = 0; i < 100; ++i){
      //Both key(string) and value(complex_data) need an allocator in their constructors
      char_string  key_object(alloc_inst);
      complex_data mapped_object(i, "default_name", alloc_inst);
      map_value_type value(key_object, mapped_object);
      //Modify values and insert them in the map
      mymap->insert(value);
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
