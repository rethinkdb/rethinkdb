/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "match_manip.hpp"

int
main()
{
    using boost::spirit::qi::_1;
    using boost::spirit::qi::_2;
    using boost::spirit::qi::match;
    using boost::spirit::qi::phrase_match;
    using boost::spirit::qi::typed_stream;
    using boost::spirit::qi::stream;
    using boost::spirit::qi::int_;

    using namespace boost::spirit::ascii;
    namespace fusion = boost::fusion;
    namespace phx = boost::phoenix;

    {
        char c = '\0';
        BOOST_TEST(test( "a",
            char_[phx::ref(c) = _1]
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( "a",
            match(char_[phx::ref(c) = _1])
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( " a",
            phrase_match(char_[phx::ref(c) = _1], space)
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( "a",
            match(char_, c)
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( " a",
            phrase_match(char_, space, c)
        ) && c == 'a');
    }

    {
        ///////////////////////////////////////////////////////////////////////
        typedef typed_stream<char> char_stream_type;
        char_stream_type const char_stream = char_stream_type();

        typedef typed_stream<int> int_stream_type;
        int_stream_type const int_stream = int_stream_type();

        ///////////////////////////////////////////////////////////////////////
        char c = '\0';
        BOOST_TEST(test( "a",
            char_stream[phx::ref(c) = _1]
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( "a",
            match(char_stream[phx::ref(c) = _1])
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( " a",
            phrase_match(char_stream[phx::ref(c) = _1], space)
        ) && c == 'a');

        int i = 0;
        BOOST_TEST(test( "42",
            int_stream[phx::ref(i) = _1]
        ) && i == 42);

        i = 0;
        BOOST_TEST(test( "42",
            match(int_stream[phx::ref(i) = _1])
        ) && i == 42);

        i = 0;
        BOOST_TEST(test( " 42",
            phrase_match(int_stream[phx::ref(i) = _1], space)
        ) && i == 42);

        ///////////////////////////////////////////////////////////////////////
        c = '\0';
        BOOST_TEST(test( "a",
            match(stream, c)
        ) && c == 'a');

        c = '\0';
        BOOST_TEST(test( " a",
            phrase_match(stream, space, c)
        ) && c == 'a');

        i = 0;
        BOOST_TEST(test( "42",
            match(stream, i)
        ) && i == 42);

        i = 0;
        BOOST_TEST(test( " 42",
            phrase_match(stream, space, i)
        ) && i == 42);
    }

    return boost::report_errors();
}

