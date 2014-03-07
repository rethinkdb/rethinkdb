
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_COROUTINES_SOURCE

#include "boost/coroutine/detail/coroutine_context.hpp"

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {

void __splitstack_getcontext( void * [BOOST_COROUTINES_SEGMENTS]);

void __splitstack_setcontext( void * [BOOST_COROUTINES_SEGMENTS]);

void __splitstack_releasecontext (void * [BOOST_COROUTINES_SEGMENTS]);

void __splitstack_block_signals_context( void * [BOOST_COROUTINES_SEGMENTS], int *, int *);

}
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

coroutine_context::coroutine_context() :
    fcontext_t(), stack_ctx_( this), ctx_( this)
{
#if defined(BOOST_USE_SEGMENTED_STACKS)
    __splitstack_getcontext( stack_ctx_->segments_ctx);
#endif
}

coroutine_context::coroutine_context( ctx_fn fn, stack_context * stack_ctx) :
    fcontext_t(), stack_ctx_( stack_ctx),
    ctx_( context::make_fcontext( stack_ctx_->sp, stack_ctx_->size, fn) )
{}

coroutine_context::coroutine_context( coroutine_context const& other) :
    fcontext_t(),
    stack_ctx_( other.stack_ctx_),
    ctx_( other.ctx_)
{}

coroutine_context &
coroutine_context::operator=( coroutine_context const& other)
{
    if ( this == & other) return * this;

    stack_ctx_ = other.stack_ctx_;
    ctx_ = other.ctx_;

    return * this;
}

intptr_t
coroutine_context::jump( coroutine_context & other, intptr_t param, bool preserve_fpu)
{
#if defined(BOOST_USE_SEGMENTED_STACKS)
    BOOST_ASSERT( stack_ctx_);
    BOOST_ASSERT( other.stack_ctx_);

    __splitstack_getcontext( stack_ctx_->segments_ctx);
    __splitstack_setcontext( other.stack_ctx_->segments_ctx);
    intptr_t ret = context::jump_fcontext( ctx_, other.ctx_, param, preserve_fpu);

    BOOST_ASSERT( stack_ctx_);
    __splitstack_setcontext( stack_ctx_->segments_ctx);

    return ret;
#else
    return context::jump_fcontext( ctx_, other.ctx_, param, preserve_fpu);
#endif
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
