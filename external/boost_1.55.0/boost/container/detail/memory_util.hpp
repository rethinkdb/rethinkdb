//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_ALLOCATOR_MEMORY_UTIL_HPP
#define BOOST_CONTAINER_ALLOCATOR_MEMORY_UTIL_HPP

#if defined(_MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
#include <boost/container/detail/preprocessor.hpp>
#include <boost/intrusive/detail/has_member_function_callable_with.hpp>


#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME allocate
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 2, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME destroy
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 3, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME max_size
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 0, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME select_on_container_copy_construction
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 0, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME construct
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, BOOST_CONTAINER_MAX_CONSTRUCTOR_PARAMETERS+1, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_FUNCNAME swap
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_BEGIN namespace boost { namespace container { namespace container_detail {
#define BOOST_INTRUSIVE_HAS_MEMBER_FUNCTION_CALLABLE_WITH_NS_END   }}}
#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 1, <boost/intrusive/detail/has_member_function_callable_with.hpp>))
#include BOOST_PP_ITERATE()

namespace boost {
namespace container {
namespace container_detail {


BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(pointer)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(const_pointer)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(reference)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(const_reference)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(void_pointer)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(const_void_pointer)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(size_type)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(propagate_on_container_copy_assignment)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(propagate_on_container_move_assignment)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(propagate_on_container_swap)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(difference_type)

}  //namespace container_detail {
}  //namespace container {
}  //namespace boost {

#include <boost/container/detail/config_end.hpp>

#endif // ! defined(BOOST_CONTAINER_ALLOCATOR_MEMORY_UTIL_HPP)
