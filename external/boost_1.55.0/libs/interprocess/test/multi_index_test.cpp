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

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/allocators/adaptive_pool.hpp>
#include <boost/interprocess/allocators/cached_adaptive_pool.hpp>
#include <boost/interprocess/allocators/private_adaptive_pool.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/allocators/cached_node_allocator.hpp>
#include <boost/interprocess/allocators/private_node_allocator.hpp>

#include <boost/interprocess/containers/string.hpp>

//<-
//Shield against external warnings
#include <boost/interprocess/detail/config_external_begin.hpp>
//->

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

//<-
#include <boost/interprocess/detail/config_external_end.hpp>
//->

using namespace boost::interprocess;
namespace bmi = boost::multi_index;

typedef managed_shared_memory::allocator<char>::type                 char_allocator;
typedef basic_string<char, std::char_traits<char>, char_allocator>   shm_string;

//Data to insert in shared memory
struct employee
{
   int         id;
   int         age;
   shm_string  name;

   employee(const employee &e)
      : id(e.id), age(e.age), name(e.name)
   {}

   employee &operator=(const employee &e)
   {
      id = e.id; age = e.age; name = e.name;
      return *this;
   }

   employee( int id_
           , int age_
           , const char *name_
           , const char_allocator &a)
      : id(id_), age(age_), name(name_, a)
   {}
};

//Tags
struct id{};
struct age{};
struct name{};

namespace boost {
namespace multi_index {

// Explicit instantiations to catch compile-time errors
template class bmi::multi_index_container<
  employee,
  bmi::indexed_by<
    bmi::ordered_unique
      <bmi::tag<id>,  BOOST_MULTI_INDEX_MEMBER(employee,int,id)>,
    bmi::ordered_non_unique<
      bmi::tag<name>,BOOST_MULTI_INDEX_MEMBER(employee,shm_string,name)>,
    bmi::ordered_non_unique
      <bmi::tag<age>, BOOST_MULTI_INDEX_MEMBER(employee,int,age)> >,
  allocator<employee,managed_shared_memory::segment_manager>
>;

// Explicit instantiations to catch compile-time errors
template class bmi::multi_index_container<
  employee,
  bmi::indexed_by<
    bmi::ordered_unique
      <bmi::tag<id>,  BOOST_MULTI_INDEX_MEMBER(employee,int,id)>,
    bmi::ordered_non_unique<
      bmi::tag<name>,BOOST_MULTI_INDEX_MEMBER(employee,shm_string,name)>,
    bmi::ordered_non_unique
      <bmi::tag<age>, BOOST_MULTI_INDEX_MEMBER(employee,int,age)> >,
  adaptive_pool<employee,managed_shared_memory::segment_manager>
>;

// Explicit instantiations to catch compile-time errors
template class bmi::multi_index_container<
  employee,
  bmi::indexed_by<
    bmi::ordered_unique
      <bmi::tag<id>,  BOOST_MULTI_INDEX_MEMBER(employee,int,id)>,
    bmi::ordered_non_unique<
      bmi::tag<name>,BOOST_MULTI_INDEX_MEMBER(employee,shm_string,name)>,
    bmi::ordered_non_unique
      <bmi::tag<age>, BOOST_MULTI_INDEX_MEMBER(employee,int,age)> >,
  node_allocator<employee,managed_shared_memory::segment_manager>
>;

}}

int main ()
{
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
