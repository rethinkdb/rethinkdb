
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/coroutine/all.hpp>

#ifdef BOOST_COROUTINES_UNIDIRECT
typedef boost::coroutines::coroutine< void >::pull_type pull_coro_t;
typedef boost::coroutines::coroutine< void >::push_type push_coro_t;

void echo( pull_coro_t & source, int i)
{
    std::cout << i;
    source();
}

void runit( push_coro_t & sink1)
{
    std::cout << "started! ";
    for ( int i = 0; i < 10; ++i)
    {
        push_coro_t sink2( boost::bind( echo, _1, i) );
        while ( sink2)
            sink2();
        sink1();
    }
}

int main( int argc, char * argv[])
{
    {
        pull_coro_t source( runit);
        while ( source) {
            std::cout << "-";
            source();
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
#else
typedef boost::coroutines::coroutine< void() >   coro_t;

void echo( coro_t & ca, int i)
{
    std::cout << i; 
    ca();
}

void runit( coro_t & ca)
{
    std::cout << "started! ";
    for ( int i = 0; i < 10; ++i)
    {
        coro_t c( boost::bind( echo, _1, i) );
        while ( c)
            c();
        ca();
    }
}

int main( int argc, char * argv[])
{
    {
        coro_t c( runit);
        while ( c) {
            std::cout << "-";
            c();
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
#endif
