///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_EXPAND_BWD_TEST_ALLOCATOR_HPP
#define BOOST_CONTAINER_EXPAND_BWD_TEST_ALLOCATOR_HPP

#if (defined _MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>

#include <boost/container/container_fwd.hpp>
#include <boost/container/throw_exception.hpp>
#include <boost/container/detail/allocation_type.hpp>
#include <boost/assert.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/version_type.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <cassert>

//!\file
//!Describes an allocator to test expand capabilities

namespace boost {
namespace container {
namespace test {

//This allocator just allows two allocations. The first one will return
//mp_buffer + m_offset configured in the constructor. The second one
//will return mp_buffer.
template<class T>
class expand_bwd_test_allocator
{
 private:
   typedef expand_bwd_test_allocator<T> self_t;
   typedef void *                   aux_pointer_t;
   typedef const void *             cvoid_ptr;

   template<class T2>
   expand_bwd_test_allocator& operator=(const expand_bwd_test_allocator<T2>&);

   expand_bwd_test_allocator& operator=(const expand_bwd_test_allocator&);

   public:
   typedef T                                    value_type;
   typedef T *                                  pointer;
   typedef const T *                            const_pointer;
   typedef typename container_detail::add_reference
                     <value_type>::type         reference;
   typedef typename container_detail::add_reference
                     <const value_type>::type   const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

   typedef boost::container::container_detail::version_type<expand_bwd_test_allocator, 2>   version;

   //Dummy multiallocation chain
   struct multiallocation_chain{};

   template<class T2>
   struct rebind
   {  typedef expand_bwd_test_allocator<T2>   other;   };

   //!Constructor from the segment manager. Never throws
   expand_bwd_test_allocator(T *buffer, size_type sz, difference_type offset)
      : mp_buffer(buffer), m_size(sz)
      , m_offset(offset),  m_allocations(0){ }

   //!Constructor from other expand_bwd_test_allocator. Never throws
   expand_bwd_test_allocator(const expand_bwd_test_allocator &other)
      : mp_buffer(other.mp_buffer), m_size(other.m_size)
      , m_offset(other.m_offset),  m_allocations(0){ }

   //!Constructor from related expand_bwd_test_allocator. Never throws
   template<class T2>
   expand_bwd_test_allocator(const expand_bwd_test_allocator<T2> &other)
      : mp_buffer(other.mp_buffer), m_size(other.m_size)
      , m_offset(other.m_offset),  m_allocations(0){ }

   pointer address(reference value)
   {  return pointer(container_detail::addressof(value));  }

   const_pointer address(const_reference value) const
   {  return const_pointer(container_detail::addressof(value));  }

   pointer allocate(size_type , cvoid_ptr hint = 0)
   {  (void)hint; return 0; }

   void deallocate(const pointer &, size_type)
   {}

   template<class Convertible>
   void construct(pointer ptr, const Convertible &value)
   {  new((void*)ptr) value_type(value);  }

   void destroy(pointer ptr)
   {  (*ptr).~value_type();  }

   size_type max_size() const
   {  return m_size;   }

   friend void swap(self_t &alloc1, self_t &alloc2)
   { 
      boost::container::swap_dispatch(alloc1.mp_buffer, alloc2.mp_buffer);
      boost::container::swap_dispatch(alloc1.m_size,    alloc2.m_size);
      boost::container::swap_dispatch(alloc1.m_offset,  alloc2.m_offset);
   }

   //Experimental version 2 expand_bwd_test_allocator functions

   std::pair<pointer, bool>
      allocation_command(boost::container::allocation_type command,
                         size_type limit_size,
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0)
   {
      (void)preferred_size;   (void)reuse;   (void)command;
      //This allocator only expands backwards!
      assert(m_allocations == 0 || (command & boost::container::expand_bwd));
     
      received_size = limit_size;

      if(m_allocations == 0){
         if((m_offset + limit_size) > m_size){
            assert(0);
         }
         ++m_allocations;
         return std::pair<pointer, bool>(mp_buffer + m_offset, false);
      }
      else if(m_allocations == 1){
         if(limit_size > m_size){
            assert(0);
         }
         ++m_allocations;
         return std::pair<pointer, bool>(mp_buffer, true);
      }
      else{
         throw_bad_alloc();
         return std::pair<pointer, bool>(mp_buffer, true);
      }
   }

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold.
   size_type size(const pointer &p) const
   {  (void)p; return m_size; }

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::container::bad_alloc if there is no enough memory
   pointer allocate_one()
   {  return this->allocate(1);  }

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p)
   {  return this->deallocate(p, 1);  }

   pointer           mp_buffer;
   size_type         m_size;
   difference_type   m_offset;
   char              m_allocations;
};

//!Equality test for same type of expand_bwd_test_allocator
template<class T> inline
bool operator==(const expand_bwd_test_allocator<T>  &alloc1,
                const expand_bwd_test_allocator<T>  &alloc2)
{  return false; }

//!Inequality test for same type of expand_bwd_test_allocator
template<class T> inline
bool operator!=(const expand_bwd_test_allocator<T>  &alloc1,
                const expand_bwd_test_allocator<T>  &alloc2)
{  return true; }

}  //namespace test {
}  //namespace container {
}  //namespace boost {

#include <boost/container/detail/config_end.hpp>

#endif   //BOOST_CONTAINER_EXPAND_BWD_TEST_ALLOCATOR_HPP

