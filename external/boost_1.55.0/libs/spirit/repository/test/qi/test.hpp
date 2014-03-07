/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TEST_FEBRUARY_01_2007_0605PM)
#define BOOST_SPIRIT_TEST_FEBRUARY_01_2007_0605PM

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_what.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/foreach.hpp>
#include <iostream>

namespace spirit_test
{
    template <typename Char, typename Parser>
    bool test(Char const* in, Parser const& p, bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        Char const* last = in;
        while (*last)
            last++;
        return boost::spirit::qi::parse(in, last, p)
            && (!full_match || (in == last));
    }

    template <typename Char, typename Parser, typename Skipper>
    bool test(Char const* in, Parser const& p
        , Skipper const& s, bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        Char const* last = in;
        while (*last)
            last++;
        return boost::spirit::qi::phrase_parse(in, last, p, s)
            && (!full_match || (in == last));
    }

    template <typename Char, typename Parser, typename Attr>
    bool test_attr(Char const* in, Parser const& p
        , Attr& attr, bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        Char const* last = in;
        while (*last)
            last++;
        return boost::spirit::qi::parse(in, last, p, attr)
            && (!full_match || (in == last));
    }

    template <typename Char, typename Parser, typename Attr, typename Skipper>
    bool test_attr(Char const* in, Parser const& p
        , Attr& attr, Skipper const& s, bool full_match = true)
    {
        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        boost::spirit::qi::what(p);

        Char const* last = in;
        while (*last)
            last++;
        return boost::spirit::qi::phrase_parse(in, last, p, s, attr)
            && (!full_match || (in == last));
    }

    struct printer
    {
        typedef boost::spirit::utf8_string string;

        void element(string const& tag, string const& value, int depth) const
        {
            for (int i = 0; i < (depth*4); ++i) // indent to depth
                std::cout << ' ';

            std::cout << "tag: " << tag;
            if (value != "")
                std::cout << ", value: " << value;
            std::cout << std::endl;
        }
    };

    void print_info(boost::spirit::info const& what)
    {
        using boost::spirit::basic_info_walker;

        printer pr;
        basic_info_walker<printer> walker(pr, what.tag, 0);
        boost::apply_visitor(walker, what.value);
    }
}

#endif
