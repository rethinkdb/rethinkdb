// Copyright (c) 2001-2011 Hartmut Kaiser
// Copyright (c) 2001-2011 Joel de Guzman
// Copyright (c)      2010 Bryce Lelbach
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/type_traits/is_same.hpp>

#include <sstream>

#include "test.hpp"

template <typename Expr, typename Iterator = boost::spirit::unused_type>
struct attribute_of_parser
{
    typedef typename boost::spirit::result_of::compile<
        boost::spirit::qi::domain, Expr
    >::type parser_expression_type;

    typedef typename boost::spirit::traits::attribute_of<
        parser_expression_type, boost::spirit::unused_type, Iterator
    >::type type;
};

template <typename Expected, typename Expr>
inline bool compare_attribute_type(Expr const&)
{
    typedef typename attribute_of_parser<Expr>::type type;
    return boost::is_same<type, Expected>::value;
}

inline bool check(boost::spirit::utree const& val, std::string expected)
{
    std::stringstream s;
    s << val;
    if (s.str() == expected + " ")
        return true;

    std::cerr << "got result: " << s.str() 
              << ", expected: " << expected << std::endl;
    return false;
}

int main()
{
    using spirit_test::test_attr;
    using boost::spirit::utree;
    using boost::spirit::utree_type;
    using boost::spirit::utf8_string_range_type;
    using boost::spirit::utf8_symbol_type;
    using boost::spirit::utf8_string_type;

    using boost::spirit::qi::real_parser;
    using boost::spirit::qi::strict_real_policies;
    using boost::spirit::qi::digit;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::string;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::space;
    using boost::spirit::qi::space_type;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::as;
    using boost::spirit::qi::lexeme;

    // kleene star
    {
        typedef real_parser<double, strict_real_policies<double> >
            strict_double_type;
        strict_double_type const strict_double = strict_double_type();

        utree ut;
        BOOST_TEST(test_attr("xy", *char_, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));
        ut.clear();
        BOOST_TEST(test_attr("123 456", *int_, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( 123 456 )"));
        ut.clear();
        BOOST_TEST(test_attr("1.23 4.56", *double_, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.23 4.56 )"));
        ut.clear();

        rule<char const*, utree(), space_type> r1;
        rule<char const*, utree::list_type(), space_type> r2 = '(' >> *r1 >> ')';
        r1 = strict_double | int_ | ~char_("()") | r2;

        BOOST_TEST(test_attr("(x y)", r1, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));
        ut.clear();
        BOOST_TEST(test_attr("(((123)) 456)", r1, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( ( ( 123 ) ) 456 )"));
        ut.clear();
        BOOST_TEST(test_attr("((1.23 4.56))", r1, ut, space) &&
            ut.which() == utree_type::list_type && check(ut, "( ( 1.23 4.56 ) )"));
        ut.clear();
        BOOST_TEST(test_attr("x", r1, ut, space) &&
            ut.which() == utree_type::string_type && check(ut, "\"x\""));
        ut.clear();
        BOOST_TEST(test_attr("123", r1, ut, space) &&
            ut.which() == utree_type::int_type && check(ut, "123"));
        ut.clear();
        BOOST_TEST(test_attr("123.456", r1, ut, space) &&
            ut.which() == utree_type::double_type && check(ut, "123.456"));
        ut.clear();
        BOOST_TEST(test_attr("()", r1, ut, space) &&
            ut.which() == utree_type::list_type && 
            check(ut, "( )"));
        ut.clear();
        BOOST_TEST(test_attr("((()))", r1, ut, space) &&
            ut.which() == utree_type::list_type && 
            check(ut, "( ( ( ) ) )")); 
        ut.clear();
    }

    // special attribute transformation for utree in alternatives
    {
        rule<char const*, utree()> r1;
        rule<char const*, utree::list_type()> r2;

        BOOST_TEST(compare_attribute_type<utree>(
            r1 | -r1 | *r1 | r2 | -r2 | *r2));
    }

    // lists
    {
        utree ut;
        BOOST_TEST(test_attr("x,y", char_ % ',', ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));
        ut.clear();
        BOOST_TEST(test_attr("123,456", int_ % ',', ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 123 456 )"));
        ut.clear();
        BOOST_TEST(test_attr("1.23,4.56", double_ % ',', ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.23 4.56 )"));

        rule<char const*, std::vector<char>()> r1 = char_ % ',';
        ut.clear();
        BOOST_TEST(test_attr("x,y", r1, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( \"x\" \"y\" )"));

        rule<char const*, std::vector<int>()> r2 = int_ % ',';
        ut.clear();
        BOOST_TEST(test_attr("123,456", r2, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 123 456 )"));

        rule<char const*, std::vector<double>()> r3 = double_ % ',';
        ut.clear();
        BOOST_TEST(test_attr("1.23,4.56", r3, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.23 4.56 )"));

        rule<char const*, utree()> r4 = double_ % ',';
        ut.clear();
        BOOST_TEST(test_attr("1.23,4.56", r4, ut) &&
            ut.which() == utree_type::list_type && check(ut, "( 1.23 4.56 )"));
    }

    return boost::report_errors();
}
