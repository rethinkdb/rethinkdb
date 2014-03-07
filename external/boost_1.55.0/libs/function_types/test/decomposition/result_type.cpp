
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/function_types/result_type.hpp>


namespace ft = boost::function_types;

class C; 
typedef C func();
typedef C const c_func();
typedef C (*func_ptr)();
typedef C const (*c_func_ptr)();
typedef C (&func_ref)();
typedef C const (&c_func_ref)();
typedef C (C::*mem_func_ptr)();
typedef C const (C::*c_mem_func_ptr)();
typedef C (C::*mem_func_ptr_c)() const;
typedef C const (C::*c_mem_func_ptr_c)() const;
typedef C (C::*mem_func_ptr_v)() volatile;
typedef C const (C::*c_mem_func_ptr_v)() volatile;
typedef C (C::*mem_func_ptr_cv)() const volatile;
typedef C const (C::*c_mem_func_ptr_cv)() const volatile;
typedef int C::* mem_ptr;
typedef int const C::* c_mem_ptr;

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<func>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_func>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<func_ptr>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_func_ptr>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<func_ref>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_func_ref>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<mem_func_ptr>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_mem_func_ptr>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<mem_func_ptr_c>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_mem_func_ptr_c>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<mem_func_ptr_v>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_mem_func_ptr_v>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<mem_func_ptr_cv>::type,C>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_mem_func_ptr_cv>::type,C const>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<mem_ptr>::type,int&>
));

BOOST_MPL_ASSERT((
  boost::is_same<ft::result_type<c_mem_ptr>::type,int const&>
));

