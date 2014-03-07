//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_MEMBER_TEMPLATE_KEYWORD
//  TITLE:         member templates keyword
//  DESCRIPTION:   Member template keyword not supported.

namespace boost_no_member_template_keyword{

#ifndef BOOST_NO_MEMBER_TEMPLATES

template <class T>
struct foo
{
   template <class U>
   struct nested
   {
      typedef foo<U> other;
   };
   template <class U>
   void mfoo(const U&);
};

template <class T>
template <class U>
void foo<T>::mfoo(const U&)
{
}

template <class T>
void test_proc(T i)
{
   foo<double> f1;
   typedef foo<T> ifoo;
   f1.mfoo(i);
   f1.template mfoo<T>(i);
   typedef typename ifoo::template nested<double> bound_t;
   typedef typename bound_t::other other;
   other o;
   (void) &o;         // avoid "unused variable" warning
}

#else
template <class T>
void test_proc(T)
{
}
#endif

int test()
{
   test_proc(0);
   return 0;
}


}








