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
#include <algorithm>
#include <memory>
#include <vector>
#include <iostream>
#include <functional>

#include <boost/container/stable_vector.hpp>
#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include "expand_bwd_test_allocator.hpp"
#include "expand_bwd_test_template.hpp"
#include "dummy_test_allocator.hpp"
#include "propagate_allocator_test.hpp"
#include "vector_test.hpp"

using namespace boost::container;

namespace boost {
namespace container {

//Explicit instantiation to detect compilation errors
template class stable_vector<test::movable_and_copyable_int,
   test::dummy_test_allocator<test::movable_and_copyable_int> >;

template class stable_vector<test::movable_and_copyable_int,
   test::simple_allocator<test::movable_and_copyable_int> >;

template class stable_vector<test::movable_and_copyable_int,
   std::allocator<test::movable_and_copyable_int> >;

namespace stable_vector_detail{

template class iterator<int*, false>;
template class iterator<int*, true >;

}  //namespace stable_vector_detail{

}}

class recursive_vector
{
   public:
   int id_;
   stable_vector<recursive_vector> vector_;
   recursive_vector &operator=(const recursive_vector &o)
   { vector_ = o.vector_;  return *this; }
};

void recursive_vector_test()//Test for recursive types
{
   stable_vector<recursive_vector> recursive, copy;
   //Test to test both move emulations
   if(!copy.size()){
      copy = recursive;
   }
}

bool default_init_test()//Test for default initialization
{
   typedef stable_vector<int, test::default_init_allocator<int> > svector_t;

   const std::size_t Capacity = 100;

   {
      test::default_init_allocator<int>::reset_pattern(0);
      svector_t v(Capacity, default_init);
      svector_t::iterator it = v.begin();
      //Compare with the pattern
      for(std::size_t i = 0; i != Capacity; ++i, ++it){
         if(*it != static_cast<int>(i))
            return false;
      }
   }
   {
      test::default_init_allocator<int>::reset_pattern(100);
      svector_t v;
      v.resize(Capacity, default_init);
      svector_t::iterator it = v.begin();
      //Compare with the pattern
      for(std::size_t i = 0; i != Capacity; ++i, ++it){
         if(*it != static_cast<int>(i+100))
            return false;
      }
   }
   return true;
}

int main()
{
   recursive_vector_test();
   {
      //Now test move semantics
      stable_vector<recursive_vector> original;
      stable_vector<recursive_vector> move_ctor(boost::move(original));
      stable_vector<recursive_vector> move_assign;
      move_assign = boost::move(move_ctor);
      move_assign.swap(original);
   }

   //Test non-copy-move operations
   {
      stable_vector<test::non_copymovable_int> sv;
      sv.emplace_back();
      sv.resize(10);
      sv.resize(1);
   }
   typedef stable_vector<int> MyVector;
   typedef stable_vector<test::movable_int> MyMoveVector;
   typedef stable_vector<test::movable_and_copyable_int> MyCopyMoveVector;
   typedef stable_vector<test::copyable_int> MyCopyVector;

   if(test::vector_test<MyVector>())
      return 1;

   if(test::vector_test<MyMoveVector>())
      return 1;

   if(test::vector_test<MyCopyMoveVector>())
      return 1;

   if(test::vector_test<MyCopyVector>())
      return 1;

   if(!test::default_init_test< stable_vector<int, test::default_init_allocator<int> > >()){
      std::cerr << "Default init test failed" << std::endl;
      return 1;
   }

   const test::EmplaceOptions Options = (test::EmplaceOptions)(test::EMPLACE_BACK | test::EMPLACE_BEFORE);
   if(!boost::container::test::test_emplace
      < stable_vector<test::EmplaceInt>, Options>())
      return 1;

   if(!boost::container::test::test_propagate_allocator<stable_vector>())
      return 1;

   return 0;
}

#include <boost/container/detail/config_end.hpp>
