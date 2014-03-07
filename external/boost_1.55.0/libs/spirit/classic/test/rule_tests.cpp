/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace std;

//#define BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT 3

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_stored_rule.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Rule tests
//
///////////////////////////////////////////////////////////////////////////////
void
aliasing_tests()
{
    rule<>  a = ch_p('a');
    rule<>  b = ch_p('b');
    rule<>  c = ch_p('c');

    cout << "sizeof(rule<>): " << sizeof(rule<>) << endl;

    BOOST_SPIRIT_DEBUG_RULE(a);
    BOOST_SPIRIT_DEBUG_RULE(b);
    BOOST_SPIRIT_DEBUG_RULE(c);

    rule<>  start;
    BOOST_SPIRIT_DEBUG_RULE(start);

    rule<>  d;
    d = start;  // aliasing

    parse_info<char const*> pi;

    start = *(a | b | c);
    pi = parse("abcabcacb", d);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    start   = (a | b) >> (start | b);
    pi = parse("aaaabababaaabbb", d);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);
}

void
rule_template_param_tests()
{
    // test that rules can be issued its template params in any order:

    rule<> rx1;
    rule<scanner<> > rx2;
    rule<scanner<>, parser_context<> > rx3;
    rule<scanner<>, parser_context<>, parser_address_tag> rx4;

    rule<parser_context<> > rx5;
    rule<parser_context<>, parser_address_tag> rx6;
    rule<parser_context<>, parser_address_tag, scanner<> > rx7;

    rule<parser_address_tag> rx8;
    rule<parser_address_tag, scanner<> > rx9;
    rule<parser_address_tag, scanner<>, parser_context<> > rx10;

    rule<parser_address_tag, parser_context<> > rx11;
    rule<parser_address_tag, parser_context<>, scanner<> > rx12;

    rule<parser_context<>, scanner<> > rx13;
    rule<parser_context<>, scanner<>, parser_address_tag> rx14;
}

struct my_grammar : public grammar<my_grammar>
{
    template <typename ScannerT>
    struct definition
    {
        definition(my_grammar const& /*self*/)
        {
            r = lower_p;
            rr = +(lexeme_d[r] >> as_lower_d[r] >> r);
        }

        typedef scanner_list<
            ScannerT
          , typename lexeme_scanner<ScannerT>::type
          , typename as_lower_scanner<ScannerT>::type
        > scanners;

        rule<scanners> r;
        rule<ScannerT> rr;
        rule<ScannerT> const& start() const { return rr; }
    };
};

void
rule_2_or_more_scanners_tests()
{
    { // 2 scanners
        typedef scanner_list<scanner<>, phrase_scanner_t> scanners;

        rule<scanners>  r = +anychar_p;
        BOOST_TEST(parse("abcdefghijk", r).full);
        BOOST_TEST(parse("a b c d e f g h i j k", r, space_p).full);
    }

    { // 3 scanners
        my_grammar g;
        BOOST_TEST(parse("abcdef aBc d e f aBc d E f", g, space_p).full);
    }
}

void
rule_basic_tests()
{
    rule<>  a = ch_p('a');
    rule<>  b = ch_p('b');
    rule<>  c = ch_p('c');

    BOOST_SPIRIT_DEBUG_RULE(a);
    BOOST_SPIRIT_DEBUG_RULE(b);
    BOOST_SPIRIT_DEBUG_RULE(c);

    parse_info<char const*> pi;

    rule<>  start = *(a | b | c);

    BOOST_SPIRIT_DEBUG_RULE(start);

    pi = parse("abcabcacb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    start   = (a | b) >> (start | b);
    pi = parse("aaaabababaaabbb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("aaaabababaaabba", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 14);

    rule<> r = anychar_p;
    r.copy(); // copy test (compile only)
}

void
stored_rule_basic_tests()
{
    stored_rule<>  a = ch_p('a');
    stored_rule<>  b = ch_p('b');
    stored_rule<>  c = ch_p('c');

    BOOST_SPIRIT_DEBUG_RULE(a);
    BOOST_SPIRIT_DEBUG_RULE(b);
    BOOST_SPIRIT_DEBUG_RULE(c);

    parse_info<char const*> pi;

    stored_rule<>  start = *(a | b | c);

    BOOST_SPIRIT_DEBUG_RULE(start);

    pi = parse("abcabcacb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    start   = (a | b) >> (start | b);
    pi = parse("aaaabababaaabbb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("aaaabababaaabba", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 14);
}

void
stored_rule_dynamic_tests()
{
    rule<>  a = ch_p('a');
    rule<>  b = ch_p('b');
    rule<>  c = ch_p('c');

    BOOST_SPIRIT_DEBUG_RULE(a);
    BOOST_SPIRIT_DEBUG_RULE(b);
    BOOST_SPIRIT_DEBUG_RULE(c);

    parse_info<char const*> pi;

    // The FF is the dynamic equivalent of start = *(a | b | c);
    stored_rule<>  start = a;
    start = start.copy() | b;
    start = start.copy() | c;
    start = *(start.copy());

    cout << "sizeof(stored_rule<>): " << sizeof(stored_rule<>) << endl;

    BOOST_SPIRIT_DEBUG_RULE(start);

    pi = parse("abcabcacb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    // The FF is the dynamic equivalent of start = (a | b) >> (start | b);
    start = b;
    start = a | start.copy();
    start = start.copy() >> (start | b);

    pi = parse("aaaabababaaabbb", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("aaaabababaaabba", start);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 14);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    rule_basic_tests();
    aliasing_tests();
    rule_template_param_tests();
    rule_2_or_more_scanners_tests();
    stored_rule_basic_tests();
    stored_rule_dynamic_tests();

    return boost::report_errors();
}

