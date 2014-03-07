//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <cstddef>
#include <cassert>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;
typedef basic_managed_shared_memory
   <char, simple_seq_fit<mutex_family>, null_index>
my_shared_objects_t;

int main ()
{
   //Create shared memory
   shared_memory_object::remove(test::get_process_id_name());
   {
      my_shared_objects_t segment
         (create_only,
         test::get_process_id_name(), //segment name
         65536);                    //segment size in bytes

      //Allocate a portion of the segment
      void * shptr   = segment.allocate(1024/*bytes to allocate*/);
      my_shared_objects_t::handle_t handle = segment.get_handle_from_address(shptr);
      if(!segment.belongs_to_segment(shptr)){
         return 1;
      }
      if(shptr != segment.get_address_from_handle(handle)){
         return 1;
      }

      segment.deallocate(shptr);
   }
   shared_memory_object::remove(test::get_process_id_name());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
