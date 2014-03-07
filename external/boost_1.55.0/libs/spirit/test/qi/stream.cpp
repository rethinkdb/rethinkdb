/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011 Brian O'Kennedy

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_stream.hpp>
#include <boost/spirit/include/qi_operator.hpp>

#include "test.hpp"

struct complex
{
    complex (double a = 0.0, double b = 0.0) : a(a), b(b) {}
    double a, b;
};

std::istream& operator>> (std::istream& is, complex& z)
{
    char lbrace = '\0', comma = '\0', rbrace = '\0';
    is >> lbrace >> z.a >> comma >> z.b >> rbrace;
    if (lbrace != '{' || comma != ',' || rbrace != '}')
        is.setstate(std::ios_base::failbit);

    return is;
}

int main()
{
    using spirit_test::test_attr;

    {
        using boost::spirit::qi::blank;
        using boost::spirit::qi::double_;
        using boost::spirit::qi::stream;
        using boost::spirit::qi::stream_parser;
        using boost::fusion::at_c;

        complex c;
        BOOST_TEST(test_attr("{1.0,2.5}", 
                stream_parser<char, complex>(), c, blank) && 
            c.a == 1.0 && c.b == 2.5);

        boost::fusion::vector<complex, double> d;
        BOOST_TEST(test_attr("{1.0,2.5},123.456", 
                stream >> ',' >> double_, d, blank) && 
            at_c<0>(d).a == 1.0 && at_c<0>(d).b == 2.5 && at_c<1>(d) == 123.456);
    }

    return boost::report_errors();
}


