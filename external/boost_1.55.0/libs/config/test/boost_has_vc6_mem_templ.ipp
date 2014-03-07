//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_MSVC6_MEMBER_TEMPLATES
//  TITLE:         microsoft member templates
//  DESCRIPTION:   Microsoft Visual C++ 6.0 has enough member
//                 template idiosyncrasies (being polite) that
//                 BOOST_NO_MEMBER_TEMPLATES is defined for this compiler.
//                 BOOST_MSVC6_MEMBER_TEMPLATES is defined to allow
//                 compiler specific workarounds.
#ifndef BOOST_NESTED_TEMPLATE
#define BOOST_NESTED_TEMPLATE template
#endif

namespace boost_msvc6_member_templates{

template <class T>
struct foo
{
   template <class U>
   struct nested
   {
      typedef foo<U> other;
   };
   template <class U>
   void mfoo(const U&)
   {
   }
};

template <class T>
void vc6_mem_test(T i)
{
   foo<double> f1;
   typedef foo<T> ifoo;
   f1.mfoo(i);
   typedef typename ifoo::BOOST_NESTED_TEMPLATE nested<double> bound_t;
   typedef typename bound_t::other other;
   other o;
   (void)o;
}

int test()
{
   int i = 0;
   vc6_mem_test(i);
   return 0;
}

}




