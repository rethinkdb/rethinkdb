///////////////////////////////////////////////////////////////////////////////
// test_assert_with_placeholder.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::xpressive;

placeholder<int> const _cnt = {{}};

struct count_a_impl
{
    typedef void result_type;

    void operator()(int &cnt) const
    {
        ++cnt;
    }
};

boost::xpressive::function<count_a_impl>::type const count_a = {{}};

struct check_a_impl
{
    typedef bool result_type;

    bool operator()(int &cnt, int val) const
    {
        if(cnt < val)
        {
            ++cnt;
            return true;
        }
        return false;
    }
};

boost::xpressive::function<check_a_impl>::type const check_a = {{}};

void test_assert_with_placeholder()
{
    int cnt = 0;
    std::string a_str("a_aaaaa___a_aa_aaa_");
    const sregex expr1(*(as_xpr('a')[count_a(_cnt)] | '_'));
    const sregex expr2(*(as_xpr('a')[check(check_a(_cnt, 5))] | '_'));

    sregex_iterator iter1(a_str.begin(), a_str.end(), expr1,
                  let(_cnt = cnt));
    BOOST_CHECK_EQUAL(iter1->str(0), a_str);
    BOOST_CHECK_EQUAL(cnt, 12);

    cnt = 0;
    sregex_iterator iter2(a_str.begin(), a_str.end(), expr2,
                  let(_cnt = cnt));

    BOOST_CHECK_EQUAL(iter2->str(0), std::string("a_aaaa"));
    BOOST_CHECK_EQUAL(cnt, 5);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test that custom assertions can use argument placeholders");

    test->add(BOOST_TEST_CASE(&test_assert_with_placeholder));

    return test;
}
