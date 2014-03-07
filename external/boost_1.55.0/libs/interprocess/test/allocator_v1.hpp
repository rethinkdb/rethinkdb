///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ALLOCATOR_V1_HPP
#define BOOST_INTERPROCESS_ALLOCATOR_V1_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/intrusive/pointer_traits.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/containers/allocation_type.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/version_type.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <boost/static_assert.hpp>

//!\file
//!Describes an allocator_v1 that allocates portions of fixed size
//!memory buffer (shared memory, mapped file...)

namespace boost {
namespace interprocess {
namespace test {

//!An STL compatible allocator_v1 that uses a segment manager as
//!memory source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator_v1 in shared memory, memory mapped-files, etc...*/
template<class T, class SegmentManager>
class allocator_v1
{
 private:
   typedef allocator_v1<T, SegmentManager>         self_t;
   typedef SegmentManager                          segment_manager;
   typedef typename segment_manager::void_pointer  aux_pointer_t;

   typedef typename boost::intrusive::
      pointer_traits<aux_pointer_t>::template
         rebind_pointer<const void>::type          cvoid_ptr;

   typedef typename boost::intrusive::
      pointer_traits<cvoid_ptr>::template
         rebind_pointer<segment_manager>::type     alloc_ptr_t;

   template<class T2, class SegmentManager2>
   allocator_v1& operator=(const allocator_v1<T2, SegmentManager2>&);

   allocator_v1& operator=(const allocator_v1&);

   alloc_ptr_t mp_mngr;

 public:
   typedef T                                    value_type;

   typedef typename boost::intrusive::
      pointer_traits<cvoid_ptr>::template
         rebind_pointer<T>::type                pointer;

   typedef typename boost::intrusive::
      pointer_traits<cvoid_ptr>::template
         rebind_pointer<const T>::type          const_pointer;

   typedef typename ipcdetail::add_reference
                     <value_type>::type         reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type   const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;

   //!Obtains an allocator_v1 of other type
   template<class T2>
   struct rebind
   {
      typedef allocator_v1<T2, SegmentManager>     other;
   };

   //!Returns the segment manager. Never throws
   segment_manager* get_segment_manager()const
   {  return ipcdetail::to_raw_pointer(mp_mngr);   }
/*
   //!Returns address of mutable object. Never throws
   pointer address(reference value) const
   {  return pointer(addressof(value));  }

   //!Returns address of non mutable object. Never throws
   const_pointer address(const_reference value) const
   {  return const_pointer(addressof(value));  }
*/
   //!Constructor from the segment manager. Never throws
   allocator_v1(segment_manager *segment_mngr)
      : mp_mngr(segment_mngr) { }

   //!Constructor from other allocator_v1. Never throws
   allocator_v1(const allocator_v1 &other)
      : mp_mngr(other.get_segment_manager()){ }

   //!Constructor from related allocator_v1. Never throws
   template<class T2>
   allocator_v1(const allocator_v1<T2, SegmentManager> &other)
      : mp_mngr(other.get_segment_manager()){}

   //!Allocates memory for an array of count elements.
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_ptr hint = 0)
   {
      if(size_overflows<sizeof(T)>(count)){
         throw bad_alloc();
      }
      (void)hint; return pointer(static_cast<T*>(mp_mngr->allocate(count*sizeof(T))));
   }

   //!Deallocates memory previously allocated. Never throws
   void deallocate(const pointer &ptr, size_type)
   {  mp_mngr->deallocate((void*)ipcdetail::to_raw_pointer(ptr));  }

   //!Construct object, calling constructor.
   //!Throws if T(const T&) throws
   void construct(const pointer &ptr, const_reference value)
   {  new((void*)ipcdetail::to_raw_pointer(ptr)) value_type(value);  }

   //!Destroys object. Throws if object's destructor throws
   void destroy(const pointer &ptr)
   {  BOOST_ASSERT(ptr != 0); (*ptr).~value_type();  }

   //!Returns the number of elements that could be allocated. Never throws
   size_type max_size() const
   {  return mp_mngr->get_size();   }

   //!Swap segment manager. Does not throw. If each allocator_v1 is placed in
   //!different memory segments, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  ipcdetail::do_swap(alloc1.mp_mngr, alloc2.mp_mngr);   }
};

//!Equality test for same type of allocator_v1
template<class T, class SegmentManager> inline
bool operator==(const allocator_v1<T , SegmentManager>  &alloc1,
                const allocator_v1<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() == alloc2.get_segment_manager(); }

//!Inequality test for same type of allocator_v1
template<class T, class SegmentManager> inline
bool operator!=(const allocator_v1<T, SegmentManager>  &alloc1,
                const allocator_v1<T, SegmentManager>  &alloc2)
   {  return alloc1.get_segment_manager() != alloc2.get_segment_manager(); }

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_ALLOCATOR_V1_HPP

