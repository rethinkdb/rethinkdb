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
        char a = '\0', b = '\0';
        BOOST_TEST(test( "ab",
            char_[phx::ref(a) = _1] >> char_[phx::ref(b) = _1]
        ) && a == 'a' && b == 'b');

        a = '\0', b = '\0';
        BOOST_TEST(test( "ab",
            match(char_[phx::ref(a) = _1] >> char_[phx::ref(b) = _1])
        ) && a == 'a' && b == 'b');

        a = '\0', b = '\0';
        BOOST_TEST(test( " a b",
            phrase_match(char_[phx::ref(a) = _1] >> char_[phx::ref(b) = _1], space)
        ) && a == 'a' && b == 'b');

        fusion::vector<char, char> t;
        BOOST_TEST(test( "ab",
            match(char_ >> char_, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b');

        t = fusion::vector<char, char>();
        BOOST_TEST(test( " a b",
            phrase_match(char_ >> char_, space, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b');
    }

    {
        char a = '\0', b = '\0', c = '\0';
        BOOST_TEST(test( "abc",
            char_[phx::ref(a) = _1] >> char_[phx::ref(b) = _1] >> char_[phx::ref(c) = _1]
        ) && a == 'a' && b == 'b' && c == 'c');

        BOOST_TEST(test( "abc",
            match(char_('a') >> char_('b') >> char_('c'))
        ));

        BOOST_TEST(test( " a b c",
            phrase_match(char_('a') >> char_('b') >> char_('c'), space)
        ));

        BOOST_TEST(!test( "abc",
            match(char_('a') >> char_('b') >> char_('d'))
        ));

        BOOST_TEST(!test( " a b c",
            phrase_match(char_('a') >> char_('b') >> char_('d'), space)
        ));

        fusion::vector<char, char, char> t;
        BOOST_TEST(test( "abc",
            match(char_ >> char_ >> char_, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b' && fusion::at_c<2>(t) == 'c');

        t = fusion::vector<char, char, char>();
        BOOST_TEST(test( " a b c",
            phrase_match(char_ >> char_ >> char_, space, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b' && fusion::at_c<2>(t) == 'c');

        t = fusion::vector<char, char, char>();
        BOOST_TEST(test( "abc",
            match(t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b' && fusion::at_c<2>(t) == 'c');

        t = fusion::vector<char, char, char>();
        BOOST_TEST(test( " a b c",
            phrase_match(t, space)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 'b' && fusion::at_c<2>(t) == 'c');
    }

    return boost::report_errors();
}

