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
#include <boost/container/list.hpp>
#include "dummy_test_allocator.hpp"
#include <memory>
#include "movable_int.hpp"
#include "list_test.hpp"
#include "propagate_allocator_test.hpp"
#include "emplace_test.hpp"

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors
template class boost::container::list<test::movable_and_copyable_int,
   test::simple_allocator<test::movable_and_copyable_int> >;

template class boost::container::list<test::movable_and_copyable_int,
   test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class boost::container::list<test::movable_and_copyable_int,
   std::allocator<test::movable_and_copyable_int> >;

namespace container_detail {

template class iterator
   <intrusive_list_type< std::allocator<int> >::container_type::iterator, true >;
template class iterator
   <intrusive_list_type< std::allocator<int> >::container_type::iterator, false>;

}

}}

typedef list<int> MyList;

typedef list<test::movable_int> MyMoveList;
typedef list<test::movable_and_copyable_int> MyCopyMoveList;
typedef list<test::copyable_int> MyCopyList;

class recursive_list
{
public:
   int id_;
   list<recursive_list> list_;
   recursive_list &operator=(const recursive_list &o)
   { list_ = o.list_;  return *this; }
};

void recursive_list_test()//Test for recursive types
{
   list<recursive_list> recursive, copy;
   //Test to test both move emulations
   if(!copy.size()){
      copy = recursive;
   }
}

int main ()
{
   recursive_list_test();
   {
      //Now test move semantics
      list<recursive_list> original;
      list<recursive_list> move_ctor(boost::move(original));
      list<recursive_list> move_assign;
      move_assign = boost::move(move_ctor);
      move_assign.swap(original);
   }
   if(test::list_test<MyList, true>())
      return 1;

   if(test::list_test<MyMoveList, true>())
      return 1;

   if(test::list_test<MyCopyMoveList, true>())
      return 1;

   if(test::list_test<MyCopyList, true>())
      return 1;

   const test::EmplaceOptions Options = (test::EmplaceOptions)(test::EMPLACE_BACK | test::EMPLACE_FRONT | test::EMPLACE_BEFORE);

   if(!boost::container::test::test_emplace<list<test::EmplaceInt>, Options>())
      return 1;

   if(!boost::container::test::test_propagate_allocator<list>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>
