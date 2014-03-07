//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP
#define BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/intrusive/pointer_traits.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/allocators/detail/adaptive_node_pool.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/allocators/detail/allocator_common.hpp>
#include <boost/container/detail/multiallocation_chain.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <memory>
#include <algorithm>
#include <cstddef>

//!\file
//!Describes adaptive_pool pooled shared memory STL compatible allocator

namespace boost {
namespace interprocess {

/// @cond

namespace ipcdetail{

template < unsigned int Version
         , class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         , std::size_t MaxFreeBlocks
         , unsigned char OverheadPercent
         >
class adaptive_pool_base
   : public node_pool_allocation_impl
   < adaptive_pool_base
      < Version, T, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>
   , Version
   , T
   , SegmentManager
   >
{
   public:
   typedef typename SegmentManager::void_pointer         void_pointer;
   typedef SegmentManager                                segment_manager;
   typedef adaptive_pool_base
      <Version, T, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>   self_t;

   /// @cond

   template <int dummy>
   struct node_pool
   {
      typedef ipcdetail::shared_adaptive_node_pool
      < SegmentManager, sizeof_value<T>::value, NodesPerBlock, MaxFreeBlocks, OverheadPercent> type;

      static type *get(void *p)
      {  return static_cast<type*>(p);  }
   };
   /// @endcond

   BOOST_STATIC_ASSERT((Version <=2));

   public:
   //-------
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<T>::type                         pointer;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<const T>::type                   const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;

   typedef boost::interprocess::version_type<adaptive_pool_base, Version>   version;
   typedef boost::container::container_detail::transform_multiallocation_chain
      <typename SegmentManager::multiallocation_chain, T>multiallocation_chain;

   //!Obtains adaptive_pool_base from
   //!adaptive_pool_base
   template<class T2>
   struct rebind
   {
      typedef adaptive_pool_base<Version, T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>       other;
   };

   /// @cond
   private:
   //!Not assignable from related adaptive_pool_base
   template<unsigned int Version2, class T2, class SegmentManager2, std::size_t N2, std::size_t F2, unsigned char O2>
   adaptive_pool_base& operator=
      (const adaptive_pool_base<Version2, T2, SegmentManager2, N2, F2, O2>&);

   /// @endcond

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   adaptive_pool_base(segment_manager *segment_mngr)
      : mp_node_pool(ipcdetail::get_or_create_node_pool<typename node_pool<0>::type>(segment_mngr)) { }

   //!Copy constructor from other adaptive_pool_base. Increments the reference
   //!count of the associated node pool. Never throws
   adaptive_pool_base(const adaptive_pool_base &other)
      : mp_node_pool(other.get_node_pool())
   {
      node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool))->inc_ref_count();
   }

   //!Assignment from other adaptive_pool_base
   adaptive_pool_base& operator=(const adaptive_pool_base &other)
   {
      adaptive_pool_base c(other);
      swap(*this, c);
      return *this;
   }

   //!Copy constructor from related adaptive_pool_base. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   adaptive_pool_base
      (const adaptive_pool_base<Version, T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other)
      : mp_node_pool(ipcdetail::get_or_create_node_pool<typename node_pool<0>::type>(other.get_segment_manager())) { }

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~adaptive_pool_base()
   {  ipcdetail::destroy_node_pool_if_last_link(node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool)));   }

   //!Returns a pointer to the node pool.
   //!Never throws
   void* get_node_pool() const
   {  return ipcdetail::to_raw_pointer(mp_node_pool);   }

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const
   {  return node_pool<0>::get(ipcdetail::to_raw_pointer(mp_node_pool))->get_segment_manager();  }

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2)
   {  ipcdetail::do_swap(alloc1.mp_node_pool, alloc2.mp_node_pool);  }

   /// @cond
   private:
   void_pointer   mp_node_pool;
   /// @endcond
};

//!Equality test for same type
//!of adaptive_pool_base
template<unsigned int V, class T, class S, std::size_t NPC, std::size_t F, unsigned char OP> inline
bool operator==(const adaptive_pool_base<V, T, S, NPC, F, OP> &alloc1,
                const adaptive_pool_base<V, T, S, NPC, F, OP> &alloc2)
   {  return alloc1.get_node_pool() == alloc2.get_node_pool(); }

//!Inequality test for same type
//!of adaptive_pool_base
template<unsigned int V, class T, class S, std::size_t NPC, std::size_t F, unsigned char OP> inline
bool operator!=(const adaptive_pool_base<V, T, S, NPC, F, OP> &alloc1,
                const adaptive_pool_base<V, T, S, NPC, F, OP> &alloc2)
   {  return alloc1.get_node_pool() != alloc2.get_node_pool(); }

template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock = 64
         , std::size_t MaxFreeBlocks = 2
         , unsigned char OverheadPercent = 5
         >
class adaptive_pool_v1
   :  public adaptive_pool_base
         < 1
         , T
         , SegmentManager
         , NodesPerBlock
         , MaxFreeBlocks
         , OverheadPercent
         >
{
   public:
   typedef ipcdetail::adaptive_pool_base
         < 1, T, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> base_t;

   template<class T2>
   struct rebind
   {
      typedef adaptive_pool_v1<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>  other;
   };

   adaptive_pool_v1(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   template<class T2>
   adaptive_pool_v1
      (const adaptive_pool_v1<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other)
      : base_t(other)
   {}
};

}  //namespace ipcdetail{

/// @endcond

//!An STL node allocator that uses a segment manager as memory
//!source. The internal pointer type will of the same type (raw, smart) as
//!"typename SegmentManager::void_pointer" type. This allows
//!placing the allocator in shared memory, memory mapped-files, etc...
//!
//!This node allocator shares a segregated storage between all instances
//!of adaptive_pool with equal sizeof(T) placed in the same segment
//!group. NodesPerBlock is the number of nodes allocated at once when the allocator
//!needs runs out of nodes. MaxFreeBlocks is the maximum number of totally free blocks
//!that the adaptive node pool will hold. The rest of the totally free blocks will be
//!deallocated with the segment manager.
//!
//!OverheadPercent is the (approximated) maximum size overhead (1-20%) of the allocator:
//!(memory usable for nodes / total memory allocated from the segment manager)
template < class T
         , class SegmentManager
         , std::size_t NodesPerBlock
         , std::size_t MaxFreeBlocks
         , unsigned char OverheadPercent
         >
class adaptive_pool
   /// @cond
   :  public ipcdetail::adaptive_pool_base
         < 2
         , T
         , SegmentManager
         , NodesPerBlock
         , MaxFreeBlocks
         , OverheadPercent
         >
   /// @endcond
{

   #ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
   typedef ipcdetail::adaptive_pool_base
         < 2, T, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> base_t;
   public:
   typedef boost::interprocess::version_type<adaptive_pool, 2>   version;

   template<class T2>
   struct rebind
   {
      typedef adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent>  other;
   };

   adaptive_pool(SegmentManager *segment_mngr)
      : base_t(segment_mngr)
   {}

   template<class T2>
   adaptive_pool
      (const adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other)
      : base_t(other)
   {}

   #else //BOOST_INTERPROCESS_DOXYGEN_INVOKED
   public:
   typedef implementation_defined::segment_manager       segment_manager;
   typedef segment_manager::void_pointer                 void_pointer;
   typedef implementation_defined::pointer               pointer;
   typedef implementation_defined::const_pointer         const_pointer;
   typedef T                                             value_type;
   typedef typename ipcdetail::add_reference
                     <value_type>::type                  reference;
   typedef typename ipcdetail::add_reference
                     <const value_type>::type            const_reference;
   typedef typename segment_manager::size_type           size_type;
   typedef typename segment_manager::difference_type     difference_type;

   //!Obtains adaptive_pool from
   //!adaptive_pool
   template<class T2>
   struct rebind
   {
      typedef adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> other;
   };

   private:
   //!Not assignable from
   //!related adaptive_pool
   template<class T2, class SegmentManager2, std::size_t N2, std::size_t F2, unsigned char OP2>
   adaptive_pool& operator=
      (const adaptive_pool<T2, SegmentManager2, N2, F2, OP2>&);

   //!Not assignable from
   //!other adaptive_pool
   //adaptive_pool& operator=(const adaptive_pool&);

   public:
   //!Constructor from a segment manager. If not present, constructs a node
   //!pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   adaptive_pool(segment_manager *segment_mngr);

   //!Copy constructor from other adaptive_pool. Increments the reference
   //!count of the associated node pool. Never throws
   adaptive_pool(const adaptive_pool &other);

   //!Copy constructor from related adaptive_pool. If not present, constructs
   //!a node pool. Increments the reference count of the associated node pool.
   //!Can throw boost::interprocess::bad_alloc
   template<class T2>
   adaptive_pool
      (const adaptive_pool<T2, SegmentManager, NodesPerBlock, MaxFreeBlocks, OverheadPercent> &other);

   //!Destructor, removes node_pool_t from memory
   //!if its reference count reaches to zero. Never throws
   ~adaptive_pool();

   //!Returns a pointer to the node pool.
   //!Never throws
   void* get_node_pool() const;

   //!Returns the segment manager.
   //!Never throws
   segment_manager* get_segment_manager()const;

   //!Returns the number of elements that could be allocated.
   //!Never throws
   size_type max_size() const;

   //!Allocate memory for an array of count elements.
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate(size_type count, cvoid_pointer hint = 0);

   //!Deallocate allocated memory.
   //!Never throws
   void deallocate(const pointer &ptr, size_type count);

   //!Deallocates all free blocks
   //!of the pool
   void deallocate_free_blocks();

   //!Swaps allocators. Does not throw. If each allocator is placed in a
   //!different memory segment, the result is undefined.
   friend void swap(self_t &alloc1, self_t &alloc2);

   //!Returns address of mutable object.
   //!Never throws
   pointer address(reference value) const;

   //!Returns address of non mutable object.
   //!Never throws
   const_pointer address(const_reference value) const;
/*
   //!Copy construct an object.
   //!Throws if T's copy constructor throws
   void construct(const pointer &ptr, const_reference v);

   //!Destroys object. Throws if object's
   //!destructor throws
   void destroy(const pointer &ptr);
*/
   //!Returns maximum the number of objects the previously allocated memory
   //!pointed by p can hold. This size only works for memory allocated with
   //!allocate, allocation_command and allocate_many.
   size_type size(const pointer &p) const;

   std::pair<pointer, bool>
      allocation_command(boost::interprocess::allocation_type command,
                         size_type limit_size,
                         size_type preferred_size,
                         size_type &received_size, const pointer &reuse = 0);

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void allocate_many(size_type elem_size, size_type num_elements, multiallocation_chain &chain);

   //!Allocates n_elements elements, each one of size elem_sizes[i]in a
   //!contiguous block
   //!of memory. The elements must be deallocated
   void allocate_many(const size_type *elem_sizes, size_type n_elements, multiallocation_chain &chain);

   //!Allocates many elements of size elem_size in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. The elements must be deallocated
   //!with deallocate(...)
   void deallocate_many(multiallocation_chain &chain);

   //!Allocates just one object. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   //!Throws boost::interprocess::bad_alloc if there is no enough memory
   pointer allocate_one();

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void allocate_individual(size_type num_elements, multiallocation_chain &chain);

   //!Deallocates memory previously allocated with allocate_one().
   //!You should never use deallocate_one to deallocate memory allocated
   //!with other functions different from allocate_one(). Never throws
   void deallocate_one(const pointer &p);

   //!Allocates many elements of size == 1 in a contiguous block
   //!of memory. The minimum number to be allocated is min_elements,
   //!the preferred and maximum number is
   //!preferred_elements. The number of actually allocated elements is
   //!will be assigned to received_size. Memory allocated with this function
   //!must be deallocated only with deallocate_one().
   void deallocate_individual(multiallocation_chain &chain);
   #endif
};

#ifdef BOOST_INTERPROCESS_DOXYGEN_INVOKED

//!Equality test for same type
//!of adaptive_pool
template<class T, class S, std::size_t NodesPerBlock, std::size_t F, unsigned char OP> inline
bool operator==(const adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc1,
                const adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc2);

//!Inequality test for same type
//!of adaptive_pool
template<class T, class S, std::size_t NodesPerBlock, std::size_t F, unsigned char OP> inline
bool operator!=(const adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc1,
                const adaptive_pool<T, S, NodesPerBlock, F, OP> &alloc2);

#endif

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_ADAPTIVE_POOL_HPP

