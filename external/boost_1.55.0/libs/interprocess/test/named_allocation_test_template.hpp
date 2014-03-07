//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER
#define BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/sync/mutex_family.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>
#include <iostream>
#include <cstdio>
#include <new>
#include <utility>
#include <iterator>
#include <set>
#include <string>
#include "get_process_id_name.hpp"

namespace boost { namespace interprocess { namespace test {

namespace {
   const wchar_t *get_prefix(wchar_t)
   {  return L"prefix_name_"; }

   const char *get_prefix(char)
   {  return "prefix_name_"; }
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_names_and_types(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;
   typedef std::char_traits<char_type> char_traits_type;
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;

      char *ptr = m.template construct<char>(name, std::nothrow)(i);

      if(!ptr)
         break;

      std::size_t namelen = char_traits_type::length(m.get_instance_name(ptr));
      if(namelen != char_traits_type::length(name)){
         return 1;
      }

      if(char_traits_type::compare(m.get_instance_name(ptr), name, namelen) != 0){
         return 1;
      }

      if(m.template find<char>(name).first == 0)
         return false;

      if(m.get_instance_type(ptr) != named_type)
         return false;

      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}


//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_named_iterators(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];
   typedef std::basic_string<char_type> string_type;
   std::set<string_type> names;

   basic_bufferstream<char_type> formatter(name, BufferLen);

   string_type aux_str;

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      aux_str = name;
      names.insert(aux_str);
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   typedef typename ManagedMemory::const_named_iterator const_named_iterator;
   const_named_iterator named_beg = m.named_begin();
   const_named_iterator named_end = m.named_end();

   if((std::size_t)std::distance(named_beg, named_end) != (std::size_t)buffers.size()){
      return 1;
   }

   for(; named_beg != named_end; ++named_beg){
      const char_type *name_str = named_beg->name();
      aux_str = name_str;
      if(names.find(aux_str) == names.end()){
         return 1;
      }

      if(aux_str.size() != named_beg->name_length()){
         return 1;
      }

      const void *found_value = m.template find<char>(name_str).first;

      if(found_value == 0)
         return false;
      if(found_value != named_beg->value())
         return false;
   }

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_shrink_to_fit(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   std::size_t free_memory_before = m.get_free_memory();

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;

      char *ptr = m.template construct<char>(name, std::nothrow)(i);

      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   std::size_t free_memory_after = m.get_free_memory();

   if(free_memory_before != free_memory_after){
      m.shrink_to_fit_indexes();
      if(free_memory_before != free_memory_after)
         return false;
   }
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_direct_named_allocation_destruction(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;
   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      if(m.template find<char>(name).first == 0)
         return false;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the inverse order
template<class ManagedMemory>
bool test_named_allocation_inverse_destruction(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;

   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = (int)buffers.size()
      ;j--
      ;){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all following a pattern
template<class ManagedMemory>
bool test_named_allocation_mixed_destruction(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;

   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   for(int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(int j = 0, max = (int)buffers.size()
      ;j < max
      ;++j){
      int pos = (j%4)*((int)buffers.size())/4;
      m.destroy_ptr(buffers[pos]);
      buffers.erase(buffers.begin()+pos);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}

//This test allocates until there is no more memory
//and after that deallocates all in the same order
template<class ManagedMemory>
bool test_inverse_named_allocation_destruction(ManagedMemory &m)
{
   typedef typename ManagedMemory::char_type char_type;

   std::vector<char*> buffers;
   const int BufferLen = 100;
   char_type name[BufferLen];

   basic_bufferstream<char_type> formatter(name, BufferLen);

   for(unsigned int i = 0; true; ++i){
      formatter.seekp(0);
      formatter << get_prefix(char_type()) << i << std::ends;
      char *ptr = m.template construct<char>(name, std::nothrow)(i);
      if(!ptr)
         break;
      buffers.push_back(ptr);
   }

   if(m.get_num_named_objects() != buffers.size() || !m.check_sanity())
      return false;

   for(unsigned int j = 0, max = (unsigned int)buffers.size()
      ;j < max
      ;++j){
      m.destroy_ptr(buffers[j]);
   }

   if(m.get_num_named_objects() != 0 || !m.check_sanity())
      return false;
   m.shrink_to_fit_indexes();
   if(!m.all_memory_deallocated())
      return false;
   return true;
}

///This function calls all tests
template<class ManagedMemory>
bool test_all_named_allocation(ManagedMemory &m)
{
   std::cout << "Starting test_names_and_types. Class: "
             << typeid(m).name() << std::endl;

   if(!test_names_and_types(m)){
      std::cout << "test_names_and_types failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_direct_named_allocation_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_direct_named_allocation_destruction(m)){
      std::cout << "test_direct_named_allocation_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_named_allocation_inverse_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_named_allocation_inverse_destruction(m)){
      std::cout << "test_named_allocation_inverse_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_named_allocation_mixed_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_named_allocation_mixed_destruction(m)){
      std::cout << "test_named_allocation_mixed_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   std::cout << "Starting test_inverse_named_allocation_destruction. Class: "
             << typeid(m).name() << std::endl;

   if(!test_inverse_named_allocation_destruction(m)){
      std::cout << "test_inverse_named_allocation_destruction failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   if(!test_named_iterators(m)){
      std::cout << "test_named_iterators failed. Class: "
                << typeid(m).name() << std::endl;
      return false;
   }

   return true;
}

//This function calls all tests
template<template <class IndexConfig> class Index>
bool test_named_allocation()
{
   using namespace boost::interprocess;

   const int memsize = 163840;
   const char *const shMemName = test::get_process_id_name();
   try
   {
      //A shared memory with rbtree best fit algorithm
      typedef basic_managed_shared_memory
         <char
         ,rbtree_best_fit<mutex_family>
         ,Index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_named_allocation(*segment.get_segment_manager())){
         return false;
      }
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);

   //Now test it with wchar_t
   try
   {
      //A shared memory with simple sequential fit algorithm
      typedef basic_managed_shared_memory
         <wchar_t
         ,rbtree_best_fit<mutex_family>
         ,Index
         > my_managed_shared_memory;

      //Create shared memory
      shared_memory_object::remove(shMemName);
      my_managed_shared_memory segment(create_only, shMemName, memsize);

      //Now take the segment manager and launch memory test
      if(!test::test_all_named_allocation(*segment.get_segment_manager())){
         return false;
      }
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);

   return true;
}

}}}   //namespace boost { namespace interprocess { namespace test {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_ALLOCATION_TEST_TEMPLATE_HEADER

