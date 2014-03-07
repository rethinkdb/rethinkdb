// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

#include <boost/config.hpp>

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost.Bimap
#include <boost/bimap/support/lambda.hpp>
#include <boost/bimap/bimap.hpp>

void test_bimap_lambda()
{
    using namespace boost::bimaps;

    typedef bimap<int,double> bm;

    bm b;
    b.insert( bm::value_type(1,0.1) );

    BOOST_CHECK( b.size() == 1 );
    BOOST_CHECK( b.left.modify_key ( b.left.begin(),  _key =   2 ) );
    BOOST_CHECK( b.left.modify_data( b.left.begin(), _data = 0.2 ) );
    BOOST_CHECK( b.left.range( _key >= 1, _key < 3 ).first == b.left.begin() );
}

int test_main( int, char* [] )
{
    test_bimap_lambda();
    return 0;
}

