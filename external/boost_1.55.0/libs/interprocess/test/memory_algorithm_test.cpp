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
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include "memory_algorithm_test_template.hpp"
#include <iostream>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

const int Memsize = 16384;
const char *const shMemName = test::get_process_id_name();

int test_simple_seq_fit()
{
   //A shared memory with simple sequential fit algorithm
   typedef basic_managed_shared_memory
      <char
      ,simple_seq_fit<mutex_family>
      ,null_index
      > my_managed_shared_memory;

   //Create shared memory
   shared_memory_object::remove(shMemName);
   my_managed_shared_memory segment(create_only, shMemName, Memsize);

   //Now take the segment manager and launch memory test
   if(!test::test_all_allocation(*segment.get_segment_manager())){
      return 1;
   }
   return 0;
}

template<std::size_t Alignment>
int test_rbtree_best_fit()
{
   //A shared memory with red-black tree best fit algorithm
   typedef basic_managed_shared_memory
      <char
      ,rbtree_best_fit<mutex_family, offset_ptr<void>, Alignment>
      ,null_index
      > my_managed_shared_memory;

   //Create shared memory
   shared_memory_object::remove(shMemName);
   my_managed_shared_memory segment(create_only, shMemName, Memsize);

   //Now take the segment manager and launch memory test
   if(!test::test_all_allocation(*segment.get_segment_manager())){
      return 1;
   }
   return 0;
}

int main ()
{
   const std::size_t void_ptr_align = ::boost::alignment_of<offset_ptr<void> >::value;

   if(test_simple_seq_fit()){
      return 1;
   }
   if(test_rbtree_best_fit<void_ptr_align>()){
      return 1;
   }
   if(test_rbtree_best_fit<2*void_ptr_align>()){
      return 1;
   }
   if(test_rbtree_best_fit<4*void_ptr_align>()){
      return 1;
   }

   shared_memory_object::remove(shMemName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
