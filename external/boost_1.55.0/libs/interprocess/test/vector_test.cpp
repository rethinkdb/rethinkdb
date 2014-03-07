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
#include <algorithm>
#include <memory>
#include <vector>
#include <iostream>
#include <functional>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "allocator_v1.hpp"
#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"
#include "dummy_test_allocator.hpp"
#include "vector_test.hpp"

using namespace boost::interprocess;

int test_expand_bwd()
{
   //Now test all back insertion possibilities

   //First raw ints
   typedef test::expand_bwd_test_allocator<int>
      int_allocator_type;
   typedef vector<int, int_allocator_type>
      int_vector;

   if(!test::test_all_expand_bwd<int_vector>())
      return 1;

   //Now user defined wrapped int
   typedef test::expand_bwd_test_allocator<test::int_holder>
      int_holder_allocator_type;
   typedef vector<test::int_holder, int_holder_allocator_type>
      int_holder_vector;

   if(!test::test_all_expand_bwd<int_holder_vector>())
      return 1;

   //Now user defined bigger wrapped int
   typedef test::expand_bwd_test_allocator<test::triple_int_holder>
      triple_int_holder_allocator_type;

   typedef vector<test::triple_int_holder, triple_int_holder_allocator_type>
      triple_int_holder_vector;

   if(!test::test_all_expand_bwd<triple_int_holder_vector>())
      return 1;

   return 0;
}

int main()
{
   typedef allocator<int, managed_shared_memory::segment_manager> ShmemAllocator;
   typedef vector<int, ShmemAllocator> MyVector;

   typedef test::allocator_v1<int, managed_shared_memory::segment_manager> ShmemV1Allocator;
   typedef vector<int, ShmemV1Allocator> MyV1Vector;

   typedef allocator<test::movable_int, managed_shared_memory::segment_manager> ShmemMoveAllocator;
   typedef vector<test::movable_int, ShmemMoveAllocator> MyMoveVector;

   typedef allocator<test::movable_and_copyable_int, managed_shared_memory::segment_manager> ShmemCopyMoveAllocator;
   typedef vector<test::movable_and_copyable_int, ShmemCopyMoveAllocator> MyCopyMoveVector;

   typedef allocator<test::copyable_int, managed_shared_memory::segment_manager> ShmemCopyAllocator;
   typedef vector<test::copyable_int, ShmemCopyAllocator> MyCopyVector;

   if(test::vector_test<managed_shared_memory, MyVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyV1Vector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyMoveVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyCopyMoveVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyCopyVector>())
      return 1;

   if(test_expand_bwd())
      return 1;

   const test::EmplaceOptions Options = (test::EmplaceOptions)(test::EMPLACE_BACK | test::EMPLACE_BEFORE);
   if(!boost::interprocess::test::test_emplace
      < vector<test::EmplaceInt>, Options>())
      return 1;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
