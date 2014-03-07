//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define KARMA_FAIL_COMPILATION

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;
    using namespace boost::phoenix;

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test(" ", space));
        BOOST_TEST(test(L" ", space));
        BOOST_TEST(!test("\t", space));
        BOOST_TEST(!test(L"\t", space));

        BOOST_TEST(test(" ", space, ' '));
        BOOST_TEST(test(L" ", space, L' '));
        BOOST_TEST(test("\t", space, '\t'));
        BOOST_TEST(test(L"\t", space, L'\t'));

        BOOST_TEST(!test("", space, 'x'));
        BOOST_TEST(!test(L"", space, L'x'));

        BOOST_TEST(!test(" ", ~space, ' '));
        BOOST_TEST(!test(L" ", ~space, L' '));

        BOOST_TEST(test("x", ~space, 'x'));
        BOOST_TEST(test(L"x", ~space, L'x'));
    }

    {
        using namespace boost::spirit::standard_wide;

        BOOST_TEST(test(" ", space));
        BOOST_TEST(test(L" ", space));
        BOOST_TEST(!test("\t", space));
        BOOST_TEST(!test(L"\t", space));

        BOOST_TEST(test(" ", space, ' '));
        BOOST_TEST(test(L" ", space, L' '));
        BOOST_TEST(test("\t", space, '\t'));
        BOOST_TEST(test(L"\t", space, L'\t'));

        BOOST_TEST(!test("", space, 'x'));
        BOOST_TEST(!test(L"", space, L'x'));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test_delimited("x ", 'x', ' '));
        BOOST_TEST(test_delimited(L"x ", L'x', L' '));
        BOOST_TEST(!test_delimited("x ", 'y', ' '));
        BOOST_TEST(!test_delimited(L"x ", L'y', L' '));

        BOOST_TEST(test_delimited("x ", 'x', ' '));
        BOOST_TEST(test_delimited(L"x ", L'x', L' '));
        BOOST_TEST(!test_delimited("x ", 'y', ' '));
        BOOST_TEST(!test_delimited(L"x ", L'y', L' '));

        BOOST_TEST(test_delimited("x ", char_, 'x', ' '));
        BOOST_TEST(test_delimited(L"x ", char_, L'x', L' '));
        BOOST_TEST(!test_delimited("x ", char_, 'y', ' '));
        BOOST_TEST(!test_delimited(L"x ", char_, L'y', L' '));

        BOOST_TEST(test_delimited("x ", char_('x'), ' '));
        BOOST_TEST(!test_delimited("x ", char_('y'), ' '));

        BOOST_TEST(test_delimited("x ", char_('x'), 'x', ' '));
        BOOST_TEST(!test_delimited("", char_('y'), 'x', ' '));

        BOOST_TEST(test_delimited("x ", char_("x"), ' '));

#if defined(KARMA_FAIL_COMPILATION)
        BOOST_TEST(test_delimited("x ", char_, ' '));   // anychar without a parameter doesn't make any sense
#endif
    }

    {   // pre-delimiting
        {
            std::string generated;
            std::back_insert_iterator<std::string> it(generated);
            BOOST_TEST(karma::generate_delimited(it, '_', '^'
              , karma::delimit_flag::predelimit));
            BOOST_TEST(generated == "^_^");
        }
        {
            using namespace boost::spirit::standard_wide;
            std::basic_string<wchar_t> generated;
            std::back_insert_iterator<std::basic_string<wchar_t> > it(generated);
            BOOST_TEST(karma::generate_delimited(it, char_, L'.'
              , karma::delimit_flag::predelimit, L'x'));
            BOOST_TEST(generated == L".x.");
        }
    }

    // action tests
    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("x", char_[_1 = val('x')]));
        BOOST_TEST(!test("x", char_[_1 = val('y')]));
    }

// we support Phoenix attributes only starting with V2.2
#if SPIRIT_VERSION >= 0x2020
    // yes, we can use phoenix expressions as attributes as well
    // but only if we include karma_phoenix_attributes.hpp
    {
        namespace ascii = boost::spirit::ascii;
        namespace phoenix = boost::phoenix;

        BOOST_TEST(test("x", ascii::char_, phoenix::val('x')));

        char c = 'x';
        BOOST_TEST(test("x", ascii::char_, phoenix::ref(c)));
        BOOST_TEST(test("y", ascii::char_, ++phoenix::ref(c)));
    }
#endif

    {
        namespace ascii = boost::spirit::ascii;
        namespace wide = boost::spirit::standard_wide;

        boost::optional<char> v ('x');
        boost::optional<wchar_t> w (L'x');

        BOOST_TEST(test("x", ascii::char_, v));
        BOOST_TEST(test(L"x", wide::char_, w));
        BOOST_TEST(test("x", ascii::char_('x'), v));
        BOOST_TEST(test(L"x", wide::char_(L'x'), w));
        BOOST_TEST(!test("", ascii::char_('y'), v));
        BOOST_TEST(!test(L"", wide::char_(L'y'), w));
    }

    return boost::report_errors();
}
