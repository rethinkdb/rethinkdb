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

#include <string>

// Boost.Bimap
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>

using namespace boost::bimaps;

struct  left_tag {};
struct right_tag {};

void test_bimap_project()
{
    typedef bimap
    <
                 tagged< int        ,  left_tag >,
        list_of< tagged< std::string, right_tag > >

    > bm_type;

    bm_type bm;

    bm.insert( bm_type::value_type(1,"1") );
    bm.insert( bm_type::value_type(2,"2") );

    bm_type::      iterator       iter = bm.begin();
    bm_type:: left_iterator  left_iter = bm.left.find(1);
    bm_type::right_iterator right_iter = bm.right.begin();

    const bm_type & cbm = bm;

    bm_type::      const_iterator       citer = cbm.begin();
    bm_type:: left_const_iterator  left_citer = cbm.left.find(1);
    bm_type::right_const_iterator right_citer = cbm.right.begin();

    // non const projection

    BOOST_CHECK( bm.project_up   (bm.end()) == bm.end()       );
    BOOST_CHECK( bm.project_left (bm.end()) == bm.left.end()  );
    BOOST_CHECK( bm.project_right(bm.end()) == bm.right.end() );

    BOOST_CHECK( bm.project_up   (iter) == iter       );
    BOOST_CHECK( bm.project_left (iter) == left_iter  );
    BOOST_CHECK( bm.project_right(iter) == right_iter );

    BOOST_CHECK( bm.project_up   (left_iter) == iter       );
    BOOST_CHECK( bm.project_left (left_iter) == left_iter  );
    BOOST_CHECK( bm.project_right(left_iter) == right_iter );

    BOOST_CHECK( bm.project_up   (right_iter) == iter       );
    BOOST_CHECK( bm.project_left (right_iter) == left_iter  );
    BOOST_CHECK( bm.project_right(right_iter) == right_iter );

    bm.project_up   ( left_iter)->right  = "u";
    bm.project_left (right_iter)->second = "l";
    bm.project_right(      iter)->first  = "r";

    // const projection

    BOOST_CHECK( cbm.project_up   (cbm.end()) == cbm.end()       );
    BOOST_CHECK( cbm.project_left (cbm.end()) == cbm.left.end()  );
    BOOST_CHECK( cbm.project_right(cbm.end()) == cbm.right.end() );

    BOOST_CHECK( cbm.project_up   (citer) == citer       );
    BOOST_CHECK( cbm.project_left (citer) == left_citer  );
    BOOST_CHECK( cbm.project_right(citer) == right_citer );

    BOOST_CHECK( cbm.project_up   (left_citer) == citer       );
    BOOST_CHECK( cbm.project_left (left_citer) == left_citer  );
    BOOST_CHECK( cbm.project_right(left_citer) == right_citer );

    BOOST_CHECK( cbm.project_up   (right_citer) == citer       );
    BOOST_CHECK( cbm.project_left (right_citer) == left_citer  );
    BOOST_CHECK( cbm.project_right(right_citer) == right_citer );

    // mixed projection

    BOOST_CHECK( bm.project_up   (left_citer) == iter       );
    BOOST_CHECK( bm.project_left (left_citer) == left_iter  );
    BOOST_CHECK( bm.project_right(left_citer) == right_iter );

    BOOST_CHECK( cbm.project_up   (right_iter) == citer       );
    BOOST_CHECK( cbm.project_left (right_iter) == left_citer  );
    BOOST_CHECK( cbm.project_right(right_iter) == right_citer );

    bm.project_up   ( left_citer)->right  = "u";
    bm.project_left (right_citer)->second = "l";
    bm.project_right(      citer)->first  = "r";

    // Support for tags

    BOOST_CHECK( bm.project< left_tag>(iter) == left_iter  );
    BOOST_CHECK( bm.project<right_tag>(iter) == right_iter );

    BOOST_CHECK( bm.project< left_tag>(left_iter) == left_iter  );
    BOOST_CHECK( bm.project<right_tag>(left_iter) == right_iter );

    BOOST_CHECK( bm.project< left_tag>(right_iter) == left_iter  );
    BOOST_CHECK( bm.project<right_tag>(right_iter) == right_iter );

    BOOST_CHECK( cbm.project< left_tag>(citer) == left_citer  );
    BOOST_CHECK( cbm.project<right_tag>(citer) == right_citer );

    BOOST_CHECK( cbm.project< left_tag>(left_citer) == left_citer  );
    BOOST_CHECK( cbm.project<right_tag>(left_citer) == right_citer );

    BOOST_CHECK( cbm.project< left_tag>(right_citer) == left_citer  );
    BOOST_CHECK( cbm.project<right_tag>(right_citer) == right_citer );

    bm.project< left_tag>(right_citer)->second = "l";
    bm.project<right_tag>( left_citer)->first  = "r";

}


int test_main( int, char* [] )
{
    test_bimap_project();
    return 0;
}

