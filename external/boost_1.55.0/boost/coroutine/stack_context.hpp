
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_STACK_CONTEXT_H
#define BOOST_COROUTINES_STACK_CONTEXT_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

#if defined(BOOST_USE_SEGMENTED_STACKS)
struct stack_context
{
    typedef void *  segments_context[BOOST_COROUTINES_SEGMENTS];

    std::size_t             size;
    void                *   sp;
    segments_context        segments_ctx;

    stack_context() :
        size( 0), sp( 0), segments_ctx()
    {}
};
#else
struct stack_context
{
    std::size_t             size;
    void                *   sp;

    stack_context() :
        size( 0), sp( 0)
    {}
};
#endif

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_STACK_CONTEXT_H
