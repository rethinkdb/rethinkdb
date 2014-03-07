//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_CONTAINER_PROPAGATE_ALLOCATOR_TEST_HPP
#define BOOST_CONTAINER_PROPAGATE_ALLOCATOR_TEST_HPP

#include <boost/container/detail/config_begin.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "dummy_test_allocator.hpp"

#include <iostream>

namespace boost{
namespace container {
namespace test{

template<template<class, class> class ContainerWrapper>
bool test_propagate_allocator()
{
   {
      typedef propagation_test_allocator<char, true, true, true, true>  AlwaysPropagate;
      typedef ContainerWrapper<char, AlwaysPropagate>                   PropagateCont;

      //////////////////////////////////////////
      //Test AlwaysPropagate allocator propagation
      //////////////////////////////////////////
      AlwaysPropagate::reset_unique_id();

      //default constructor
      PropagateCont c;
      BOOST_TEST (c.get_stored_allocator().id_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_copies_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_moves_ == 0);
      BOOST_TEST (c.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c.get_stored_allocator().swaps_ == 0);

      //copy constructor
      PropagateCont c2(c);
      //propagate_on_copy_constructor produces copies, moves or RVO (depending on the compiler).
      //For allocators that copy in select_on_container_copy_construction, at least we must have a copy
      unsigned int ctr_copies = c2.get_stored_allocator().ctr_copies_;
      unsigned int ctr_moves = c2.get_stored_allocator().ctr_moves_;
      BOOST_TEST (c2.get_stored_allocator().id_ == 0);
      BOOST_TEST (ctr_copies > 0);
      BOOST_TEST (c2.get_stored_allocator().ctr_moves_ >= 0);
      BOOST_TEST (c2.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c2.get_stored_allocator().swaps_ == 0);

      //move constructor
      PropagateCont c3(boost::move(c2));
      BOOST_TEST (c3.get_stored_allocator().id_ == 0);
      BOOST_TEST (c3.get_stored_allocator().ctr_copies_ == ctr_copies);
      BOOST_TEST (c3.get_stored_allocator().ctr_moves_ > ctr_moves);
      ctr_moves = c3.get_stored_allocator().ctr_moves_;
      BOOST_TEST (ctr_moves > 0);
      BOOST_TEST (c3.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c3.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c3.get_stored_allocator().swaps_ == 0);

      //copy assign
      c2 = c3;
      unsigned int assign_copies = c2.get_stored_allocator().assign_copies_;
      BOOST_TEST (c2.get_stored_allocator().id_ == 0);
      BOOST_TEST (c2.get_stored_allocator().ctr_copies_ == ctr_copies);
      BOOST_TEST (c2.get_stored_allocator().ctr_moves_  == ctr_moves);
      BOOST_TEST (assign_copies == 1);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c2.get_stored_allocator().swaps_ == 0);

      //move assign
      c = boost::move(c2);
      unsigned int assign_moves = c.get_stored_allocator().assign_moves_;
      BOOST_TEST (c.get_stored_allocator().id_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_copies_    == ctr_copies);
      BOOST_TEST (c.get_stored_allocator().ctr_moves_     == ctr_moves);
      BOOST_TEST (c.get_stored_allocator().assign_copies_ == assign_copies);
      BOOST_TEST (assign_moves == 1);
      BOOST_TEST (c.get_stored_allocator().swaps_ == 0);

      //swap
      c.get_stored_allocator().id_ = 999;
      c.swap(c2);
      unsigned int swaps = c2.get_stored_allocator().swaps_;
      BOOST_TEST (c2.get_stored_allocator().id_ == 999);
      BOOST_TEST (c2.get_stored_allocator().ctr_copies_    == ctr_copies);
      BOOST_TEST (c2.get_stored_allocator().ctr_moves_     == ctr_moves);
      BOOST_TEST (c2.get_stored_allocator().assign_copies_ == assign_copies);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_  == assign_moves);
      BOOST_TEST (swaps == 1);
   }

   //////////////////////////////////////////
   //Test NeverPropagate allocator propagation
   //////////////////////////////////////////
   {
      typedef propagation_test_allocator<char, false, false, false, false> NeverPropagate;
      typedef ContainerWrapper<char, NeverPropagate>                       NoPropagateCont;
      NeverPropagate::reset_unique_id();

      //default constructor
      NoPropagateCont c;
      BOOST_TEST (c.get_stored_allocator().id_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_copies_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_moves_ == 0);
      BOOST_TEST (c.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c.get_stored_allocator().swaps_ == 0);

      //copy constructor
      //propagate_on_copy_constructor produces copies, moves or RVO (depending on the compiler)
      //For allocators that don't copy in select_on_container_copy_construction we must have a default
      //construction
      NoPropagateCont c2(c);
      unsigned int ctr_copies = c2.get_stored_allocator().ctr_copies_;
      unsigned int ctr_moves = c2.get_stored_allocator().ctr_moves_;
      BOOST_TEST (c2.get_stored_allocator().id_ == 1);
      BOOST_TEST (ctr_copies >= 0);
      BOOST_TEST (ctr_moves >= 0);
      BOOST_TEST (c2.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c2.get_stored_allocator().swaps_ == 0);

      //move constructor
      NoPropagateCont c3(boost::move(c2));
      BOOST_TEST (c3.get_stored_allocator().id_ == 1);
      BOOST_TEST (c3.get_stored_allocator().ctr_copies_ == ctr_copies);
      BOOST_TEST (c3.get_stored_allocator().ctr_moves_ > ctr_moves);
      unsigned int ctr_moves2 = ctr_moves;
      ctr_moves = c3.get_stored_allocator().ctr_moves_;
      BOOST_TEST (c3.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c3.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c3.get_stored_allocator().swaps_ == 0);

      //copy assign
      c2 = c3;
      BOOST_TEST (c2.get_stored_allocator().id_ == 1);
      BOOST_TEST (c2.get_stored_allocator().ctr_copies_ == ctr_copies);
      BOOST_TEST (c2.get_stored_allocator().ctr_moves_  == ctr_moves2);
      BOOST_TEST (c2.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c2.get_stored_allocator().swaps_ == 0);

      //move assign
      c = boost::move(c2);
      BOOST_TEST (c.get_stored_allocator().id_ == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_copies_    == 0);
      BOOST_TEST (c.get_stored_allocator().ctr_moves_     == 0);
      BOOST_TEST (c.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c.get_stored_allocator().swaps_ == 0);

      //swap
      c.get_stored_allocator().id_ = 999;
      c2.swap(c);
      BOOST_TEST (c2.get_stored_allocator().id_ == 1);
      BOOST_TEST (c.get_stored_allocator().id_ == 999);
      BOOST_TEST (c2.get_stored_allocator().ctr_copies_    == ctr_copies);
      BOOST_TEST (c2.get_stored_allocator().ctr_moves_  == ctr_moves2);
      BOOST_TEST (c2.get_stored_allocator().assign_copies_ == 0);
      BOOST_TEST (c2.get_stored_allocator().assign_moves_ == 0);
      BOOST_TEST (c2.get_stored_allocator().swaps_ == 0);
   }

   return report_errors() == 0;
}

}  //namespace test{
}  //namespace container {
}  //namespace boost{

#include <boost/container/detail/config_end.hpp>

#endif   //#ifndef BOOST_CONTAINER_PROPAGATE_ALLOCATOR_TEST_HPP
