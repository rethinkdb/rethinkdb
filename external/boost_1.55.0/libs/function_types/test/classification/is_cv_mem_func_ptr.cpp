
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/function_types/is_member_function_pointer.hpp>

namespace ft = boost::function_types;

class C; 
typedef void (C::*mem_func_ptr)();
typedef void (C::*c_mem_func_ptr)() const;
typedef void (C::*v_mem_func_ptr)() volatile;
typedef void (C::*cv_mem_func_ptr)() const volatile;


BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_ptr, ft::non_const >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_ptr, ft::non_volatile >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< mem_func_ptr, 
        ft::tag<ft::non_const, ft::non_volatile> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< mem_func_ptr, ft::const_qualified >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< mem_func_ptr, ft::volatile_qualified >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::volatile_qualified> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< mem_func_ptr, 
        ft::tag<ft::non_const, ft::volatile_qualified> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::non_volatile> >
));

//

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< c_mem_func_ptr, ft::const_qualified >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< c_mem_func_ptr, ft::non_volatile >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< c_mem_func_ptr, ft::non_const >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< c_mem_func_ptr, ft::volatile_qualified >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< c_mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::non_volatile> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< c_mem_func_ptr, 
        ft::tag<ft::non_const, ft::volatile_qualified> >
));

//

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< v_mem_func_ptr, ft::volatile_qualified >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< v_mem_func_ptr, ft::non_const >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< v_mem_func_ptr, ft::non_volatile >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< v_mem_func_ptr, ft::const_qualified >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< v_mem_func_ptr, 
        ft::tag<ft::non_const, ft::volatile_qualified> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< v_mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::non_volatile> >
));

//

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< cv_mem_func_ptr, ft::const_qualified >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< cv_mem_func_ptr, ft::volatile_qualified >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< cv_mem_func_ptr, ft::non_const >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< cv_mem_func_ptr, ft::non_volatile >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< cv_mem_func_ptr, 
        ft::tag<ft::non_const, ft::non_volatile> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< cv_mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::non_volatile> >
));

BOOST_MPL_ASSERT_NOT((
  ft::is_member_function_pointer< cv_mem_func_ptr, 
        ft::tag<ft::non_const, ft::volatile_qualified> >
));

BOOST_MPL_ASSERT((
  ft::is_member_function_pointer< cv_mem_func_ptr, 
        ft::tag<ft::const_qualified, ft::volatile_qualified> >
));

