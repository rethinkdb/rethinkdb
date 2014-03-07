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
        char a = '\0';
        int i = 0;
        BOOST_TEST(test( "a2",
            (char_ >> int_)[phx::ref(a) = _1, phx::ref(i) = _2]
        ) && a == 'a' && i == 2);

        fusion::vector<char, int> t;
        BOOST_TEST(test( "a2",
            match(char_ >> int_, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 2);

        t = fusion::vector<char, int>();
        BOOST_TEST(test( " a 2",
            phrase_match(char_ >> int_, space, t)
        ) && fusion::at_c<0>(t) == 'a' && fusion::at_c<1>(t) == 2);

        BOOST_TEST(!test( "a2",
            match(char_ >> alpha, t)
        ));
        BOOST_TEST(!test( " a 2",
            phrase_match(char_ >> alpha, space, t)
        ));
    }

    {
        // parse elements of a vector
        std::vector<char> v;
        BOOST_TEST(test( "abc",
            (*char_)[phx::ref(v) = _1]
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        v.clear();
        BOOST_TEST(test( "abc",
            match(*char_, v)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        v.clear();
        BOOST_TEST(test( " a b c",
            phrase_match(*char_, space, v)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        v.clear();
        BOOST_TEST(test( "abc",
            match(v)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        v.clear();
        BOOST_TEST(test( " a b c",
            phrase_match(v, space)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        // parse a comma separated list of vector elements
        v.clear();
        BOOST_TEST(test( "a,b,c",
            match(char_ % ',', v)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        v.clear();
        BOOST_TEST(test( " a , b , c",
            phrase_match(char_ % ',', space, v)
        ) && 3 == v.size() && v[0] == 'a' && v[1] == 'b' && v[2] == 'c');

        // output all elements of a list
        std::list<char> l;
        BOOST_TEST(test( "abc",
            match(*char_, l)
        ) && 3 == l.size() && is_list_ok(l));

        l.clear();
        BOOST_TEST(test( " a b c",
            phrase_match(*char_, space, l)
        ) && 3 == l.size() && is_list_ok(l));

        l.clear();
        BOOST_TEST(test( "abc",
            match(l)
        ) && 3 == l.size() && is_list_ok(l));

        l.clear();
        BOOST_TEST(test( " a b c",
            phrase_match(l, space)
        ) && 3 == l.size() && is_list_ok(l));
    }

    return boost::report_errors();
}

