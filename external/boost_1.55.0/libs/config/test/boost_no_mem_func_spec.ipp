//  (C) Copyright John Maddock 2002. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_MEMBER_FUNCTION_SPECIALIZATIONS
//  TITLE:         Specialisation of individual member functions.
//  DESCRIPTION:   Verify that specializations of individual members
//                 of template classes work OK.


namespace boost_no_member_function_specializations{


template<class T>
class foo
{
public:
   foo();
   foo(const T&);
   ~foo();
   int bar();
};

// declare specialisations:
template<> foo<int>::foo();
template<> foo<int>::foo(const int&);
template<> foo<int>::~foo();
template<> int foo<int>::bar();

// provide defaults:
template<class T> foo<T>::foo(){}
template<class T> foo<T>::foo(const T&){}
template<class T> foo<T>::~foo(){}
template<class T> int foo<T>::bar(){ return 0; }

// provide defs:
template<> foo<int>::foo(){}
template<> foo<int>::foo(const int&){}
template<> foo<int>::~foo(){}
template<> int foo<int>::bar(){ return 1; }


int test()
{
   foo<double> f1;
   foo<int> f2;
   f1.bar();
   f2.bar();
   return 0;
}


}






