//////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2002, 2003 Peter Dimov
//
// This file is the adaptation of shared_from_this_test.cpp from smart_ptr library
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/smart_ptr/enable_shared_from_this.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "get_process_id_name.hpp"

//

using namespace boost::interprocess;

typedef allocator<void, managed_shared_memory::segment_manager>
   v_allocator_t;

struct X;

typedef deleter<X, managed_shared_memory::segment_manager>  x_deleter_t;

struct X :
   public enable_shared_from_this<X, v_allocator_t, x_deleter_t>
{
};

typedef shared_ptr<X, v_allocator_t, x_deleter_t>           v_shared_ptr;


template<class ManagedMemory>
void test_enable_shared_this(ManagedMemory &managed_mem)
{
   v_shared_ptr p(make_managed_shared_ptr
      (managed_mem.template construct<X>(anonymous_instance)(), managed_mem));

   v_shared_ptr q = p->shared_from_this();
   BOOST_TEST(p == q);
   BOOST_TEST(!(p < q) && !(q < p));

   X v2(*p);

   try
   {
      //This should throw bad_weak_ptr
      v_shared_ptr r = v2.shared_from_this();
      BOOST_ERROR("v2.shared_from_this() failed to throw");
   }
   catch(boost::interprocess::bad_weak_ptr const &)
   {
      //This is the expected path
   }
   catch(...){
      BOOST_ERROR("v2.shared_from_this() threw an unexpected exception");
   }

   try
   {
      //This should not throw bad_weak_ptr
      *p = X();
      v_shared_ptr r = p->shared_from_this();
      BOOST_TEST(p == r);
      BOOST_TEST(!(p < r) && !(r < p));
   }
   catch(boost::interprocess::bad_weak_ptr const &)
   {
      BOOST_ERROR("p->shared_from_this() threw bad_weak_ptr after *p = X()");
   }
   catch(...)
   {
      BOOST_ERROR("p->shared_from_this() threw an unexpected exception after *p = X()");
   }
}


int main()
{
   std::string process_name;
   test::get_process_id_name(process_name);
   shared_memory_object::remove(process_name.c_str());
   managed_shared_memory shmem(create_only, process_name.c_str(), 65536);
   test_enable_shared_this(shmem);
   shared_memory_object::remove(process_name.c_str());
   return boost::report_errors();
}

#include <boost/interprocess/detail/config_end.hpp>
