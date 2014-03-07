
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_COROUTINE_CONTEXT_H
#define BOOST_COROUTINES_DETAIL_COROUTINE_CONTEXT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/stack_context.hpp>

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4275)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C"  void *__splitstack_makecontext(
        std::size_t, void * [BOOST_COROUTINES_SEGMENTS], std::size_t *);
#endif

namespace boost {
namespace coroutines {
namespace detail {


class BOOST_COROUTINES_DECL coroutine_context : private context::fcontext_t,
                                                private stack_context
                    
{
private:
    stack_context       *   stack_ctx_;
    context::fcontext_t *   ctx_;

public:
    typedef void( * ctx_fn)( intptr_t);

    coroutine_context();

    explicit coroutine_context( ctx_fn, stack_context *);

    coroutine_context( coroutine_context const&);

    coroutine_context& operator=( coroutine_context const&);

    intptr_t jump( coroutine_context &, intptr_t = 0, bool = true);
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_COROUTINES_DETAIL_COROUTINE_CONTEXT_H
