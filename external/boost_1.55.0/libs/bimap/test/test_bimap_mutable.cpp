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
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/vector_of.hpp>
#include <boost/bimap/unconstrained_set_of.hpp>

using namespace boost::bimaps;

template< class BimapType >
void test_bimap_mutable()
{
    typedef BimapType bm_type;

    bm_type bm;
    bm.insert( BOOST_DEDUCED_TYPENAME bm_type::value_type(1,0.1) );

    const bm_type & cbm = bm;

    // Map Mutable Iterator test
    {

    BOOST_DEDUCED_TYPENAME bm_type::left_iterator iter = bm.left.begin();
    iter->second = 0.2;
    BOOST_CHECK( iter->second == bm.left.begin()->second );

    BOOST_DEDUCED_TYPENAME bm_type::left_const_iterator citer = bm.left.begin();
    BOOST_CHECK( citer->second == bm.left.begin()->second );

    BOOST_DEDUCED_TYPENAME bm_type::left_const_iterator cciter = cbm.left.begin();
    BOOST_CHECK( cciter->second == cbm.left.begin()->second );

    }

    // Set Mutable Iterator test
    {

    BOOST_DEDUCED_TYPENAME bm_type::iterator iter = bm.begin();
    iter->right = 0.1;
    BOOST_CHECK( iter->right == bm.begin()->right );

    BOOST_DEDUCED_TYPENAME bm_type::const_iterator citer = bm.begin();
    BOOST_CHECK( citer->right == bm.begin()->right );

    BOOST_DEDUCED_TYPENAME bm_type::const_iterator cciter = cbm.begin();
    BOOST_CHECK( cciter->left == cbm.begin()->left );

    }

    // Map Assignable Reference test
    {

    BOOST_DEDUCED_TYPENAME bm_type::left_reference r = *bm.left.begin();
    r.second = 0.2;
    BOOST_CHECK( r == *bm.left.begin() );

    BOOST_DEDUCED_TYPENAME bm_type::left_const_reference cr = *bm.left.begin();
    BOOST_CHECK( cr == *bm.left.begin() );

    BOOST_DEDUCED_TYPENAME bm_type::left_const_reference ccr = *cbm.left.begin();
    BOOST_CHECK( ccr == *cbm.left.begin() );

    }

    // Set Assignable Reference test
    {

    BOOST_DEDUCED_TYPENAME bm_type::reference r = *bm.begin();
    r.right = 0.1;
    BOOST_CHECK( r == *bm.begin() );

    BOOST_DEDUCED_TYPENAME bm_type::const_reference cr = *bm.begin();
    BOOST_CHECK( cr == *bm.begin() );

    BOOST_DEDUCED_TYPENAME bm_type::const_reference ccr = *cbm.begin();
    BOOST_CHECK( ccr == *bm.begin() );

    }
}

int test_main( int, char* [] )
{
    test_bimap_mutable< bimap< int, list_of<double> > >();
    test_bimap_mutable< bimap< int, vector_of<double> > >();
    test_bimap_mutable< bimap< int, unconstrained_set_of<double> > >();
    return 0;
}

