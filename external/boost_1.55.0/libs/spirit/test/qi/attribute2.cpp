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

    {
        std::vector<test_int_data1> v;
        qi::rule<char const*, int()> r = qi::int_;

        BOOST_TEST(test_attr("1,2", r % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    {
        std::vector<double> v;
        qi::rule<char const*, int()> r = qi::int_;

        BOOST_TEST(test_attr("1,2", r % ',', v));
        BOOST_TEST(v.size() == 2 && v[0] == 1.0 && v[1] == 2.0);
    }

    {
        std::vector<test_int_data1> v;

// this won't compile as there is no defined transformation for
// test_int_data1 and double
//      BOOST_TEST(test_attr("1.0,2.2", qi::attr_cast(qi::double_) % ',', v));
//      BOOST_TEST(test_attr("1.0,2.2"
//        , qi::attr_cast<test_int_data1>(qi::double_) % ',', v));

        BOOST_TEST(test_attr("1.0,2.2"
          , qi::attr_cast<test_int_data1, int>(qi::double_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        qi::rule<char const*, int()> r = qi::double_;

        v.clear();
        BOOST_TEST(test_attr("1.0,2.0", r % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    // testing explicit transformation if attribute is taken by reference
    {
        test_int_data2 d = { 0 };
        BOOST_TEST(test_attr("1", qi::attr_cast(qi::int_), d));
        BOOST_TEST(d.i == 1);
        BOOST_TEST(test_attr("2", qi::attr_cast<test_int_data2>(qi::int_), d));
        BOOST_TEST(d.i == 2);
        BOOST_TEST(test_attr("3", qi::attr_cast<test_int_data2, int>(qi::int_), d));
        BOOST_TEST(d.i == 3);
    }

    {
        std::vector<test_int_data2> v;

        BOOST_TEST(test_attr("1,2", qi::attr_cast(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        v.clear();
        BOOST_TEST(test_attr("1,2"
          , qi::attr_cast<test_int_data2>(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        v.clear();
        BOOST_TEST(test_attr("1,2"
          , qi::attr_cast<test_int_data2, int>(qi::int_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    {
        std::vector<test_int_data2> v;
        qi::rule<char const*, int()> r = qi::int_;

        BOOST_TEST(test_attr("1,2", r % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    {
        std::vector<test_int_data2> v;

// this won't compile as there is no defined transformation for
// test_int_data2 and double
//      BOOST_TEST(test_attr("1.0,2.2", qi::attr_cast(qi::double_) % ',', v));
//      BOOST_TEST(test_attr("1.0,2.2"
//        , qi::attr_cast<test_int_data2>(qi::double_) % ',', v));

        BOOST_TEST(test_attr("1.0,2.2"
          , qi::attr_cast<test_int_data2, int>(qi::double_) % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);

        qi::rule<char const*, int()> r = qi::double_;

        v.clear();
        BOOST_TEST(test_attr("1.0,2.0", r % ',', v));
        BOOST_TEST(v.size() == 2 && v[0].i == 1 && v[1].i == 2);
    }

    return boost::report_errors();
}
