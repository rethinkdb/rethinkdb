
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/function_types/is_callable_builtin.hpp>

namespace ft = boost::function_types;

typedef void func();
typedef void (*func_ptr)();
typedef void (&func_ref)();
class C; 
typedef void (C::*mem_func_ptr)();
typedef void (C::*c_mem_func_ptr)() const;
typedef void (C::*v_mem_func_ptr)() volatile;
typedef void (C::*cv_mem_func_ptr)() const volatile;

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< func >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< func_ptr >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< func_ref >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< mem_func_ptr >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< c_mem_func_ptr >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< v_mem_func_ptr >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin< cv_mem_func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< func_ptr* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< mem_func_ptr* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< C >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int** >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int& >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int[] >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_callable_builtin< int[1] >
));

