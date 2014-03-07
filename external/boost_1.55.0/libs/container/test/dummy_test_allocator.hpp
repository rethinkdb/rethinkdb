///////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_DUMMY_TEST_ALLOCATOR_HPP
#define BOOST_CONTAINER_DUMMY_TEST_ALLOCATOR_HPP

#if (defined _MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>

#include <boost/container/container_fwd.hpp>
#include <boost/container/detail/allocation_type.hpp>
#include <boost/assert.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/type_traits.hpp>
#include <boost/container/detail/mpl.hpp>
#include <boost/container/detail/version_type.hpp>
#include <boost/container/detail/multiallocation_chain.hpp>
#include <boost/container/throw_exception.hpp>
#include <boost/move/utility.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>
#include <cassert>

//!\file
//!Describes an allocator to test expand capabilities

namespace boost {
namespace container {
namespace test {

//Very simple version 1 allocator
template<class T>
class simple_allocator
{
   public:
   typedef T value_type;

   simple_allocator()
   {}

   template<class U>
   simple_allocator(const simple_allocator<U> &)
   {}

   T* allocate(std::size_t n)
   { return (T*)::new char[sizeof(T)*n];  }

   void deallocate(T*p, std::size_t)
   { delete[] ((char*)p);}

   friend bool operator==(const simple_allocator &, const simple_allocator &)
   {  return true;  }

   friend bool operator!=(const simple_allocator &, const simple_allocator &)
   {  return false;  }
};

//Version 2 allocator with rebind
template<class T>
class dummy_test_allocator
{
 private:
   typedef dummy_test_allocator<T>  self_t;
   typedef void *                   aux_pointer_t;
   typedef const void *             cvoid_ptr;

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

   typedef container_detail::basic_multiallocation_chain
      <void*>                                            multialloc_cached_counted;
   typedef boost::container::container_detail::transform_multiallocation_chain
      <multialloc_cached_counted, value_type>            multiallocation_chain;

   typedef boost::container::container_detail::version_type<dummy_test_allocator, 2>   version;

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
   {  return pointer(container_detail::addressof(value));  }

   const_pointer address(const_reference value) const
   {  return const_pointer(container_detail::addressof(value));  }

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
      allocation_command(boost::container::allocation_type,
                         size_type,
                         size_type,
                         size_type &, const pointer & = 0)
   {  return std::pair<pointer, bool>(pointer(), true); }

   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold.
   size_type size(const pointer &) const
   {  return 0; }

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::container::bad_alloc if there is no enough memory
   pointer allocate_one()
   {  return pointer();  }

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &)
   {}

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void allocate_individual(size_type, multiallocation_chain &)
   {}

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void deallocate_individual(multiallocation_chain &)
   {}

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void deallocate_many(multiallocation_chain &)
   {}
};

//!Equality test for same type of dummy_test_allocator
template<class T> inline
bool operator==(const dummy_test_allocator<T>  &,
                const dummy_test_allocator<T>  &)
{  return true; }

//!Inequality test for same type of dummy_test_allocator
template<class T> inline
bool operator!=(const dummy_test_allocator<T>  &,
                const dummy_test_allocator<T>  &)
{  return false; }


template< class T
        , bool PropagateOnContCopyAssign
        , bool PropagateOnContMoveAssign
        , bool PropagateOnContSwap
        , bool CopyOnPropagateOnContSwap
        >
class propagation_test_allocator
{
   BOOST_COPYABLE_AND_MOVABLE(propagation_test_allocator)

   public:
   typedef T value_type;
   typedef boost::container::container_detail::bool_<PropagateOnContCopyAssign>
      propagate_on_container_copy_assignment;
   typedef boost::container::container_detail::bool_<PropagateOnContMoveAssign>
      propagate_on_container_move_assignment;
   typedef boost::container::container_detail::bool_<PropagateOnContSwap>
      propagate_on_container_swap;

   template<class T2>
   struct rebind
   {  typedef propagation_test_allocator
         < T2
         , PropagateOnContCopyAssign
         , PropagateOnContMoveAssign
         , PropagateOnContSwap
         , CopyOnPropagateOnContSwap>   other;
   };

   propagation_test_allocator select_on_container_copy_construction() const
   {  return CopyOnPropagateOnContSwap ? propagation_test_allocator(*this) : propagation_test_allocator();  }

   explicit propagation_test_allocator()
      : id_(unique_id_++)
      , ctr_copies_(0)
      , ctr_moves_(0)
      , assign_copies_(0)
      , assign_moves_(0)
      , swaps_(0)
   {}

   propagation_test_allocator(const propagation_test_allocator &x)
      : id_(x.id_)
      , ctr_copies_(x.ctr_copies_+1)
      , ctr_moves_(x.ctr_moves_)
      , assign_copies_(x.assign_copies_)
      , assign_moves_(x.assign_moves_)
      , swaps_(x.swaps_)
   {}

   template<class U>
   propagation_test_allocator(const propagation_test_allocator
                                       < U
                                       , PropagateOnContCopyAssign
                                       , PropagateOnContMoveAssign
                                       , PropagateOnContSwap
                                       , CopyOnPropagateOnContSwap> &x)
      : id_(x.id_)
      , ctr_copies_(0)
      , ctr_moves_(0)
      , assign_copies_(0)
      , assign_moves_(0)
      , swaps_(0)
   {}

   propagation_test_allocator(BOOST_RV_REF(propagation_test_allocator) x)
      : id_(x.id_)
      , ctr_copies_(x.ctr_copies_)
      , ctr_moves_(x.ctr_moves_ + 1)
      , assign_copies_(x.assign_copies_)
      , assign_moves_(x.assign_moves_)
      , swaps_(x.swaps_)
   {}

   propagation_test_allocator &operator=(BOOST_COPY_ASSIGN_REF(propagation_test_allocator) x)
   {
      id_ = x.id_;
      ctr_copies_ = x.ctr_copies_;
      ctr_moves_ = x.ctr_moves_;
      assign_copies_ = x.assign_copies_+1;
      assign_moves_ = x.assign_moves_;
      swaps_ = x.swaps_;
      return *this;
   }

   propagation_test_allocator &operator=(BOOST_RV_REF(propagation_test_allocator) x)
   {
      id_ = x.id_;
      ctr_copies_ = x.ctr_copies_;
      ctr_moves_ = x.ctr_moves_;
      assign_copies_ = x.assign_copies_;
      assign_moves_ = x.assign_moves_+1;
      swaps_ = x.swaps_;
      return *this;
   }

   static void reset_unique_id()
   {  unique_id_ = 0;  }

   T* allocate(std::size_t n)
   {  return (T*)::new char[sizeof(T)*n];  }

   void deallocate(T*p, std::size_t)
   { delete[] ((char*)p);}

   friend bool operator==(const propagation_test_allocator &, const propagation_test_allocator &)
   {  return true;  }

   friend bool operator!=(const propagation_test_allocator &, const propagation_test_allocator &)
   {  return false;  }

   void swap(propagation_test_allocator &r)
   {
      ++this->swaps_; ++r.swaps_;
      boost::container::swap_dispatch(this->id_, r.id_);
      boost::container::swap_dispatch(this->ctr_copies_, r.ctr_copies_);
      boost::container::swap_dispatch(this->ctr_moves_, r.ctr_moves_);
      boost::container::swap_dispatch(this->assign_copies_, r.assign_copies_);
      boost::container::swap_dispatch(this->assign_moves_, r.assign_moves_);
      boost::container::swap_dispatch(this->swaps_, r.swaps_);
   }

   friend void swap(propagation_test_allocator &l, propagation_test_allocator &r)
   {
      l.swap(r);
   }

   unsigned int id_;
   unsigned int ctr_copies_;
   unsigned int ctr_moves_;
   unsigned int assign_copies_;
   unsigned int assign_moves_;
   unsigned int swaps_;
   static unsigned unique_id_;
};

template< class T
        , bool PropagateOnContCopyAssign
        , bool PropagateOnContMoveAssign
        , bool PropagateOnContSwap
        , bool CopyOnPropagateOnContSwap
        >
unsigned int propagation_test_allocator< T
                                       , PropagateOnContCopyAssign
                                       , PropagateOnContMoveAssign
                                       , PropagateOnContSwap
                                       , CopyOnPropagateOnContSwap>::unique_id_ = 0;


}  //namespace test {
}  //namespace container {
}  //namespace boost {

#include <boost/container/detail/config_end.hpp>

#endif   //BOOST_CONTAINER_DUMMY_TEST_ALLOCATOR_HPP

