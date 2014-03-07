//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
Revision history:
25 August 2005 : Initial version.
*/

#include <boost/test/minimal.hpp>

///////////////////////////////////////////////////////////////////////////////
// define a user-defined collection type and teach BOOST_FOREACH how to enumerate it
//
namespace mine
{
    struct dummy {};
    char * range_begin(mine::dummy&) {return 0;}
    char const * range_begin(mine::dummy const&) {return 0;}
    char * range_end(mine::dummy&) {return 0;}
    char const * range_end(mine::dummy const&) {return 0;}
}

#include <boost/foreach.hpp>

namespace boost
{
    template<>
    struct range_mutable_iterator<mine::dummy>
    {
        typedef char * type;
    };
    template<>
    struct range_const_iterator<mine::dummy>
    {
        typedef char const * type;
    };
}

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    // loop over a user-defined type (just make sure this compiles)
    mine::dummy d;
    BOOST_FOREACH( char c, d )
    {
        ((void)c); // no-op
    }

    return 0;
}
