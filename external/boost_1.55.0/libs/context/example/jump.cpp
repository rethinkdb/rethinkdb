
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <boost/assert.hpp>
#include <boost/context/all.hpp>

#include "simple_stack_allocator.hpp"

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

ctx::fcontext_t fcm;
ctx::fcontext_t * fc1 = 0;
ctx::fcontext_t * fc2 = 0;

void f1( intptr_t)
{
        std::cout << "f1: entered" << std::endl;
        std::cout << "f1: call jump_fcontext( fc1, fc2, 0)" << std::endl;
        ctx::jump_fcontext( fc1, fc2, 0);
        std::cout << "f1: return" << std::endl;
        ctx::jump_fcontext( fc1, & fcm, 0);
}

void f2( intptr_t)
{
        std::cout << "f2: entered" << std::endl;
        std::cout << "f2: call jump_fcontext( fc2, fc1, 0)" << std::endl;
        ctx::jump_fcontext( fc2, fc1, 0);
        BOOST_ASSERT( false && ! "f2: never returns");
}

int main( int argc, char * argv[])
{
        stack_allocator alloc;

        void * base1 = alloc.allocate( stack_allocator::default_stacksize());
        BOOST_ASSERT( base1);
        fc1 = ctx::make_fcontext( base1, stack_allocator::default_stacksize(), f1);
        BOOST_ASSERT( fc1);
        BOOST_ASSERT( base1 == fc1->fc_stack.sp);
        BOOST_ASSERT( stack_allocator::default_stacksize() == fc1->fc_stack.size);

        void * base2 = alloc.allocate( stack_allocator::default_stacksize());
        BOOST_ASSERT( base2);
        fc2 = ctx::make_fcontext( base2, stack_allocator::default_stacksize(), f2);
        BOOST_ASSERT( fc2);
        BOOST_ASSERT( base2 == fc2->fc_stack.sp);
        BOOST_ASSERT( stack_allocator::default_stacksize() == fc2->fc_stack.size);

        std::cout << "main: call start_fcontext( & fcm, fc1, 0)" << std::endl;
        ctx::jump_fcontext( & fcm, fc1, 0);

        std::cout << "main: done" << std::endl;

        return EXIT_SUCCESS;
}
