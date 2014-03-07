
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_PP_LIMIT_MAG  10

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/context/all.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include "bind_processor.hpp"
#include "cycle.hpp"
#include "simple_stack_allocator.hpp"

#if _POSIX_C_SOURCE >= 199309L
#include "zeit.hpp"
#endif

namespace coro = boost::coroutines;

# define COUNTER BOOST_PP_LIMIT_MAG

# define CALL_COROUTINE(z,n,unused) \
    c();

#ifdef BOOST_COROUTINES_UNIDIRECT
void fn( boost::coroutines::coroutine< void >::push_type & c)
{ while ( true) c(); }

# ifdef BOOST_CONTEXT_CYCLE
cycle_t test_cycles( cycle_t ov, coro::flag_fpu_t preserve_fpu)
{
#  if defined(BOOST_USE_SEGMENTED_STACKS)
    boost::coroutines::coroutine< void >::pull_type c( fn, coro::attributes( preserve_fpu) );
#  else
    coro::simple_stack_allocator< 8 * 1024 * 1024, 64 * 1024, 8 * 1024 > alloc;
    boost::coroutines::coroutine< void >::pull_type c( fn, coro::attributes( preserve_fpu), alloc);
#  endif

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, COUNTER, CALL_COROUTINE, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, COUNTER, CALL_COROUTINE, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= COUNTER; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif

# if _POSIX_C_SOURCE >= 199309L
zeit_t test_zeit( zeit_t ov, coro::flag_fpu_t preserve_fpu)
{
#  if defined(BOOST_USE_SEGMENTED_STACKS)
    boost::coroutines::coroutine< void >::pull_type c( fn, coro::attributes( preserve_fpu) );
#  else
    coro::simple_stack_allocator< 8 * 1024 * 1024, 64 * 1024, 8 * 1024 > alloc;
    boost::coroutines::coroutine< void >::pull_type c( fn, coro::attributes( preserve_fpu), alloc);
#  endif

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_COROUTINE, ~)

    zeit_t start( zeit() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_COROUTINE, ~)
    zeit_t total( zeit() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif
#else
typedef coro::coroutine< void() >   coro_t;

void fn( coro_t::caller_type & c)
{ while ( true) c(); }

# ifdef BOOST_CONTEXT_CYCLE
cycle_t test_cycles( cycle_t ov, coro::flag_fpu_t preserve_fpu)
{
#  if defined(BOOST_USE_SEGMENTED_STACKS)
    coro_t c( fn, coro::attributes( preserve_fpu) );
#  else
    coro::simple_stack_allocator< 8 * 1024 * 1024, 64 * 1024, 8 * 1024 > alloc;
    coro_t c( fn, coro::attributes( preserve_fpu), alloc);
#  endif

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, COUNTER, CALL_COROUTINE, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, COUNTER, CALL_COROUTINE, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= COUNTER; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif

# if _POSIX_C_SOURCE >= 199309L
zeit_t test_zeit( zeit_t ov, coro::flag_fpu_t preserve_fpu)
{
#  if defined(BOOST_USE_SEGMENTED_STACKS)
    coro_t c( fn, coro::attributes( preserve_fpu) );
#  else
    coro::simple_stack_allocator< 8 * 1024 * 1024, 64 * 1024, 8 * 1024 > alloc;
    coro_t c( fn, coro::attributes( preserve_fpu), alloc);
#  endif

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_COROUTINE, ~)

    zeit_t start( zeit() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_COROUTINE, ~)
    zeit_t total( zeit() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif
#endif

int main( int argc, char * argv[])
{
    try
    {
        coro::flag_fpu_t preserve_fpu = coro::fpu_not_preserved;
        bind_to_processor( 0);

#ifdef BOOST_CONTEXT_CYCLE
        {
            cycle_t ov( overhead_cycles() );
            std::cout << "overhead for rdtsc == " << ov << " cycles" << std::endl;

            unsigned int res = test_cycles( ov, preserve_fpu);
            std::cout << "coroutine: average of " << res << " cycles per switch" << std::endl;
        }
#endif

#if _POSIX_C_SOURCE >= 199309L
        {
            zeit_t ov( overhead_zeit() );
            std::cout << "\noverhead for clock_gettime()  == " << ov << " ns" << std::endl;

            unsigned int res = test_zeit( ov, preserve_fpu);
            std::cout << "coroutine: average of " << res << " ns per switch" << std::endl;
        }
#endif

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
