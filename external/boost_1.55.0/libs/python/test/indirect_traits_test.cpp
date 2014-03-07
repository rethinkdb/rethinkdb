// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//#include <stdio.h>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/python/detail/indirect_traits.hpp>
#include <boost/mpl/assert.hpp>

//#define print(expr) printf("%s ==> %s\n", #expr, expr)

// not all the compilers can handle an incomplete class type here.
struct X {};

using namespace boost::python::indirect_traits;

typedef void (X::*pmf)();

BOOST_MPL_ASSERT((is_reference_to_function<int (&)()>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function<int (*)()>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function<int&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function<pmf>));
    
BOOST_MPL_ASSERT_NOT((is_pointer_to_function<int (&)()>));
BOOST_MPL_ASSERT((is_pointer_to_function<int (*)()>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_function<int (*&)()>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_function<int (*const&)()>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_function<pmf>));
    
BOOST_MPL_ASSERT_NOT((is_reference_to_function_pointer<int (&)()>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function_pointer<int (*)()>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function_pointer<int&>));
BOOST_MPL_ASSERT((is_reference_to_function_pointer<int (*&)()>));
BOOST_MPL_ASSERT((is_reference_to_function_pointer<int (*const&)()>));
BOOST_MPL_ASSERT_NOT((is_reference_to_function_pointer<pmf>));

BOOST_MPL_ASSERT((is_reference_to_pointer<int*&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int* const&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int*volatile&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int*const volatile&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int const*&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int const* const&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int const*volatile&>));
BOOST_MPL_ASSERT((is_reference_to_pointer<int const*const volatile&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_pointer<pmf>));

BOOST_MPL_ASSERT_NOT((is_reference_to_pointer<int const volatile>));
BOOST_MPL_ASSERT_NOT((is_reference_to_pointer<int>));
BOOST_MPL_ASSERT_NOT((is_reference_to_pointer<int*>));

BOOST_MPL_ASSERT_NOT((is_reference_to_const<int*&>));
BOOST_MPL_ASSERT((is_reference_to_const<int* const&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_const<int*volatile&>));
BOOST_MPL_ASSERT((is_reference_to_const<int*const volatile&>));
    
BOOST_MPL_ASSERT_NOT((is_reference_to_const<int const volatile>));
BOOST_MPL_ASSERT_NOT((is_reference_to_const<int>));
BOOST_MPL_ASSERT_NOT((is_reference_to_const<int*>));

BOOST_MPL_ASSERT((is_reference_to_non_const<int*&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_non_const<int* const&>));
BOOST_MPL_ASSERT((is_reference_to_non_const<int*volatile&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_non_const<int*const volatile&>));
    
BOOST_MPL_ASSERT_NOT((is_reference_to_non_const<int const volatile>));
BOOST_MPL_ASSERT_NOT((is_reference_to_non_const<int>));
BOOST_MPL_ASSERT_NOT((is_reference_to_non_const<int*>));
    
BOOST_MPL_ASSERT_NOT((is_reference_to_volatile<int*&>));
BOOST_MPL_ASSERT_NOT((is_reference_to_volatile<int* const&>));
BOOST_MPL_ASSERT((is_reference_to_volatile<int*volatile&>));
BOOST_MPL_ASSERT((is_reference_to_volatile<int*const volatile&>));
    
BOOST_MPL_ASSERT_NOT((is_reference_to_volatile<int const volatile>));
BOOST_MPL_ASSERT_NOT((is_reference_to_volatile<int>));
BOOST_MPL_ASSERT_NOT((is_reference_to_volatile<int*>));

namespace tt = boost::python::indirect_traits;

BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<int>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<int&>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<int*>));
    

BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<pmf>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<pmf const&>));
    
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_class<X>));

BOOST_MPL_ASSERT((tt::is_reference_to_class<X&>));
BOOST_MPL_ASSERT((tt::is_reference_to_class<X const&>));
BOOST_MPL_ASSERT((tt::is_reference_to_class<X volatile&>));
BOOST_MPL_ASSERT((tt::is_reference_to_class<X const volatile&>));
    
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<int>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<int*>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<int&>));
    
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<X>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<X&>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<pmf>));
BOOST_MPL_ASSERT_NOT((is_pointer_to_class<pmf const>));
BOOST_MPL_ASSERT((is_pointer_to_class<X*>));
BOOST_MPL_ASSERT((is_pointer_to_class<X const*>));
BOOST_MPL_ASSERT((is_pointer_to_class<X volatile*>));
BOOST_MPL_ASSERT((is_pointer_to_class<X const volatile*>));

BOOST_MPL_ASSERT((tt::is_reference_to_member_function_pointer<pmf&>));
BOOST_MPL_ASSERT((tt::is_reference_to_member_function_pointer<pmf const&>));
BOOST_MPL_ASSERT((tt::is_reference_to_member_function_pointer<pmf volatile&>));
BOOST_MPL_ASSERT((tt::is_reference_to_member_function_pointer<pmf const volatile&>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_member_function_pointer<pmf[2]>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_member_function_pointer<pmf(&)[2]>));
BOOST_MPL_ASSERT_NOT((tt::is_reference_to_member_function_pointer<pmf>));
    
