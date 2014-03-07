/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011 Matthias Born

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include "test.hpp"

int main()
{
// This test assumes a little endian architecture
#ifdef BOOST_LITTLE_ENDIAN
    using spirit_test::test_attr;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::locals;
    using boost::spirit::qi::little_word;
    using boost::spirit::qi::omit;
    using boost::spirit::qi::_1;
    using boost::spirit::qi::_a;
    using boost::spirit::qi::attr;

    rule<char const*, short int(), locals<short int> > pass; 
    pass = little_word;

    rule<char const*, short int(), locals<short int> > pass_ugly; 
    pass_ugly %= omit[little_word[_a=_1]] >> attr(_a);

    rule<char const*, short int(), locals<short int> > fail; 
    fail %= little_word[_a=_1];

    short int us = 0;
    BOOST_TEST(test_attr("\x01\x02", pass, us) && us == 0x0201);

    us = 0;
    BOOST_TEST(test_attr("\x01\x02", pass_ugly, us) && us == 0x0201);

    us = 0;
    BOOST_TEST(test_attr("\x01\x02", fail, us) && us == 0x0201);
#endif
    return boost::report_errors();
}
