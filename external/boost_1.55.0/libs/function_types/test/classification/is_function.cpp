
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/function_types/is_function.hpp>

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
  ft::is_function< func >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< func_ref >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< mem_func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< c_mem_func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< v_mem_func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< cv_mem_func_ptr >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< func_ptr* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< mem_func_ptr* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< C >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int* >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int** >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int& >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int[] >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_function< int[1] >
));

