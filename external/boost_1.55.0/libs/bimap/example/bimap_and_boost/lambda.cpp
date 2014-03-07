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

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

#include <string>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;

int main()
{
    //[ code_bimap_and_boost_lambda

    typedef bimap< std::string, int > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("one",1) );
    bm.insert( bm_type::value_type("two",2) );

    bm.right.range( 5 < _key, _key < 10 );

    bm.left.modify_key( bm.left.find("one"), _key = "1" );

    bm.left.modify_data( bm.left.begin(), _data *= 10 );
    //]
    return 0;
}


