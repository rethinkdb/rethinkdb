/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <string>

using namespace std;

#include "impl/string_length.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Directives tests
//
///////////////////////////////////////////////////////////////////////////////
void
directives_test1()
{
    char const* cpx = "H e l l o";
    char const* cpx_first = cpx;
    char const* cpx_last = cpx + test_impl::string_length(cpx);

    match<> hit;
    typedef skipper_iteration_policy<iteration_policy> iter_policy;
    scanner<char const*, scanner_policies<iter_policy> >
        scanx(cpx_first, cpx_last);

    hit = str_p("Hello").parse(scanx);
    BOOST_TEST(!hit);
    scanx.first = cpx;

    hit = chseq_p("Hello").parse(scanx);
    BOOST_TEST(!!hit);
    scanx.first = cpx;

    char const* cp = "Hello \n\tWorld";
    char const* cp_first = cp;
    char const* cp_last = cp + test_impl::string_length(cp);

    scanner<char const*, scanner_policies<iter_policy> >
        scan(cp_first, cp_last);

    hit = (+(alpha_p | punct_p)).parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    hit = (+(lexeme_d[+(alpha_p | '\'')])).parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    hit = (+(lexeme_d[lexeme_d[+anychar_p]])).parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    hit = (str_p("Hello") >> "World").parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    hit = as_lower_d[str_p("hello") >> "world"].parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    hit = (+(as_lower_d[as_lower_d[+lower_p | '\'']])).parse(scan);
    BOOST_TEST(!!hit);
    BOOST_TEST(scan.first == scan.last);
    scan.first = cp;

    char const* cpy = "123.456";
    char const* cpy_first = cpy;
    char const* cpy_last = cpy + test_impl::string_length(cpy);

    scanner<> scany(cpy_first, cpy_last);
    hit = longest_d[(+digit_p >> '.' >> +digit_p) | (+digit_p)].parse(scany);
    BOOST_TEST(!!hit);
    BOOST_TEST(scany.first == scany.last);
    scany.first = cpy;

    hit = shortest_d[(+digit_p >> '.' >> +digit_p) | (+digit_p)].parse(scany);
    BOOST_TEST(!!hit);
    BOOST_TEST(scany.first != scany.last);
    scany.first = cpy;

    char const* cpz = "razamanaz";
    char const* cpz_first = cpz;
    char const* cpz_last = cpz + test_impl::string_length(cpz);

    scanner<> scanz(cpz_first, cpz_last);
    hit = longest_d[str_p("raza") | "razaman" | "razamanaz"].parse(scanz);
    BOOST_TEST(!!hit);
    BOOST_TEST(scanz.first == scanz.last);
    scanz.first = cpz;

    hit = shortest_d[str_p("raza") | "razaman" | "razamanaz"].parse(scanz);
    BOOST_TEST(!!hit);
    BOOST_TEST(scanz.first == cpz+4);
    scanz.first = cpz;

//  bounds_d

    parse_info<> pr = parse("123", limit_d(0, 60)[int_p]);
    BOOST_TEST(!pr.hit);

    pr = parse("-2", limit_d(0, 60)[int_p]);
    BOOST_TEST(!pr.hit);

    pr = parse("60", limit_d(0, 60)[int_p]);
    BOOST_TEST(pr.hit);

    pr = parse("0", limit_d(0, 60)[int_p]);
    BOOST_TEST(pr.hit);

    pr = parse("-2", min_limit_d(0)[int_p]);
    BOOST_TEST(!pr.hit);

    pr = parse("-2", min_limit_d(-5)[int_p]);
    BOOST_TEST(pr.hit);

    pr = parse("101", max_limit_d(100)[int_p]);
    BOOST_TEST(!pr.hit);

    pr = parse("100", max_limit_d(100)[int_p]);
    BOOST_TEST(pr.hit);
}

struct identifier : public grammar<identifier>
{
    template <typename ScannerT>
    struct definition
    {
        definition(identifier const& /*self*/)
        {
            rr = +(alpha_p | '_');
            r = lexeme_d[rr];
        }

        rule<typename lexeme_scanner<ScannerT>::type> rr;
        rule<ScannerT> r;

        rule<ScannerT> const&
        start() const { return r; }
    };
};

void
directives_test2()
{
    //  Test that lexeme_d does not skip trailing spaces

    string str1, str2;
    identifier ident;

    parse("rock_n_roll never_dies ",

        ident[assign_a(str1)] >> ident[assign_a(str2)], space_p
    );

    cout << '*' << str1 << ',' << str2 << '*' << endl;


    BOOST_TEST(str1 == "rock_n_roll");
    BOOST_TEST(str2 == "never_dies");
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    directives_test1();
    directives_test2();
    return boost::report_errors();
}

