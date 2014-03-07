// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/range/detail/implementation_help.hpp>
#include <boost/test/test_tools.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // suppress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
//#define BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION 1

#include <boost/range/iterator.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/range/size_type.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/difference_type.hpp>

#include <boost/range/functions.hpp>
#include <boost/range/detail/sfinae.hpp>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <iostream>
#include <vector>

using namespace boost;
using namespace std;

void check_partial_workaround()
{
    using namespace range_detail;
    using type_traits::yes_type;
    using type_traits::no_type;

    //////////////////////////////////////////////////////////////////////
    // string
    //////////////////////////////////////////////////////////////////////
    char*          c_ptr;
    const char*    cc_ptr;
    wchar_t*       w_ptr;
    const wchar_t* cw_ptr;

    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_string_impl( c_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_string_impl( cc_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_string_impl( w_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_string_impl( cw_ptr ) ) );

    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_char_ptr_impl( c_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( no_type ) == sizeof( is_char_ptr_impl( cc_ptr ) ) );

    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_wchar_t_ptr_impl( w_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( no_type ) == sizeof( is_wchar_t_ptr_impl( cw_ptr ) ) );
    
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_const_char_ptr_impl( c_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_const_char_ptr_impl( cc_ptr ) ) );
    
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_const_wchar_t_ptr_impl( w_ptr ) ) );
    BOOST_STATIC_ASSERT( sizeof( yes_type ) == sizeof( is_const_wchar_t_ptr_impl( cw_ptr ) ) );

    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::std_container_, 
                          boost::range_detail::range< vector<int> >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::std_pair_, 
                          boost::range_detail::range< pair<int,int> >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::array_, 
                          boost::range_detail::range< int[42] >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::char_ptr_, 
                          boost::range_detail::range< char* >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::const_char_ptr_, 
                          boost::range_detail::range< const char* >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::wchar_t_ptr_, 
                          boost::range_detail::range< wchar_t* >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::const_wchar_t_ptr_,
                          boost::range_detail::range< const wchar_t* >::type >::value ));
    BOOST_STATIC_ASSERT(( boost::is_same< boost::range_detail::std_container_, 
                          boost::range_detail::range< vector<int> >::type >::value ));

}

#else // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

void check_partial_workaround()
{
    //
    // test if warnings are generated
    //
    std::size_t s = boost::range_detail::array_size( "foo" );
    BOOST_CHECK_EQUAL( s,  4u );
}

#endif

#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &check_partial_workaround ) );

    return test;
}
