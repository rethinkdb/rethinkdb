//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_XSI_SHARED_MEMORY_HPP
#define BOOST_INTERPROCESS_MANAGED_XSI_SHARED_MEMORY_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if !defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)
#error "This header can't be used in operating systems without XSI (System V) shared memory support"
#endif

#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/xsi_shared_memory_file_wrapper.hpp>
#include <boost/interprocess/creation_tags.hpp>
//These includes needed to fulfill default template parameters of
//predeclarations in interprocess_fwd.hpp
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>

namespace boost {

namespace interprocess {

namespace ipcdetail {

template<class AllocationAlgorithm>
struct xsishmem_open_or_create
{
   typedef  ipcdetail::managed_open_or_create_impl                 //!FileBased, StoreDevice
      < xsi_shared_memory_file_wrapper, AllocationAlgorithm::Alignment, false, true> type;
};

}  //namespace ipcdetail {

//!A basic X/Open System Interface (XSI) shared memory named object creation class. Initializes the
//!shared memory segment. Inherits all basic functionality from
//!basic_managed_memory_impl<CharType, AllocationAlgorithm, IndexType>
template
      <
         class CharType,
         class AllocationAlgorithm,
         template<class IndexConfig> class IndexType
      >
class basic_managed_xsi_shared_memory
   : public ipcdetail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType
      ,ipcdetail::xsishmem_open_or_create<AllocationAlgorithm>::type::ManagedOpenOrCreateUserOffset>
   , private ipcdetail::xsishmem_open_or_create<AllocationAlgorithm>::type
{
   /// @cond
   public:
   typedef xsi_shared_memory_file_wrapper device_type;

   public:
   typedef typename ipcdetail::xsishmem_open_or_create<AllocationAlgorithm>::type base2_t;
   typedef ipcdetail::basic_managed_memory_impl
      <CharType, AllocationAlgorithm, IndexType,
      base2_t::ManagedOpenOrCreateUserOffset>   base_t;

   typedef ipcdetail::create_open_func<base_t>        create_open_func_t;

   basic_managed_xsi_shared_memory *get_this_pointer()
   {  return this;   }

   private:
   typedef typename base_t::char_ptr_holder_t   char_ptr_holder_t;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_managed_xsi_shared_memory)
   /// @endcond

   public: //functions
   typedef typename base_t::size_type              size_type;

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~basic_managed_xsi_shared_memory()
   {}

   //!Default constructor. Does nothing.
   //!Useful in combination with move semantics
   basic_managed_xsi_shared_memory()
   {}

   //!Creates shared memory and creates and places the segment manager.
   //!This can throw.
   basic_managed_xsi_shared_memory(create_only_t, const xsi_key &key,
                             std::size_t size, const void *addr = 0, const permissions& perm = permissions())
      : base_t()
      , base2_t(create_only, key, size, read_write, addr,
                create_open_func_t(get_this_pointer(), ipcdetail::DoCreate), perm)
   {}

   //!Creates shared memory and creates and places the segment manager if
   //!segment was not created. If segment was created it connects to the
   //!segment.
   //!This can throw.
   basic_managed_xsi_shared_memory (open_or_create_t,
                              const xsi_key &key, std::size_t size,
                              const void *addr = 0, const permissions& perm = permissions())
      : base_t()
      , base2_t(open_or_create, key, size, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpenOrCreate), perm)
   {}

   //!Connects to a created shared memory and its segment manager.
   //!in read-only mode.
   //!This can throw.
   basic_managed_xsi_shared_memory (open_read_only_t, const xsi_key &key,
                                const void *addr = 0)
      : base_t()
      , base2_t(open_only, key, read_only, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Connects to a created shared memory and its segment manager.
   //!This can throw.
   basic_managed_xsi_shared_memory (open_only_t, const xsi_key &key,
                                const void *addr = 0)
      : base_t()
      , base2_t(open_only, key, read_write, addr,
                create_open_func_t(get_this_pointer(),
                ipcdetail::DoOpen))
   {}

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_xsi_shared_memory(BOOST_RV_REF(basic_managed_xsi_shared_memory) moved)
   {
      basic_managed_xsi_shared_memory tmp;
      this->swap(moved);
      tmp.swap(moved);
   }

   //!Moves the ownership of "moved"'s managed memory to *this.
   //!Does not throw
   basic_managed_xsi_shared_memory &operator=(BOOST_RV_REF(basic_managed_xsi_shared_memory) moved)
   {
      basic_managed_xsi_shared_memory tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps the ownership of the managed shared memories managed by *this and other.
   //!Never throws.
   void swap(basic_managed_xsi_shared_memory &other)
   {
      base_t::swap(other);
      base2_t::swap(other);
   }

   //!Erases a XSI shared memory object identified by shmid
   //!from the system.
   //!Returns false on error. Never throws
   static bool remove(int shmid)
   {  return device_type::remove(shmid); }

   int get_shmid() const
   {  return base2_t::get_device().get_shmid(); }

   /// @cond

   //!Tries to find a previous named allocation address. Returns a memory
   //!buffer and the object count. If not found returned pointer is 0.
   //!Never throws.
   template <class T>
   std::pair<T*, std::size_t> find  (char_ptr_holder_t name)
   {
      if(base2_t::get_mapped_region().get_mode() == read_only){
         return base_t::template find_no_lock<T>(name);
      }
      else{
         return base_t::template find<T>(name);
      }
   }

   /// @endcond
};

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_MANAGED_XSI_SHARED_MEMORY_HPP

