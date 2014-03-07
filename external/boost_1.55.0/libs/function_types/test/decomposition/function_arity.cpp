
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>

#include <boost/function_types/function_arity.hpp>

class C; 
typedef C func();
typedef C (*func_ptr)(int);
typedef C (&func_ref)(int,int);
typedef C (C::*mem_func_ptr)();
typedef C (C::*c_mem_func_ptr)(int) const;
typedef C (C::*v_mem_func_ptr)(int,int) volatile;
typedef C (C::*cv_mem_func_ptr)(int,int,int) const volatile;
typedef int C::* mem_ptr;

BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<func>::value, ==, 0 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<func_ptr>::value, ==, 1 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<func_ref>::value, ==, 2 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<mem_func_ptr>::value, ==, 1 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<c_mem_func_ptr>::value, ==, 2 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<v_mem_func_ptr>::value, ==, 3 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<cv_mem_func_ptr>::value, ==, 4 );
BOOST_MPL_ASSERT_RELATION( ::boost::function_types::function_arity<mem_ptr>::value, ==, 1 );

