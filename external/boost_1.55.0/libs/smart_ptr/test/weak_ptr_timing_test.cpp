#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  weak_ptr_timing_test.cpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>
#include <cstdio>
#include <ctime>
#include <cstdlib>

//

int const n = 29000;
int const k = 2048;

void test( std::vector< boost::shared_ptr<int> > & v )
{
    using namespace std; // printf, rand

    std::vector< boost::weak_ptr<int> > w( v.begin(), v.end() );

    int s = 0, r = 0;

    for( int i = 0; i < n; ++i )
    {
        // randomly kill a pointer

        v[ rand() % k ].reset();

        for( int j = 0; j < k; ++j )
        {
            if( boost::shared_ptr<int> px = w[ j ].lock() )
            {
                ++s;
            }
            else
            {
                ++r;
                w[ j ] = v[ rand() % k ];
            }
        }
    }

    printf( "\n%d locks, %d rebinds.", s, r );
}

int main()
{
    using namespace std; // printf, clock_t, clock

    std::vector< boost::shared_ptr<int> > v( k );

    for( int i = 0; i < k; ++i )
    {
        v[ i ].reset( new int( 0 ) );
    }

    clock_t t = clock();

    test( v );

    t = clock() - t;

    printf( "\n\n%.3f seconds.\n", static_cast<double>( t ) / CLOCKS_PER_SEC );

    return 0;
}
