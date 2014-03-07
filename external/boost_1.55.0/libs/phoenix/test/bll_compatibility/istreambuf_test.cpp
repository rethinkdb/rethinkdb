// istreambuf_test - test lambda function objects with istreambuf_iterator
//
// Copyright (c) 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/lambda/lambda.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iterator>
#include <sstream>
#include <algorithm>

int main()
{
    using namespace boost::lambda;

    std::stringstream is( "ax2" );

    std::istreambuf_iterator<char> b2( is );
    std::istreambuf_iterator<char> e2;

    std::istreambuf_iterator<char> i = std::find_if( b2, e2, _1 == 'x' );

    BOOST_TEST( *i == 'x' );
    BOOST_TEST( std::distance( i, e2 ) == 2 );

    return boost::report_errors();
}
