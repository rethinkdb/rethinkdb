///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DUMMY_TEST_ALLOCATOR_HPP
#define BOOST_INTERPROCESS_DUMMY_TEST_ALLOCATOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/containers/allocation_type.hpp>
#include <boost/assert.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <cassert>

//!\file
//!Describes an allocator to test expand capabilities

namespace boost {
namespace interprocess {
namespace test {

//This allocator just allows two allocations. The first one will return
//mp_buffer + m_offset configured in the constructor. The second one
//will return mp_buffer.
template<class T>
class dummy_test_allocator
{
 private:
   typedef dummy_test_allocator<T> self_t;
   typedef void *                   aux_pointer_t;
   typedef const void *             cvoid_ptr;

   template<class T2>
   dummy_test_allocator& operator=(const dummy_test_allocator<T2>&);

   dummy_test_allocator& operator=(const dummy_test_allocator&);

   public:
   typedef T                                    value_type;
   typedef T *                                  pointer;
   typedef const T *                            const_pointer;
   typedef typename ipcdetail::add_reference
                     <value_type>::type         reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type   const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

//   typedef boost::interprocess::version_type<dummy_test_allocator, 2>   version;

   template<class T2>
   struct rebind
   {  typedef dummy_test_allocator<T2>   other;   };

   //!Default constructor. Never throws
   dummy_test_allocator()
   {}

   //!Constructor from other dummy_test_allocator. Never throws
   dummy_test_allocator(const dummy_test_allocator &)
   {}

   //!Constructor from related dummy_test_allocator. Never throws
   template<class T2>
   dummy_test_allocator(const dummy_test_allocator<T2> &)
   {}

   pointer address(reference value)
   {  return pointer(addressof(value));  }

   const_pointer address(const_reference value) const
   {  return const_pointer(addressof(value));  }

   pointer allocate(size_type, cvoid_ptr = 0)
   {  return 0; }

   void deallocate(const pointer &, size_type)
   { }

   template<class Convertible>
   void construct(pointer, const Convertible &)
   {}

   void destroy(pointer)
   {}

   size_type max_size() const
   {  return 0;   }

   friend void swap(self_t &, self_t &)
   {}

   //Experimental version 2 dummy_test_allocator functions

   std::pair<pointer, bool>
      allocation_command(boost::interprocess::allocation_type,
                         size_type,
                         size_type,
                         size_type &, const pointer & = 0)
   {  return std::pair<pointer, bool>(pointer(0), true); }

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold.
   size_type size(const pointer &) const
   {  return 0; }

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one()
   {  return pointer(0);  }

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &)
   {}
};

//!Equality test for same type of dummy_test_allocator
template<class T> inline
bool operator==(const dummy_test_allocator<T>  &,
                const dummy_test_allocator<T>  &)
{  return false; }

//!Inequality test for same type of dummy_test_allocator
template<class T> inline
bool operator!=(const dummy_test_allocator<T>  &,
                const dummy_test_allocator<T>  &)
{  return true; }

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DUMMY_TEST_ALLOCATOR_HPP

