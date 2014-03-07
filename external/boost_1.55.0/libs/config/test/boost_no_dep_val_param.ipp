//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_DEPENDENT_TYPES_IN_TEMPLATE_VALUE_PARAMETERS
//  TITLE:         dependent non-type template parameters
//  DESCRIPTION:   Template value parameters cannot have a dependent 
//                 type, for example:
//                 template<class T, typename T::type value> 
//                 class X { ... };


namespace boost_no_dependent_types_in_template_value_parameters{

template <class T, typename T::type value = 0>
class X
{};

template <class T>
struct typifier
{
   typedef T type;
};

int test()
{
   X<typifier<int> > x;
   (void) &x;        // avoid "unused variable" warning
   return 0;
}

}


