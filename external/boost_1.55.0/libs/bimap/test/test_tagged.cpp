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

// Boost.MPL
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/add_const.hpp>

// Boost.Bimap
#include <boost/bimap/detail/test/check_metadata.hpp>

// Boost.Bimap.Tags
#include <boost/bimap/tags/tagged.hpp>
#include <boost/bimap/tags/support/default_tagged.hpp>
#include <boost/bimap/tags/support/is_tagged.hpp>
#include <boost/bimap/tags/support/overwrite_tagged.hpp>
#include <boost/bimap/tags/support/tag_of.hpp>
#include <boost/bimap/tags/support/value_type_of.hpp>
#include <boost/bimap/tags/support/apply_to_value_type.hpp>




BOOST_BIMAP_TEST_STATIC_FUNCTION( test_metafunctions )
{
    using namespace boost::bimaps::tags::support;
    using namespace boost::bimaps::tags;
    using namespace boost::mpl::placeholders;
    using namespace boost;

    struct tag      {};
    struct value    {};

    // Test apply_to_value_type metafunction
    // tagged<value,tag> ----(add_const<_>)----> tagged<value const,tag>
    typedef tagged< value, tag > ttype;
    typedef apply_to_value_type< add_const<_>,ttype>::type result;
    typedef is_same<tagged<value const,tag>,result> compare;
    BOOST_MPL_ASSERT_MSG(compare::value,tag,(result));
}

struct tag_a {};
struct tag_b {};
struct data  {};

void test_function()
{

    using namespace boost::bimaps::tags::support;
    using namespace boost::bimaps::tags;
    using boost::is_same;

    typedef tagged< data, tag_a > data_a;
    typedef tagged< data, tag_b > data_b;

    BOOST_CHECK(( is_same< data_a::value_type   , data  >::value ));
    BOOST_CHECK(( is_same< data_a::tag          , tag_a >::value ));

    BOOST_CHECK((
        is_same< overwrite_tagged < data_a, tag_b >::type, data_b >::value
    ));
    BOOST_CHECK((
        is_same< default_tagged   < data_a, tag_b >::type, data_a >::value
    ));
    BOOST_CHECK(( 
        is_same< default_tagged   < data  , tag_b >::type, data_b >::value
    ));

    BOOST_CHECK(( is_tagged< data   >::value == false ));
    BOOST_CHECK(( is_tagged< data_a >::value == true  ));

    BOOST_CHECK(( is_same< value_type_of<data_a>::type, data  >::value ));
    BOOST_CHECK(( is_same< tag_of       <data_a>::type, tag_a >::value ));

}

int test_main( int, char* [] )
{
    test_function();

    // Test metanfunctions
    BOOST_BIMAP_CALL_TEST_STATIC_FUNCTION( test_metafunctions );

    return 0;
}

