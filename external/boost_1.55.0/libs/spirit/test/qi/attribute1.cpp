/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/phoenix_limits.hpp>

#include <boost/fusion/include/struct.hpp>
#include <boost/fusion/include/nview.hpp>

#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>

#include <iostream>
#include <vector>
#include <string>
#include "test.hpp"

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
struct test_int_data1
{
    int i;
};

// we provide a custom attribute transformation taking copy of the actual 
// attribute value, simulating more complex type transformations
namespace boost { namespace spirit { namespace traits
{
    template <>
    struct transform_attribute<test_int_data1, int, qi::domain>
    {
        typedef int type;
        static int pre(test_int_data1& d) { return d.i; }
        static void post(test_int_data1& d, int i) { d.i = i; }
        static void fail(test_int_data1&) {}
    };
}}}

///////////////////////////////////////////////////////////////////////////////
struct test_int_data2
{
    int i;
};

// we provide a simple custom attribute transformation utilizing passing the 
// actual attribute by reference
namespace boost { namespace spirit { namespace traits
{
    template <>
    struct transform_attribute<test_int_data2, int, qi::domain>
    {
        typedef int& type;
        static int& pre(test_int_data2& d) { return d.i; }
        static void post(test_int_data2&, int const&) {}
        static void fail(test_int_data2&) {}
    };
}}}

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using spirit_test::test_attr;
    namespace qi = boost::spirit::qi;
    namespace fusion = boost::fusion;

    // testing attribute reordering in a fusion sequence as explicit attribute
    {
        typedef fusion::result_of::as_nview<test_data, 1, 0, 4>::type 
            test_view;

        test_data d1 = { "", "", 0, 0.0, "" };
        test_view v1 = fusion::as_nview<1, 0, 4>(d1);
        BOOST_TEST(test_attr("s1,2,1.5", 
            *(qi::char_ - ',') >> ',' >> qi::int_ >> ',' >> qi::double_, v1));
        BOOST_TEST(d1.i1 == 2 && d1.s1 == "s1" && d1.d1 == 1.5);

        test_data d2 = { "", "", 0, 0.0, "" };
        test_view v2 = fusion::as_nview<1, 0, 4>(d2);
        BOOST_TEST(test_attr("s1, 2, 1.5 ", 
            *(qi::char_ - ',') >> ',' >> qi::int_ >> ',' >> qi::double_, 
            v2, qi::space));
        BOOST_TEST(d2.i1 == 2 && d2.s1 == "s1" && d2.d1 == 1.5);
    }

    {
        // this won't work without the second template argument as *digit
        // exposes a vector<char> as its attribute
        std::string str;
        BOOST_TEST(test_attr("123"
          , qi::attr_cast<std::string, std::string>(*qi::digit), str));
        BOOST_TEST(str == "123");
    }

    // testing attribute reordering in a fusion sequence involving a rule
    {
        typedef fusion::result_of::as_nview<test_data, 1, 0, 4>::type 
            test_view;
        std::vector<test_data> v;

        qi::rule<char const*, test_view()> r1 = 
                *(qi::char_ - ',') >> ',' >> qi::int_ >> ',' >> qi::double_;

        BOOST_TEST(test_attr("s1,2,1.5\ns2,4,3.5", r1 % qi::eol, v));
        BOOST_TEST(v.size() == 2 &&
            v[0].i1 == 2 && v[0].s1 == "s1" && v[0].d1 == 1.5 &&
            v[1].i1 == 4 && v[1].s1 == "s2" && v[1].d1 == 3.5);

        qi::rule<char const*, test_view(), qi::blank_type> r2 =
                *(qi::char_ - ',') >> ',' >> qi::int_ >> ',' >> qi::double_;

        v.clear();
        BOOST_TEST(test_attr("s1, 2, 1.5 \n s2, 4, 3.5", r2 % qi::eol, v, qi::blank));
        BOOST_TEST(v.size() == 2 &&
            v[0].i1 == 2 && v[0].s1 == "s1" && v[0].d1 == 1.5 &&
            v[1].i1 == 4 && v[1].s1 == "s2" && v[1].d1 == 3.5);
    }

    // testing explicit transformation if attribute needs to be copied
    {
        test_int_data1 d = { 0 };
        BOOST_TEST(test_attr("1", qi::attr_cast(qi::int_), d));
        BOOST_TEST(d.i == 1);
        BOOST_TEST(test_attr("2", qi::attr_cast<test_int_data1>(qi::int_), d));
        BOOST_TEST(d.i == 2);
        BOOST_TEST(test_attr("3", qi::attr_cast<test_int_data1, int>(qi::int_), d));
        BOOST_TEST(d.i == 3);
    }

    {
        std::vector<test_int_data1> v;

        BOOST_TEST(test_attr("1,2", qi::attr_cast(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        v.clear();
        BOOST_TEST(test_attr("1,2"
          , qi::attr_cast<test_int_data1>(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        v.clear();
        BOOST_TEST(test_attr("1,2"
          , qi::attr_cast<test_int_data1, int>(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    return boost::report_errors();
}
