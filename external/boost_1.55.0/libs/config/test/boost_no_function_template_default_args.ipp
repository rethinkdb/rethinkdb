//  (C) Copyright Mathias Gaunard 2009. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
//  TITLE:         Default template arguments for function templates
//  DESCRIPTION:   Default template arguments for function templates are not supported.

namespace boost_no_cxx11_function_template_default_args
{

template<typename T = int>
T foo()
{
    return 0;
}

template<typename T, typename U>
bool is_same(T, U)
{
    return false;
}

template<typename T>
bool is_same(T, T)
{
    return true;
}

int test()
{
   return !is_same(foo<>(), 0) || is_same(foo<>(), 0L);
}

} // namespace boost_no_function_template_default_args
