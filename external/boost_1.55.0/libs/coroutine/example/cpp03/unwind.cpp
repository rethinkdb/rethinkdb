
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/coroutine/all.hpp>

#ifdef BOOST_COROUTINES_UNIDIRECT
struct X : private boost::noncopyable
{
    X() { std::cout << "X()" << std::endl; }
    ~X() { std::cout << "~X()" << std::endl; }
};

void fn( boost::coroutines::coroutine< void >::push_type & sink)
{
    X x;
    int i = 0;
    while ( true)
    {
        std::cout << "fn() : " << ++i << std::endl;
        sink();
    }
}

int main( int argc, char * argv[])
{
    {
        boost::coroutines::coroutine< void >::pull_type source( fn);
        for ( int k = 0; k < 3; ++k)
        {
            source();
        }
        std::cout << "destroying coroutine and unwinding stack" << std::endl;
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
#else
typedef boost::coroutines::coroutine< void() >   coro_t;

struct X : private boost::noncopyable
{
    X() { std::cout << "X()" << std::endl; }
    ~X() { std::cout << "~X()" << std::endl; }
};

void fn( coro_t & ca)
{
    X x;
    int i = 0;
    while ( true)
    {
        std::cout << "fn() : " << ++i << std::endl;
        ca();
    }
}

int main( int argc, char * argv[])
{
    {
        coro_t c( fn);
        for ( int k = 0; k < 3; ++k)
        {
            c();
        }
        std::cout << "destroying coroutine and unwinding stack" << std::endl;
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
#endif
