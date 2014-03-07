// Boost.Range library
//
//  Copyright Thorsten Ottosen & Larry Evans 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

//#include <boost/range/as_array.hpp>

#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // suppress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/range/iterator_range.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>
#include <vector>

void check_reference_type();

void check_iterator_range()
{

    typedef std::string::iterator                 iterator;
    typedef std::string::const_iterator           const_iterator;
    typedef boost::iterator_range<iterator>       irange;
    typedef boost::iterator_range<const_iterator> cirange;
    std::string       str  = "hello world";
    const std::string cstr = "const world";
    irange r    = boost::make_iterator_range( str );
    r           = boost::make_iterator_range( str.begin(), str.end() );
    cirange r2  = boost::make_iterator_range( cstr );
    r2          = boost::make_iterator_range( cstr.begin(), cstr.end() );
    r2          = boost::make_iterator_range( str );

    BOOST_CHECK( !r.empty() );
    BOOST_CHECK( !r2.empty() );

//#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
//    if( !(bool)r )
//        BOOST_CHECK( false );
//    if( !(bool)r2 )
//        BOOST_CHECK( false );
//#else
    if( !r )
        BOOST_CHECK( false );
    if( !r2 )
        BOOST_CHECK( false );
//#endif

    BOOST_CHECK_EQUAL( r.size(), size( r ) );
    BOOST_CHECK_EQUAL( r2.size(), size( r2 ) );

    BOOST_CHECK_EQUAL( std::distance( r.begin(), r.end() ),
                       std::distance( boost::begin( r2 ), boost::end( r2 ) ) );
    std::cout << r << r2;


#ifndef BOOST_NO_STD_WSTRING
    std::wcout << boost::make_iterator_range( std::wstring( L"a wide string" ) )
               << boost::make_iterator_range( L"another wide string" );
#endif

    std::string res  = boost::copy_range<std::string>( r );
    BOOST_CHECK_EQUAL_COLLECTIONS( res.begin(), res.end(), r.begin(), r.end() );

    irange rr = boost::make_iterator_range( str );
    BOOST_CHECK( rr.equal( r ) );

    rr  = boost::make_iterator_range( str.begin(), str.begin() + 5 );
    BOOST_CHECK( rr == boost::as_literal("hello") );
    BOOST_CHECK( rr != boost::as_literal("hell") );
    BOOST_CHECK( rr < boost::as_literal("hello dude") );
    BOOST_CHECK( boost::as_literal("hello") == rr );
    BOOST_CHECK( boost::as_literal("hell")  != rr );
    BOOST_CHECK( ! (boost::as_literal("hello dude") < rr ) );
    irange rrr = rr;
    BOOST_CHECK( rrr == rr );
    BOOST_CHECK( !( rrr != rr ) );
    BOOST_CHECK( !( rrr < rr ) );

    const irange cr = boost::make_iterator_range( str );
    BOOST_CHECK_EQUAL( cr.front(), 'h' );
    BOOST_CHECK_EQUAL( cr.back(), 'd' );
    BOOST_CHECK_EQUAL( cr[1], 'e' );
    BOOST_CHECK_EQUAL( cr(1), 'e' );

    rrr = boost::make_iterator_range( str, 1, -1 );
    BOOST_CHECK( rrr == boost::as_literal("ello worl") );
    rrr = boost::make_iterator_range( rrr, -1, 1 );
    BOOST_CHECK( rrr == str );

    check_reference_type();
    
    // Check that an iterator range can be instantiated with
    // a pointer to an array as an iterator.
    int arr[2][2];
    boost::make_iterator_range(arr, arr + 2);
}

namespace iterator_range_test_detail
{
    struct less
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l < r;
        }
    };
    
    struct greater
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l > r;
        }
    };
    
    struct less_or_equal
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l <= r;
        }
    };
    
    struct greater_or_equal
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l >= r;
        }
    };
    
    struct equal_to
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l == r;
        }
    };
    
    struct not_equal_to
    {
        template< class Left, class Right >
        bool operator()(const Left& l, const Right& r) const
        {
            return l != r;
        }
    };
    
    template< class Pred >
    void check_iterator_range_operators_impl(Pred pred)
    {
        std::vector<std::string> vals;
        vals.push_back(std::string());
        vals.push_back("a");
        vals.push_back("b");
        vals.push_back("z");
        vals.push_back("ab");
        vals.push_back("ba");
        vals.push_back("abc");
        vals.push_back("cba");
        vals.push_back("aa");
        vals.push_back("aaa");
        vals.push_back("aab");
        vals.push_back("bba");

        typedef std::string::const_iterator citer;
        typedef boost::iterator_range<citer> iter_range;

        typedef std::vector<std::string>::const_iterator value_const_iterator;
        value_const_iterator first_val = vals.begin();
        value_const_iterator last_val = vals.end();
        
        for (value_const_iterator left_it = first_val; left_it != last_val; ++left_it)
        {
            const std::string& leftValue = *left_it;
            for (value_const_iterator right_it = first_val; right_it != last_val; ++right_it)
            {
                const std::string& rightValue = *right_it;
                iter_range left = boost::make_iterator_range(leftValue);
                iter_range right = boost::make_iterator_range(rightValue);
                
                const bool reference = pred(leftValue, rightValue);
                
                BOOST_CHECK_EQUAL( pred(left, right), reference );
                BOOST_CHECK_EQUAL( pred(left, rightValue), reference );
                BOOST_CHECK_EQUAL( pred(leftValue, right), reference );
            }
        }
    }
    
    void check_iterator_range_from_array()
    {
        double source[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 };
        boost::iterator_range<double*> rng = boost::make_iterator_range(source);
        BOOST_CHECK_EQUAL_COLLECTIONS( rng.begin(), rng.end(),
                                       source, source + 6 );
    }
    
} // namespace iterator_range_test_detail

template<typename Pred>
inline void check_iterator_range_operator()
{
    iterator_range_test_detail::check_iterator_range_operators_impl(
        Pred());
}

boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add(BOOST_TEST_CASE(&check_iterator_range));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::less>));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::less_or_equal>));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::greater>));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::greater_or_equal>));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::equal_to>));
    test->add(BOOST_TEST_CASE(&check_iterator_range_operator<iterator_range_test_detail::not_equal_to>));

    return test;
}

//
//
// Check that constness is propagated correct from
// the iterator types.
//
// Test contributed by Larry Evans.
//

template< class Container >
int test_iter_range( Container& a_cont )
{
    typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type citer_type;
    typedef boost::iterator_range<citer_type> riter_type;
    riter_type a_riter( boost::make_iterator_range( a_cont ) );
    a_riter.front();
    a_riter.back();
    int i = a_riter[0];
    return i;
}



void check_reference_type()
{
    typedef std::vector<int> veci_type;
    veci_type a_vec;
    a_vec.push_back( 999 );
    test_iter_range<veci_type>(a_vec);
    test_iter_range<veci_type const>(a_vec);
}
