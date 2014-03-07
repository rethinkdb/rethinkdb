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

// std
#include <sstream>
#include <algorithm>
#include <set>

// Boost
#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>

// Boost.Bimap
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/bimap/vector_of.hpp>
#include <boost/bimap/bimap.hpp>

namespace ba =  boost::assign;


void test_bimap_assign()
{
    using namespace boost::bimaps;

    // test
    {
        typedef bimap< list_of<int>, double > bm_type;
        bm_type bm = ba::list_of< bm_type::relation >(1,0.1)(2,0.2)(3,0.3);
        ba::push_back( bm )(4,0.4)(5,0.5);
        ba::insert( bm.right )(0.5,5)(0.6,6);
        ba::push_back( bm.left )(6,0.6)(7,0.7);
    }

    // test
    {
        typedef bimap< unordered_multiset_of<int>, vector_of<double>,
                       list_of_relation > bm_type;
        bm_type bm = ba::list_of< bm_type::relation >(1,0.1)(2,0.2)(3,0.3);
        ba::push_front( bm )(4,0.4)(5,0.5);
        ba::push_back( bm.right )(0.6,6)(0.7,7);
        ba::insert( bm.left )(8,0.8)(9,0.9);
    }

    // test
    {
        typedef bimap< int, vector_of<double>, right_based > bm_type;
        bm_type bm = ba::list_of< bm_type::relation >(1,0.1)(2,0.2)(3,0.3);
        ba::push_back( bm )(4,0.4)(5,0.5);
        ba::push_back( bm.right )(0.6,6)(0.7,7);
        ba::insert( bm.left )(8,0.8)(9,0.9);
    }

    // test
    {
        typedef bimap< int, vector_of<double>, set_of_relation<> > bm_type;
        bm_type bm = ba::list_of< bm_type::relation >(1,0.1)(2,0.2)(3,0.3);
        ba::insert( bm )(4,0.4)(5,0.5);
        ba::push_back( bm.right )(0.6,6)(0.7,7);
        ba::insert( bm.left )(8,0.8)(9,0.9);
    }
}


int test_main( int, char* [] )
{
    test_bimap_assign();
    return 0;
}

