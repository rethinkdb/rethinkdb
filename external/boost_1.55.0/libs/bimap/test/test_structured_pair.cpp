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
#include <utility>
#include <cstddef>

// Boost.Static_assert
#include <boost/static_assert.hpp>

// Boost.Bimap
#include <boost/bimap/detail/test/check_metadata.hpp>

#include <boost/bimap/relation/structured_pair.hpp>


BOOST_BIMAP_TEST_STATIC_FUNCTION( static_metadata_test )
{
    using namespace boost::bimaps::relation;

    struct data_a { char     data; };
    struct data_b { double   data; };

    typedef structured_pair
    <
        data_a,
        data_b,
        normal_layout

    > sp_ab;

    typedef structured_pair
    <
        data_b,
        data_a,
        mirror_layout

    > sp_ba;

    BOOST_BIMAP_CHECK_METADATA(sp_ab, first_type , data_a);
    BOOST_BIMAP_CHECK_METADATA(sp_ab, second_type, data_b);

    BOOST_BIMAP_CHECK_METADATA(sp_ba, first_type , data_b);
    BOOST_BIMAP_CHECK_METADATA(sp_ba, second_type, data_a);

}


void test_basic()
{

    using namespace boost::bimaps::relation;

    // Instanciate two pairs and test the storage alignmentDataData

    typedef structured_pair< short, double, normal_layout > pair_type;
    typedef structured_pair< double, short, mirror_layout > mirror_type;

    pair_type   pa( 2, 3.1416 );
    mirror_type pb( 3.1416, 2 );

    BOOST_CHECK( pa.first  == pb.second );
    BOOST_CHECK( pa.second == pb.first  );

}


int test_main( int, char* [] )
{

    BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION( static_are_storage_compatible_test );

    BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION( static_metadata_test );

    test_basic();

    return 0;
}

