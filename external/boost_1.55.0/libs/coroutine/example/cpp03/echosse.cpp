
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <emmintrin.h>

#include <boost/bind.hpp>
#include <boost/coroutine/all.hpp>

void echoSSE( int i)
{
    __m128i xmm;
    xmm = _mm_set_epi32(i, i+1, i+2, i+3);
    uint32_t v32[4];
    
    memcpy(&v32, &xmm, 16);

    std::cout << v32[0]; 
    std::cout << v32[1]; 
    std::cout << v32[2]; 
    std::cout << v32[3]; 
}

void echo( boost::coroutines::coroutine< void >::push_type & c, int i)
{
    std::cout << i << ":"; 
    echoSSE(i);
    c();
}

void runit( boost::coroutines::coroutine< void >::push_type & ca)
{
    std::cout << "started! ";
    for ( int i = 0; i < 10; ++i)
    {
        boost::coroutines::coroutine< void >::pull_type c( boost::bind( echo, _1, i) );
        while ( c)
            c();
        ca();
    }
}

int main( int argc, char * argv[])
{
    {
        boost::coroutines::coroutine< void >::pull_type c( runit);
        while ( c) {
            std::cout << "-";
            c();
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
