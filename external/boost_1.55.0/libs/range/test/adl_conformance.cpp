// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // suppress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

enum adl_types
{
    unused,
    boost_namespace,
    templated_namespace,
    non_templated_namespace,
    global_namespace
};

// Use boost_test rather than boost as the namespace for this test
// to allow the test framework to use boost::begin() etc. without
// violating the One Defintion Rule.
namespace boost_test
{
    namespace range_detail
    {
        template< class Range >
        inline typename Range::iterator begin( Range& r )
        {
            return boost_namespace;
        }

        template< class Range >
        inline typename Range::iterator begin( const Range& r )
        {
            return boost_namespace;
        }

    }

    template< class Range >
    inline typename Range::iterator begin( Range& r )
    {
        using range_detail::begin; // create ADL hook
        return begin( r );
    }

    template< class Range >
    inline typename Range::iterator begin( const Range& r )
    {
        using range_detail::begin; // create ADL hook
        return begin( r );
    }
} // 'boost_test'


namespace find_templated
{
    template< class T >
    struct range
    {
        typedef adl_types iterator;

        range()                { /* allow const objects */ }
        iterator begin()       { return unused; }
        iterator begin() const { return unused; }
        iterator end()         { return unused; }
        iterator end() const   { return unused; }
    };

    //
    // A fully generic version here will create
    // ambiguity.
    //
    template< class T >
    inline typename range<T>::iterator begin( range<T>& r )
    {
        return templated_namespace;
    }

    template< class T >
    inline typename range<T>::iterator begin( const range<T>& r )
    {
        return templated_namespace;
    }

}

namespace find_non_templated
{
    struct range
    {
        typedef adl_types iterator;

        range()                { /* allow const objects */ }
        iterator begin()       { return unused; }
        iterator begin() const { return unused; }
        iterator end()         { return unused; }
        iterator end() const   { return unused; }
    };

    inline range::iterator begin( range& r )
    {
        return non_templated_namespace;
    }


    inline range::iterator begin( const range& r )
    {
        return non_templated_namespace;
    }
}

struct range
{
    typedef adl_types iterator;

    range()                { /* allow const objects */ }
    iterator begin()       { return unused; }
    iterator begin() const { return unused; }
    iterator end()         { return unused; }
    iterator end() const   { return unused; }
};

inline range::iterator begin( range& r )
{
    return global_namespace;
}

inline range::iterator begin( const range& r )
{
    return global_namespace;
}

void check_adl_conformance()
{
    find_templated::range<int>       r;
    const find_templated::range<int> r2;
    find_non_templated::range        r3;
    const find_non_templated::range  r4;
    range                            r5;
    const range                      r6;

    //
    // Notice how ADL kicks in even when we have qualified
    // notation!
    //


    BOOST_CHECK( boost_test::begin( r )  != boost_namespace );
    BOOST_CHECK( boost_test::begin( r2 ) != boost_namespace );
    BOOST_CHECK( boost_test::begin( r3 ) != boost_namespace );
    BOOST_CHECK( boost_test::begin( r4 ) != boost_namespace );
    BOOST_CHECK( boost_test::begin( r5 ) != boost_namespace );
    BOOST_CHECK( boost_test::begin( r6 ) != boost_namespace );

    BOOST_CHECK_EQUAL( boost_test::begin( r ), templated_namespace ) ;
    BOOST_CHECK_EQUAL( boost_test::begin( r2 ), templated_namespace );
    BOOST_CHECK_EQUAL( boost_test::begin( r3 ), non_templated_namespace );
    BOOST_CHECK_EQUAL( boost_test::begin( r4 ), non_templated_namespace );
    BOOST_CHECK_EQUAL( boost_test::begin( r5 ), global_namespace );
    BOOST_CHECK_EQUAL( boost_test::begin( r6 ), global_namespace );
}

#include <boost/test/included/unit_test.hpp>

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &check_adl_conformance ) );

    return test;
}


