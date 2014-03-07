
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/function_types/function_type.hpp>
#include <boost/function_types/function_pointer.hpp>
#include <boost/function_types/function_reference.hpp>
#include <boost/function_types/member_function_pointer.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;
using boost::is_same;

class C; 

typedef C func1(C &);
typedef C (*func_ptr1)(C &);
typedef C (&func_ref1)(C &);
typedef C (C::*mem_func_ptr1)();

BOOST_MPL_ASSERT(( is_same< func1, ft::function_type<func1>::type > ));
BOOST_MPL_ASSERT(( is_same< func1, ft::function_type<func_ptr1>::type > ));
BOOST_MPL_ASSERT(( is_same< func1, ft::function_type<func_ref1>::type > ));
BOOST_MPL_ASSERT(( is_same< func1, ft::function_type<mem_func_ptr1>::type > ));

BOOST_MPL_ASSERT(( is_same< func_ptr1, ft::function_pointer<func1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr1, ft::function_pointer<func_ptr1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr1, ft::function_pointer<func_ref1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr1, ft::function_pointer<mem_func_ptr1>::type > ));

BOOST_MPL_ASSERT(( is_same< func_ref1, ft::function_reference<func1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref1, ft::function_reference<func_ptr1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref1, ft::function_reference<func_ref1>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref1, ft::function_reference<mem_func_ptr1>::type > ));

BOOST_MPL_ASSERT(( is_same< mem_func_ptr1, ft::member_function_pointer<func1>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr1, ft::member_function_pointer<func_ptr1>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr1, ft::member_function_pointer<func_ref1>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr1, ft::member_function_pointer<mem_func_ptr1>::type > ));


typedef C func2(C const &);
typedef C (*func_ptr2)(C const &);
typedef C (&func_ref2)(C const &);
typedef C (C::*mem_func_ptr2)() const;

BOOST_MPL_ASSERT(( is_same< func2, ft::function_type<func2>::type > ));
BOOST_MPL_ASSERT(( is_same< func2, ft::function_type<func_ptr2>::type > ));
BOOST_MPL_ASSERT(( is_same< func2, ft::function_type<func_ref2>::type > ));
BOOST_MPL_ASSERT(( is_same< func2, ft::function_type<mem_func_ptr2>::type > ));

BOOST_MPL_ASSERT(( is_same< func_ptr2, ft::function_pointer<func2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr2, ft::function_pointer<func_ptr2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr2, ft::function_pointer<func_ref2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ptr2, ft::function_pointer<mem_func_ptr2>::type > ));

BOOST_MPL_ASSERT(( is_same< func_ref2, ft::function_reference<func2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref2, ft::function_reference<func_ptr2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref2, ft::function_reference<func_ref2>::type > ));
BOOST_MPL_ASSERT(( is_same< func_ref2, ft::function_reference<mem_func_ptr2>::type > ));

BOOST_MPL_ASSERT(( is_same< mem_func_ptr2, ft::member_function_pointer<func2>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr2, ft::member_function_pointer<func_ptr2>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr2, ft::member_function_pointer<func_ref2>::type > ));
BOOST_MPL_ASSERT(( is_same< mem_func_ptr2, ft::member_function_pointer<mem_func_ptr2>::type > ));



