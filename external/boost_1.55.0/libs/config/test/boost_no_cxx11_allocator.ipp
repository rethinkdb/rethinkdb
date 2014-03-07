//  (C) Copyright John Maddock 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_ALLOCATOR
//  TITLE:         C++11 <memory> doesn't have C++0x allocator support
//  DESCRIPTION:   The compiler does not support the C++11 allocator features added to <memory>

#include <memory>

namespace boost_no_cxx11_allocator {

int test()
{
   std::pointer_traits<char*>* p = 0;
   (void) p;
   //std::pointer_safety s = std::relaxed;

   //char* (*l_undeclare_reachable)(char *p) = std::undeclare_reachable;
   //void (*l_declare_no_pointers)(char *p, size_t n) = std::declare_no_pointers;
   //void (*l_undeclare_no_pointers)(char *p, size_t n) = std::undeclare_no_pointers;
   //std::pointer_safety (*l_get_pointer_safety)() = std::get_pointer_safety;
   //void* (*l_align)(std::size_t alignment, std::size_t size, void *&ptr, std::size_t& space) = std::align;
   std::allocator_arg_t aat;
   std::uses_allocator<int, std::allocator<int> > ua;
   std::allocator_traits<std::allocator<int> > at;

   (void)aat;
   (void)ua;
   (void)at;
   return 0;
}

}
