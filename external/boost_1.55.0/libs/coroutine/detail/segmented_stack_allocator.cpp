
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_COROUTINES_SOURCE

#if defined(BOOST_USE_SEGMENTED_STACKS)

#include <boost/coroutine/detail/segmented_stack_allocator.hpp>

#include <boost/assert.hpp>
#include <boost/context/fcontext.hpp>

#include <boost/coroutine/stack_context.hpp>

extern "C" {
    
void *__splitstack_makecontext( std::size_t,
                                void * [BOOST_COROUTINES_SEGMENTS],
                                std::size_t *);

void __splitstack_releasecontext( void * [BOOST_COROUTINES_SEGMENTS]);

void __splitstack_resetcontext( void * [BOOST_COROUTINES_SEGMENTS]);

void __splitstack_block_signals_context( void * [BOOST_COROUTINES_SEGMENTS],
                                         int * new_value, int * old_value);
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#if !defined (SIGSTKSZ)
# define SIGSTKSZ (8 * 1024)
# define UDEF_SIGSTKSZ
#endif

namespace boost {
namespace coroutines {
namespace detail {

bool
segmented_stack_allocator::is_stack_unbound()
{ return true; }

std::size_t
segmented_stack_allocator::minimum_stacksize()
{ return SIGSTKSZ + sizeof( context::fcontext_t) + 15; }

std::size_t
segmented_stack_allocator::default_stacksize()
{ return minimum_stacksize(); }

std::size_t
segmented_stack_allocator::maximum_stacksize()
{
    BOOST_ASSERT_MSG( false, "segmented stack is unbound");
    return 0;
}

void
segmented_stack_allocator::allocate( stack_context & ctx, std::size_t size)
{
    BOOST_ASSERT( default_stacksize() <= size);

    void * limit = __splitstack_makecontext( size, ctx.segments_ctx, & ctx.size);
    BOOST_ASSERT( limit);
    ctx.sp = static_cast< char * >( limit) + ctx.size;

    int off = 0;
     __splitstack_block_signals_context( ctx.segments_ctx, & off, 0);
}

void
segmented_stack_allocator::deallocate( stack_context & ctx)
{ 
    __splitstack_releasecontext( ctx.segments_ctx);
}

}}}

#ifdef UDEF_SIGSTKSZ
# undef SIGSTKSZ
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif
