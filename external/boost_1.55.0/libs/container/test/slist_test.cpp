//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/slist.hpp>
#include <memory>
#include "dummy_test_allocator.hpp"
#include "movable_int.hpp"
#include "list_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors
template class boost::container::slist<test::movable_and_copyable_int,
   test::simple_allocator<test::movable_and_copyable_int> >;

template class boost::container::slist<test::movable_and_copyable_int,
   test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::container::slist<test::movable_and_copyable_int,
   std::allocator<test::movable_and_copyable_int> >;

}}

typedef slist<int> MyList;
typedef slist<test::movable_int> MyMoveList;
typedef slist<test::movable_and_copyable_int> MyCopyMoveList;
typedef slist<test::copyable_int> MyCopyList;

class recursive_slist
{
public:
   int id_;
   slist<recursive_slist> slist_;
   recursive_slist &operator=(const recursive_slist &o)
   { slist_ = o.slist_;  return *this; }
};

void recursive_slist_test()//Test for recursive types
{
   slist<recursive_slist> recursive_list_list;
}

int main ()
{
   recursive_slist_test();
   {
      //Now test move semantics
      slist<recursive_slist> original;
      slist<recursive_slist> move_ctor(boost::move(original));
      slist<recursive_slist> move_assign;
      move_assign = boost::move(move_ctor);
      move_assign.swap(original);
      {
         slist<recursive_slist> recursive, copy;
         //Test to test both move emulations
         if(!copy.size()){
            copy = recursive;
         }
      }
   }

   if(test::list_test<MyList, false>())
      return 1;

   if(test::list_test<MyMoveList, false>())
      return 1;

   if(test::list_test<MyCopyMoveList, false>())
      return 1;

   if(test::list_test<MyCopyList, false>())
      return 1;

   const test::EmplaceOptions Options = (test::EmplaceOptions)
      (test::EMPLACE_FRONT | test::EMPLACE_AFTER | test::EMPLACE_BEFORE  | test::EMPLACE_AFTER);

   if(!boost::container::test::test_emplace
      < slist<test::EmplaceInt>, Options>())
      return 1;

   if(!boost::container::test::test_propagate_allocator<slist>())
      return 1;
}

#include <boost/container/detail/config_end.hpp>

