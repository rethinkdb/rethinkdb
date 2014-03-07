
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
#include <boost/function.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/program_options.hpp>

#include "../example/simple_stack_allocator.hpp"

#ifdef BOOST_USE_UCONTEXT
#include <ucontext.h>
#endif

#include "bind_processor.hpp"
#include "cycle.hpp"

#if _POSIX_C_SOURCE >= 199309L
#include "zeit.hpp"
#endif

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

bool pres_fpu = false;

#define CALL_FCONTEXT(z,n,unused) ctx::jump_fcontext( & fcm, fc, 7, pres_fpu);

#ifdef BOOST_USE_UCONTEXT
# define CALL_UCONTEXT(z,n,unused) ::swapcontext( & ucm, & uc);
#endif

#define CALL_FUNCTION(z,n,unused) fn();


ctx::fcontext_t fcm, * fc;

#ifdef BOOST_USE_UCONTEXT
ucontext_t uc, ucm;
#endif


static void f1( intptr_t)
{ while ( true) ctx::jump_fcontext( fc, & fcm, 7, pres_fpu); }

#ifdef BOOST_USE_UCONTEXT
static void f2()
{ while ( true) ::swapcontext( & uc, & ucm); }
#endif

static void f3()
{}


#ifdef BOOST_CONTEXT_CYCLE
cycle_t test_fcontext_cycle( cycle_t ov)
{
    stack_allocator alloc;
    fc = ctx::make_fcontext(
        alloc.allocate(stack_allocator::default_stacksize()),
        stack_allocator::default_stacksize(),
        f1);

    ctx::jump_fcontext( & fcm, fc, 7, pres_fpu);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}

# ifdef BOOST_USE_UCONTEXT
cycle_t test_ucontext_cycle( cycle_t ov)
{
    stack_allocator alloc;

    ::getcontext( & uc);
    uc.uc_stack.ss_sp = alloc.allocate(stack_allocator::default_stacksize());
    uc.uc_stack.ss_size = stack_allocator::default_stacksize();
    ::makecontext( & uc, f2, 7);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif

cycle_t test_function_cycle( cycle_t ov)
{
    boost::function< void() > fn( boost::bind( f3) );
    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FUNCTION, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FUNCTION, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
#endif


#if _POSIX_C_SOURCE >= 199309L
zeit_t test_fcontext_zeit( zeit_t ov)
{
    stack_allocator alloc;
    fc = ctx::make_fcontext(
        alloc.allocate(stack_allocator::default_stacksize()),
        stack_allocator::default_stacksize(),
        f1);

    ctx::jump_fcontext( & fcm, fc, 7, pres_fpu);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)

    zeit_t start( zeit() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)
    zeit_t total( zeit() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}

# ifdef BOOST_USE_UCONTEXT
zeit_t test_ucontext_zeit( zeit_t ov)
{
    stack_allocator alloc;

    ::getcontext( & uc);
    uc.uc_stack.ss_sp = alloc.allocate(stack_allocator::default_stacksize());
    uc.uc_stack.ss_size = stack_allocator::default_stacksize();
    ::makecontext( & uc, f2, 7);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)

    zeit_t start( zeit() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)
    zeit_t total( zeit() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
# endif

zeit_t test_function_zeit( zeit_t ov)
{
    boost::function< void() > fn( boost::bind( f3) );
    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FUNCTION, ~)

    zeit_t start( zeit() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FUNCTION, ~)
    zeit_t total( zeit() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
#endif

int main( int argc, char * argv[])
{
    try
    {
        bind_to_processor( 0);

#ifdef BOOST_CONTEXT_CYCLE
        {
            cycle_t ov( overhead_cycles() );
            std::cout << "overhead for rdtsc == " << ov << " cycles" << std::endl;

            unsigned int res = test_fcontext_cycle( ov);
            std::cout << "fcontext: average of " << res << " cycles per switch" << std::endl;
# ifdef BOOST_USE_UCONTEXT
            res = test_ucontext_cycle( ov);
            std::cout << "ucontext: average of " << res << " cycles per switch" << std::endl;
# endif
            res = test_function_cycle( ov);
            std::cout << "boost::function: average of " << res << " cycles per switch" << std::endl;
        }
#endif

#if _POSIX_C_SOURCE >= 199309L
        {
            zeit_t ov( overhead_zeit() );
            std::cout << "\noverhead for clock_gettime() == " << ov << " ns" << std::endl;

            unsigned int res = test_fcontext_zeit( ov);
            std::cout << "fcontext: average of " << res << " ns per switch" << std::endl;
# ifdef BOOST_USE_UCONTEXT
            res = test_ucontext_zeit( ov);
            std::cout << "ucontext: average of " << res << " ns per switch" << std::endl;
# endif
            res = test_function_zeit( ov);
            std::cout << "boost::function: average of " << res << " ns per switch" << std::endl;
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

#undef CALL_FCONTEXT
#undef CALL_UCONTEXT
