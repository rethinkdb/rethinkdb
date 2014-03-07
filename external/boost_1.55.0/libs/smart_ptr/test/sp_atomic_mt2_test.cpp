
// Copyright (c) 2008 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include <boost/detail/lightweight_mutex.hpp>
#include <boost/detail/lightweight_thread.hpp>

#include <vector>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <ctime>

//

static void next_value( unsigned & v )
{
    v = v % 2? 3 * v + 1: v / 2;
}

struct X
{
    std::vector<unsigned> v_;

    explicit X( std::size_t n ): v_( n )
    {
        for( std::size_t i = 0; i < n; ++i )
        {
            v_[ i ] = i;
        }
    }

    unsigned get() const
    {
        return std::accumulate( v_.begin(), v_.end(), 0 );
    }

    void set()
    {
        std::for_each( v_.begin(), v_.end(), next_value );
    }
};

static boost::shared_ptr<X> ps;

static boost::detail::lightweight_mutex lm;
static boost::shared_mutex rw;

enum prim_type
{
    pt_mutex,
    pt_rwlock,
    pt_atomics
};

int read_access( prim_type pt )
{
    switch( pt )
    {
    case pt_mutex:
        {
            boost::detail::lightweight_mutex::scoped_lock lock( lm );
            return ps->get();
        }

    case pt_rwlock:
        {
            boost::shared_lock<boost::shared_mutex> lock( rw );
            return ps->get();
        }

    case pt_atomics:
        {
            boost::shared_ptr<X> p2 = boost::atomic_load( &ps );
            return p2->get();
        }
    }
}

void write_access( prim_type pt )
{
    switch( pt )
    {
    case pt_mutex:
        {
            boost::detail::lightweight_mutex::scoped_lock lock( lm );
            ps->set();
        }
        break;

    case pt_rwlock:
        {
            boost::unique_lock<boost::shared_mutex> lock( rw );
            ps->set();
        }
        break;

    case pt_atomics:
        {
            boost::shared_ptr<X> p1 = boost::atomic_load( &ps );

            for( ;; )
            {
                boost::shared_ptr<X> p2( new X( *p1 ) );
                p2->set();

                if( boost::atomic_compare_exchange( &ps, &p1, p2 ) ) break;
            }
        }
        break;
    }
}

void worker( int k, prim_type pt, int n, int r )
{
    ++r;

    unsigned s = 0, nr = 0, nw = 0;

    for( int i = 0; i < n; ++i )
    {
        if( i % r )
        {
            s += read_access( pt );
            ++nr;
        }
        else
        {
            write_access( pt );
            ++s;
            ++nw;
        }
    }

    printf( "Worker %2d: %u:%u, %10u\n", k, nr, nw, s );
}

#if defined( BOOST_HAS_PTHREADS )
  char const * thmodel = "POSIX";
#else
  char const * thmodel = "Windows";
#endif

char const * pt_to_string( prim_type pt )
{
    switch( pt )
    {
    case pt_mutex:

        return "mutex";

    case pt_rwlock:

        return "rwlock";

    case pt_atomics:

        return "atomics";
    }
}

static void handle_pt_option( std::string const & opt, prim_type & pt, prim_type pt2 )
{
    if( opt == pt_to_string( pt2 ) )
    {
        pt = pt2;
    }
}

static void handle_int_option( std::string const & opt, std::string const & prefix, int & k, int kmin, int kmax )
{
    if( opt.substr( 0, prefix.size() ) == prefix )
    {
        int v = atoi( opt.substr( prefix.size() ).c_str() );

        if( v >= kmin && v <= kmax )
        {
            k = v;
        }
    }
}

int main( int ac, char const * av[] )
{
    using namespace std; // printf, clock_t, clock

    int m = 4;          // threads
    int n = 10000;      // vector size
    int k = 1000000;    // iterations
    int r = 100;        // read/write ratio, r:1

    prim_type pt = pt_atomics;

    for( int i = 1; i < ac; ++i )
    {
        handle_pt_option( av[i], pt, pt_mutex );
        handle_pt_option( av[i], pt, pt_rwlock );
        handle_pt_option( av[i], pt, pt_atomics );

        handle_int_option( av[i], "n=", n, 1, INT_MAX );
        handle_int_option( av[i], "size=", n, 1, INT_MAX );

        handle_int_option( av[i], "k=", k, 1, INT_MAX );
        handle_int_option( av[i], "iterations=", k, 1, INT_MAX );

        handle_int_option( av[i], "m=", m, 1, INT_MAX );
        handle_int_option( av[i], "threads=", m, 1, INT_MAX );

        handle_int_option( av[i], "r=", r, 1, INT_MAX );
        handle_int_option( av[i], "ratio=", r, 1, INT_MAX );
    }

    printf( "%s: threads=%d size=%d iterations=%d ratio=%d %s\n\n", thmodel, m, n, k, r, pt_to_string( pt ) );

    ps.reset( new X( n ) );

    clock_t t = clock();

    std::vector<pthread_t> a( m );

    for( int i = 0; i < m; ++i )
    {
        boost::detail::lw_thread_create( a[ i ], boost::bind( worker, i, pt, k, r ) );
    }

    for( int j = 0; j < m; ++j )
    {
        pthread_join( a[ j ], 0 );
    }

    t = clock() - t;

    double ts = static_cast<double>( t ) / CLOCKS_PER_SEC;
    printf( "%.3f seconds, %.3f accesses per microsecond.\n", ts, m * k / ts / 1e+6 );
}
