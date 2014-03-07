//  (C) Copyright John Maddock and Dave Abrahams 2002.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_IS_ABSTRACT
//  TITLE:         is_abstract implementation technique
//  DESCRIPTION:   Some compilers can't handle the code used for is_abstract even if they support SFINAE.


namespace boost_no_is_abstract{

#if defined(__CODEGEARC__)
template<class T>
struct is_abstract_test
{
   enum{ value = __is_abstract(T) };
};
#else
template<class T>
struct is_abstract_test
{
   // Deduction fails if T is void, function type,
   // reference type (14.8.2/2)or an abstract class type
   // according to review status issue #337
   //
   template<class U>
   static double check_sig(U (*)[1]);
   template<class U>
   static char check_sig(...);

#ifdef __GNUC__
   enum{ s1 = sizeof(is_abstract_test<T>::template check_sig<T>(0)) };
#else
   enum{ s1 = sizeof(check_sig<T>(0)) };
#endif

   enum{ value = (s1 == sizeof(char)) };
};
#endif

struct non_abstract{};
struct abstract{ virtual void foo() = 0; };

int test()
{
   return static_cast<bool>(is_abstract_test<non_abstract>::value) == static_cast<bool>(is_abstract_test<abstract>::value);
}

}

