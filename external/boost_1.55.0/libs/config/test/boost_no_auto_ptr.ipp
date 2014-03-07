//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_AUTO_PTR
//  TITLE:         std::auto_ptr
//  DESCRIPTION:   If the compiler / library supplies non-standard or broken
//                 std::auto_ptr.

#include <memory>

namespace boost_no_auto_ptr{

template <class T>
class my_ptr
{
   T* p;
public:
   my_ptr(std::auto_ptr<T>& r)
   {
      p = r.release();
   }
   my_ptr& operator=(std::auto_ptr<T>& r)
   {
      delete p;
      p = r.release();
      return *this;
   }
   ~my_ptr()
   {
      delete p;
   }
};


int test()
{
   std::auto_ptr<int> ap1(new int);
   my_ptr<int> mp(ap1);
   std::auto_ptr<int> ap2(new int);
   mp = ap2;

   return 0;
}

}




