//  Copyright (C) 2008 N. Musatti
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_NESTED_FRIENDSHIP
//  TITLE:         Access to private members from nested classes
//  DESCRIPTION:   If the compiler fails to support access to private members
//                 from nested classes

namespace boost_no_nested_friendship {

class A {
public:
   A() {}
   struct B {
      int f(A& a) 
      {
         a.f1();
         a.f2(a); 
         return a.b; 
      }
   };

private:
   static int b;
   static void f1(){}
   template <class T>
   static void f2(const T&){}
};

int A::b = 0;

int test()
{
    A a;
    A::B b;
    return b.f(a);
}

}

