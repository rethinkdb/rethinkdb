
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/function_types/is_function_pointer.hpp>
#include <boost/function_types/is_member_function_pointer.hpp>

namespace ft = boost::function_types;

typedef void(* const func_c_ptr)();
typedef void(* volatile func_v_ptr)();
typedef void(* const volatile func_cv_ptr)();
class C;
typedef void(C::* const mem_func_c_ptr)();
typedef void(C::* volatile mem_func_v_ptr)();
typedef void(C::* const volatile mem_func_cv_ptr)();

// note: the pointer has cv-qualifiers, not the function - non_cv tag must match

BOOST_MPL_ASSERT((
  ft::is_function_pointer< func_c_ptr, ft::non_cv >
));

BOOST_MPL_ASSERT((
  ft::is_function_pointer< func_v_ptr, ft::non_cv >
));

BOOST_MPL_ASSERT((
  ft::is_function_pointer< func_cv_ptr, ft::non_cv >
));


BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_c_ptr, ft::non_cv >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_v_ptr, ft::non_cv >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_cv_ptr, ft::non_cv >
));


