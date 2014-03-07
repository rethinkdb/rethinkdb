
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/function_types/function_type.hpp>
#include <boost/function_types/function_pointer.hpp>
#include <boost/function_types/function_reference.hpp>
#include <boost/function_types/member_function_pointer.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;
using boost::is_same;

class C;
typedef int expected_v_1(...);
typedef int expected_nv_1();
typedef int (C::*expected_v_2)(...);
typedef int (C::*expected_nv_2)();

BOOST_MPL_ASSERT(( is_same< 
    ft::function_type<mpl::vector<int>, ft::variadic>::type, expected_v_1 
> ));

BOOST_MPL_ASSERT(( is_same< 
    ft::function_type<mpl::vector<int>, ft::non_variadic>::type, expected_nv_1 
> ));

BOOST_MPL_ASSERT(( is_same< 
    ft::function_pointer<mpl::vector<int>, ft::variadic>::type, expected_v_1 * 
> ));

BOOST_MPL_ASSERT(( is_same< 
    ft::function_pointer<mpl::vector<int>, ft::non_variadic>::type
  , expected_nv_1 * 
> ));

BOOST_MPL_ASSERT(( is_same<
    ft::function_reference<mpl::vector<int>, ft::variadic>::type, expected_v_1 &
> ));

BOOST_MPL_ASSERT(( is_same<
    ft::function_reference<mpl::vector<int>, ft::non_variadic>::type
  , expected_nv_1 &
> ));

BOOST_MPL_ASSERT(( is_same< 
    ft::member_function_pointer<mpl::vector<int,C>, ft::variadic>::type
  , expected_v_2 
> ));

BOOST_MPL_ASSERT(( is_same< 
    ft::member_function_pointer<mpl::vector<int,C>, ft::non_variadic>::type
  , expected_nv_2 
> ));

