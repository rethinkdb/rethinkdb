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

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

// Boost.Bimap
#include <boost/bimap/support/lambda.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>

// Support metafunctions
#include <boost/bimap/support/data_type_by.hpp>
#include <boost/bimap/support/key_type_by.hpp>
#include <boost/bimap/support/map_type_by.hpp>
#include <boost/bimap/support/value_type_by.hpp>
#include <boost/bimap/support/iterator_type_by.hpp>
#include <boost/bimap/relation/support/pair_type_by.hpp>

using namespace boost::bimaps;
using namespace boost::bimaps::support;
using namespace boost::bimaps::relation::support ;

typedef bimap<int, unconstrained_set_of<double> > bm_type;

namespace support_metafunctions_test {

    typedef boost::is_same
    <
        data_type_by< member_at::left , bm_type >::type,
        key_type_by < member_at::right, bm_type >::type

    >::type test_metafunction_1;
    BOOST_STATIC_ASSERT(test_metafunction_1::value);

    typedef boost::is_same
    <
        data_type_by< member_at::right, bm_type >::type,
        key_type_by < member_at::left , bm_type >::type

    >::type test_metafunction_2;
    BOOST_STATIC_ASSERT(test_metafunction_2::value);

    typedef boost::is_same
    <
        map_type_by  < member_at::left , bm_type >::type::value_type,
        value_type_by< member_at::left , bm_type >::type

    >::type test_metafunction_3;
    BOOST_STATIC_ASSERT(test_metafunction_3::value);

    typedef boost::is_same
    <
        pair_type_by< member_at::left, bm_type::relation>::type,
        value_type_by< member_at::left , bm_type >::type

    >::type test_metafunction_4;
    BOOST_STATIC_ASSERT(test_metafunction_4::value);
    
} // namespace support_metafunctions_test

void test_bimap_extra()
{
    // extra tests
    // ---------------------------------------------------------------
    // This section test small things... when a group of this checks
    // can be related it is moved to a separate unit test file.



}


int test_main( int, char* [] )
{
    test_bimap_extra();
    return 0;
}

