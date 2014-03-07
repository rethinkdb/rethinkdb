/* Copyright (C) 2011 John Maddock
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

// Test of bug #1252 (https://svn.boost.org/trac/boost/ticket/1252)

#include <boost/pool/pool.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cstring>

struct limited_allocator_new_delete
{
   typedef std::size_t size_type; 
   typedef std::ptrdiff_t difference_type; 

   static char * malloc BOOST_PREVENT_MACRO_SUBSTITUTION(const size_type bytes)
   { 
      if(bytes > 1510 * 32)
         return 0;
      return new (std::nothrow) char[bytes];
   }
   static void free BOOST_PREVENT_MACRO_SUBSTITUTION(char * const block)
   { 
      delete [] block;
   }
};

template <class T>
void test_alignment(T)
{
   unsigned align = boost::alignment_of<T>::value;
   boost::pool<> p(sizeof(T));
   unsigned limit = 100000;
   for(unsigned i = 0; i < limit; ++i)
   {
      void* ptr = (p.malloc)();
      BOOST_TEST(reinterpret_cast<std::size_t>(ptr) % align == 0);
      // Trample over the memory just to be sure the allocated block is big enough, 
      // if it's not, we'll trample over the next block as well (and our internal housekeeping).
      std::memset(ptr, 0xff, sizeof(T));
   }
}


int main() 
{
   boost::pool<limited_allocator_new_delete> po(1501);
   void* p = (po.malloc)();
   BOOST_TEST(p != 0);

   test_alignment(char(0));
   test_alignment(short(0));
   test_alignment(int(0));
   test_alignment(long(0));
   test_alignment(double(0));
   test_alignment(float(0));
   test_alignment((long double)(0));
#ifndef BOOST_NO_LONG_LONG
   test_alignment((long long)(0));
#endif
   return boost::report_errors();
}
