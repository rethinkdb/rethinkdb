
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#define BOOST_FT_AUTODETECT_CALLING_CONVENTIONS

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/function_types/member_function_pointer.hpp>
#include <boost/function_types/is_callable_builtin.hpp>

#ifndef BOOST_FT_CC_STDCALL
#   error "test not supported with this compiler/platform"
#endif

namespace ft = boost::function_types;
namespace mpl = boost::mpl;

typedef ft::stdcall_cc cc;

class C; 

BOOST_MPL_ASSERT((
  ft::is_callable_builtin<
    ft::member_function_pointer<mpl::vector<int, C &>, cc>::type 
  >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin<
     ft::member_function_pointer<mpl::vector<int, C const &>, cc>::type
  >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin<
     ft::member_function_pointer<mpl::vector<int, C volatile &>, cc>::type
  >
));

BOOST_MPL_ASSERT((
  ft::is_callable_builtin<
     ft::member_function_pointer<mpl::vector<int, C const volatile &>, cc>::type
  >
));

