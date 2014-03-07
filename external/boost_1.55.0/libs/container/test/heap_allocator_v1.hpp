///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_HEAP_ALLOCATOR_V1_HPP
#define BOOST_CONTAINER_HEAP_ALLOCATOR_V1_HPP

#if (defined _MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>

#include <boost/intrusive/pointer_traits.hpp>

#include <boost/container/container_fwd.hpp>
#include <boost/container/detail/allocation_type.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/version_type.hpp>
#include <boost/container/exceptions.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>

//!\file
//!Describes an heap_allocator_v1 that allocates portions of fixed size
//!memory buffer (shared memory, mapped file...)

namespace boost {
namespace container {
namespace test {

//!An STL compatible heap_allocator_v1 that uses a segment manager as
//!memory source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the heap_allocator_v1 in shared memory, memory mapped-files, etc...*/
template<class T, class SegmentManager>
class heap_allocator_v1
{
 private:
   typedef heap_allocator_v1<T, SegmentManager>         self_t;
   typedef SegmentManager                          segment_manager;
   typedef typename segment_manager::void_pointer  aux_pointer_t;

   typedef typename
      boost::pointer_to_other
         <aux_pointer_t, const void>::type   cvoid_ptr;

   typedef typename boost::pointer_to_other
      <cvoid_ptr, segment_manager>::type     alloc_ptr_t;

   template<class T2, class SegmentManager2>
   heap_allocator_v1& operator=(const heap_allocator_v1<T2, SegmentManager2>&);

   heap_allocator_v1& operator=(const heap_allocator_v1&);

   alloc_ptr_t mp_mngr;

 public:
   typedef T                                    value_type;
   typedef typename boost::pointer_to_other
      <cvoid_ptr, T>::type                      pointer;
   typedef typename boost::
      pointer_to_other<pointer, const T>::type  const_pointer;
   typedef typename detail::add_reference
                     <value_type>::type         reference;
   typedef typename detail::add_reference
                     <const value_type>::type   const_reference;
   typedef std::size_t                          size_type;
   typedef std::ptrdiff_t                       difference_type;

   //!Obtains an heap_allocator_v1 of other type
   template<class T2>
   struct rebind
   {  
      typedef heap_allocator_v1<T2, SegmentManager>     other;
   };

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
   {  return detail::get_pointer(mp_mngr);   }
/*
   //!Returns address of mutable object. Never throws
   pointer address(reference value) const
   {  return pointer(addressof(value));  }

   //!Returns address of non mutable object. Never throws
   const_pointer address(const_reference value) const
   {  return const_pointer(addressof(value));  }
*/
   //!Constructor from the segment manager. Never throws
   heap_allocator_v1(segment_manager *segment_mngr)
      : mp_mngr(segment_mngr) { }

   //!Constructor from other heap_allocator_v1. Never throws
   heap_allocator_v1(const heap_allocator_v1 &other)
      : mp_mngr(other.get_segment_manager()){ }

   //!Constructor from related heap_allocator_v1. Never throws
   template<class T2>
   heap_allocator_v1(const heap_allocator_v1<T2, SegmentManager> &other)
      : mp_mngr(other.get_segment_manager()){}

   //!Allocates memory for an array of count elements.
   //!Throws boost::container::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_ptr hint = 0)
   {  (void)hint; return ::new value_type[count];  }

   //!Deallocates memory previously allocated. Never throws
   void deallocate(const pointer &ptr, size_type)
   {  return ::delete[] detail::get_pointer(ptr) ;  }

   //!Construct object, calling constructor.
   //!Throws if T(const T&) throws
   void construct(const pointer &ptr, const_reference value)
   {  new((void*)detail::get_pointer(ptr)) value_type(value);  }

   //!Destroys object. Throws if object's destructor throws
   void destroy(const pointer &ptr)
   {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }

   //!Returns the number of elements that could be allocated. Never throws
   size_type max_size() const
   {  return mp_mngr->get_size();   }

   //!Swap segment manager. Does not throw. If each heap_allocator_v1 is placed in
   //!different memory segments, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  boost::container::boost::container::swap_dispatch(alloc1.mp_mngr, alloc2.mp_mngr);   }
};

//!Equality test for same type of heap_allocator_v1
template<class T, class SegmentManager> inline
bool operator==(const heap_allocator_v1<T , SegmentManager>  &alloc1,
                const heap_allocator_v1<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

//!Inequality test for same type of heap_allocator_v1
template<class T, class SegmentManager> inline
bool operator!=(const heap_allocator_v1<T, SegmentManager>  &alloc1,
                const heap_allocator_v1<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace test {
}  //namespace container {
}  //namespace boost {

#include <boost/container/detail/config_end.hpp>

#endif   //BOOST_CONTAINER_HEAP_ALLOCATOR_V1_HPP

