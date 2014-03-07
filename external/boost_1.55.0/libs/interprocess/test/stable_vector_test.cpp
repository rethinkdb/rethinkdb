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
#include <boost/interprocess/containers/stable_vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "allocator_v1.hpp"
#include "heap_allocator_v1.hpp"
#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"
#include "dummy_test_allocator.hpp"
#include "vector_test.hpp"

using namespace boost::interprocess;

int main()
{
   typedef allocator<int, managed_shared_memory::segment_manager> ShmemAllocator;
   typedef stable_vector<int, ShmemAllocator> MyVector;

   typedef test::allocator_v1<int, managed_shared_memory::segment_manager> ShmemV1Allocator;
   typedef stable_vector<int, ShmemV1Allocator> MyV1Vector;

   typedef test::heap_allocator_v1<int, managed_shared_memory::segment_manager> ShmemHeapV1Allocator;
   typedef stable_vector<int, ShmemHeapV1Allocator> MyHeapV1Vector;

   typedef allocator<test::movable_int, managed_shared_memory::segment_manager> ShmemMoveAllocator;
   typedef stable_vector<test::movable_int, ShmemMoveAllocator> MyMoveVector;

   typedef allocator<test::movable_and_copyable_int, managed_shared_memory::segment_manager> ShmemCopyMoveAllocator;
   typedef stable_vector<test::movable_and_copyable_int, ShmemCopyMoveAllocator> MyCopyMoveVector;

   typedef allocator<test::copyable_int, managed_shared_memory::segment_manager> ShmemCopyAllocator;
   typedef stable_vector<test::copyable_int, ShmemCopyAllocator> MyCopyVector;

   if(test::vector_test<managed_shared_memory, MyVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyV1Vector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyHeapV1Vector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyMoveVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyCopyMoveVector>())
      return 1;

   if(test::vector_test<managed_shared_memory, MyCopyVector>())
      return 1;

   const test::EmplaceOptions Options = (test::EmplaceOptions)(test::EMPLACE_BACK | test::EMPLACE_BEFORE);
   if(!boost::interprocess::test::test_emplace
      < stable_vector<test::EmplaceInt>, Options>())
      return 1;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
