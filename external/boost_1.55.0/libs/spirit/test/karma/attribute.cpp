//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/phoenix_limits.hpp>

#include <boost/fusion/include/struct.hpp>
#include <boost/fusion/include/nview.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
struct test_data
{
    std::string s1;
    std::string s2;
    int i1;
    double d1;
    std::string s3;
};

BOOST_FUSION_ADAPT_STRUCT(
    test_data,
    (int, i1)
    (std::string, s1)
    (std::string, s2)
    (std::string, s3)
    (double, d1)
)

///////////////////////////////////////////////////////////////////////////////
// this is just a test structure we need to use in place of an int
struct test_int_data1
{
    int i;
};

// so we provide a custom attribute transformation
namespace boost { namespace spirit { namespace traits
{
    template <>
    struct transform_attribute<test_int_data1 const, int, karma::domain>
    {
        typedef int type;
        static int pre(test_int_data1 const& d) { return d.i; }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
// this is another test structure we need to use in place of an int, but this
// time we use a reference to the embedded element
struct test_int_data2
{
    int i;
};

// so we provide a custom attribute transformation
namespace boost { namespace spirit { namespace traits
{
    template <>
    struct transform_attribute<test_int_data2 const, int, karma::domain>
    {
        typedef int const& type;
        static int const& pre(test_int_data2 const& d) { return d.i; }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace fusion = boost::fusion;
    namespace karma = boost::spirit::karma;

    test_data d1 = { "s11", "s12", 1, 2.5, "s13" };
    {

        BOOST_TEST(test("s121", 
            karma::string << karma::int_, 
            fusion::as_nview<2, 0>(d1)));

        BOOST_TEST(test_delimited("s12 1 ", 
            karma::string << karma::int_, 
            fusion::as_nview<2, 0>(d1), ' '));
    }

    {
        test_data d2 = { "s21", "s22", 2, 3.4, "s23" };
        typedef fusion::result_of::as_nview<test_data const, 1, 2, 4>::type 
            test_view;
        std::vector<test_data> v;
        v.push_back(d1);
        v.push_back(d2);

        karma::rule<output_iterator<char>::type, test_view()> r =
                karma::string << karma::string << karma::double_;

        BOOST_TEST(test("s11s122.5\ns21s223.4", r % karma::eol, v));
        BOOST_TEST(test_delimited("s11s122.5 \n s21s223.4 ", 
            r % karma::eol, v, ' '));
    }

    {
        test_int_data1 d = { 1 };
        BOOST_TEST(test("1", karma::attr_cast(karma::int_), d));
        BOOST_TEST(test("1", karma::attr_cast<test_int_data1>(karma::int_), d));
        BOOST_TEST(test("1", karma::attr_cast<test_int_data1, int>(karma::int_), d));
    }

    {
        test_int_data1 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data1> v;
        v.push_back(d[0]);
        v.push_back(d[1]);

        BOOST_TEST(test("1,2", karma::attr_cast(karma::int_) % ',', v));
        BOOST_TEST(test("1,2"
          , karma::attr_cast<test_int_data1>(karma::int_) % ',', v));
        BOOST_TEST(test("1,2"
          , karma::attr_cast<test_int_data1, int>(karma::int_) % ',', v));
    }

    {
        test_int_data1 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data1> v;
        v.push_back(d[0]);
        v.push_back(d[1]);

        karma::rule<output_iterator<char>::type, int()> r = karma::int_;
        BOOST_TEST(test("1,2", r % ',', v));
    }

    {
        test_int_data1 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data1> v;
        v.push_back(d[0]);
        v.push_back(d[1] );

// this won't compile as there is no defined transformation for
// test_int_data1 and double
//      BOOST_TEST(test("1.0,2.0", karma::attr_cast(karma::double_) % ',', v));
//      BOOST_TEST(test("1.0,2.0"
//        , karma::attr_cast<test_int_data1>(karma::double_) % ',', v));

        BOOST_TEST(test("1.0,2.0"
          , karma::attr_cast<test_int_data1, int>(karma::double_) % ',', v));

        karma::rule<output_iterator<char>::type, int()> r = karma::double_;
        BOOST_TEST(test("1.0,2.0", r % ',', v));
    }

    {
        test_int_data2 d = { 1 };
        BOOST_TEST(test("1", karma::attr_cast(karma::int_), d));
        BOOST_TEST(test("1", karma::attr_cast<test_int_data2>(karma::int_), d));
        BOOST_TEST(test("1", karma::attr_cast<test_int_data2, int>(karma::int_), d));
    }

    {
        test_int_data2 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data2> v;
        v.push_back(d[0]);
        v.push_back(d[1]);

        BOOST_TEST(test("1,2", karma::attr_cast(karma::int_) % ',', v));
        BOOST_TEST(test("1,2"
          , karma::attr_cast<test_int_data2>(karma::int_) % ',', v));
        BOOST_TEST(test("1,2"
          , karma::attr_cast<test_int_data2, int>(karma::int_) % ',', v));
    }

    {
        test_int_data2 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data2> v;
        v.push_back(d[0]);
        v.push_back(d[1]);

        karma::rule<output_iterator<char>::type, int()> r = karma::int_;
        BOOST_TEST(test("1,2", r % ',', v));
    }

    {
        test_int_data2 d[] = {{ 1 }, { 2 }};
        std::vector<test_int_data2> v;
        v.push_back(d[0]);
        v.push_back(d[1] );

// this won't compile as there is no defined transformation for
// test_int_data2 and double
//      BOOST_TEST(test("1.0,2.0", karma::attr_cast(karma::double_) % ',', v));
//      BOOST_TEST(test("1.0,2.0"
//        , karma::attr_cast<test_int_data2>(karma::double_) % ',', v));

        BOOST_TEST(test("1.0,2.0"
          , karma::attr_cast<test_int_data2, int>(karma::double_) % ',', v));

        karma::rule<output_iterator<char>::type, int()> r = karma::double_;
        BOOST_TEST(test("1.0,2.0", r % ',', v));
    }

    return boost::report_errors();
}
