#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//  weak_ptr_mt_test.cpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright 2005, 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>

#include <vector>

#include <cstdio>
#include <ctime>
#include <cstdlib>

#include <boost/detail/lightweight_thread.hpp>

//

int const n = 16384;
int const k = 512; // vector size
int const m = 16; // threads

void test( std::vector< boost::shared_ptr<int> > & v )
{
    using namespace std; // printf, rand

    std::vector< boost::weak_ptr<int> > w( v.begin(), v.end() );

    int s = 0, f = 0, r = 0;

    for( int i = 0; i < n; ++i )
    {
        // randomly kill a pointer

        v[ rand() % k ].reset();
        ++s;

        for( int j = 0; j < k; ++j )
        {
            if( boost::shared_ptr<int> px = w[ j ].lock() )
            {
                ++s;

                if( rand() & 4 )
                {
                    continue;
                }

                // rebind anyway with prob. 50% for add_ref_lock() against weak_release() contention
                ++f;
            }
            else
            {
                ++r;
            }

            w[ j ] = v[ rand() % k ];
        }
    }

    printf( "\n%d locks, %d forced rebinds, %d normal rebinds.", s, f, r );
}

#if defined( BOOST_HAS_PTHREADS )

char const * thmodel = "POSIX";

#else

char const * thmodel = "Windows";

#endif

int main()
{
    using namespace std; // printf, clock_t, clock

    printf("Using %s threads: %d threads, %d * %d iterations: ", thmodel, m, n, k );

    std::vector< boost::shared_ptr<int> > v( k );

    for( int i = 0; i < k; ++i )
    {
        v[ i ].reset( new int( 0 ) );
    }

    clock_t t = clock();

    pthread_t a[ m ];

    for( int i = 0; i < m; ++i )
    {
        boost::detail::lw_thread_create( a[ i ], boost::bind( test, v ) );
    }

    v.resize( 0 ); // kill original copies

    for( int j = 0; j < m; ++j )
    {
        pthread_join( a[j], 0 );
    }

    t = clock() - t;

    printf("\n\n%.3f seconds.\n", static_cast<double>(t) / CLOCKS_PER_SEC);

    return 0;
}
