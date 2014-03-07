/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <string>

using namespace std;

#include <boost/spirit/include/classic_core.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Match tests
//
///////////////////////////////////////////////////////////////////////////////
struct X {};
struct Y { Y(int) {} }; // not default constructible
struct Z { Z(double n):n(n){} double n; }; // implicitly convertible from double

void
match_tests()
{
    match<> m0;
    BOOST_TEST(!m0.has_valid_attribute());

    match<int> m1(m0);
    m1.value(123);
    BOOST_TEST(m1.has_valid_attribute());
    BOOST_TEST(m1.value() == 123);

    match<double> m2(m1);
    BOOST_TEST(m2.has_valid_attribute());
    BOOST_TEST(m1.value() == int(m2.value()));
    m2.value(456);

    m0 = m0; // match<nil> = match<nil>
    m0 = m1; // match<nil> = match<int>
    m0 = m2; // match<nil> = match<double>
    m1 = m0; // match<int> = match<nil>
    BOOST_TEST(!m1);
    BOOST_TEST(!m1.has_valid_attribute());

    m1 = m1; // match<int> = match<int>
    m1.value(int(m2.value()));
    BOOST_TEST(m1.has_valid_attribute());
    BOOST_TEST(m1.value() == int(m2.value()));

    m2.value(123.456);
    match<Z> mz(m2); // copy from match<double>
    mz = m2; // assign from match<double>
    BOOST_TEST(mz.value().n == 123.456);

    m1.value(123);
    m2 = m0;
    BOOST_TEST(!m2);
    BOOST_TEST(!m2.has_valid_attribute());

    m2 = m1; // match<double> = match<int>
    BOOST_TEST(m2.has_valid_attribute());
    BOOST_TEST(m1.value() == int(m2.value()));
    m2 = m2; // match<double> = match<double>

    cout << "sizeof(int) == " << sizeof(int) << '\n';
    cout << "sizeof(match<>) == " << sizeof(m0) << '\n';
    cout << "sizeof(match<int>) == " << sizeof(m1) << '\n';
    cout << "sizeof(match<double>) == " << sizeof(m2) << '\n';

    match<int&> mr;
    BOOST_TEST(!mr.has_valid_attribute());

    match<int&> mrr(4);
    BOOST_TEST(!mrr.has_valid_attribute());

    int ri = 3;
    match<int&> mr2(4, ri);
    BOOST_TEST(mr2.has_valid_attribute());
    mr = mr2;
    BOOST_TEST(mr.has_valid_attribute());

    match<int&> mr3(mrr);
    BOOST_TEST(!mr3.has_valid_attribute());
    mr2 = mr3;
    BOOST_TEST(!mr2.has_valid_attribute());

    match<X> mx;
    m1 = mx;
    m0 = mx;
    BOOST_TEST(!mx.has_valid_attribute());
    BOOST_TEST(!m0.has_valid_attribute());
    BOOST_TEST(!m1.has_valid_attribute());

    match<Y> my;
    BOOST_TEST(!my.has_valid_attribute());

    match<std::string> ms;
    BOOST_TEST(!ms.has_valid_attribute());
    ms.value("Kimpo Ponchwayla");
    BOOST_TEST(ms.has_valid_attribute());
    BOOST_TEST(ms.value() == "Kimpo Ponchwayla");
    ms = match<>();
    BOOST_TEST(!ms.has_valid_attribute());

    // let's try a match with a reference:
    int i;
    match<int&> mr4(4, i);
    BOOST_TEST(mr4.has_valid_attribute());
    mr4.value(3);
    BOOST_TEST(mr4.value() == 3);
    BOOST_TEST(i == 3);
    (void)i;

    int x = 456;
    match<int&> mref(1, x);
    BOOST_TEST(mref.value() == 456);
    mref.value(123);
    BOOST_TEST(mref.value() == 123);
    x = mref.value();
    BOOST_TEST(x == 123);
    mref.value() = 986;
    BOOST_TEST(x == 986);
    
    std::string s("hello");
    match<int> mint(1, x);
    BOOST_TEST(mint.value() == x);
    match<std::string> mstr(1, s);
    BOOST_TEST(mstr.value() == "hello");
    mstr = mint;
    mint = mstr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Match Policy tests
//
///////////////////////////////////////////////////////////////////////////////
void
match_policy_tests()
{
    match<>         m0;
    match<int>      m1;
    match<double>   m2;
    match_policy    mp;

    m0 = mp.no_match();     BOOST_TEST(!m0);
    m1 = mp.no_match();     BOOST_TEST(!m1);
    m0 = mp.empty_match();  BOOST_TEST(!!m0);
    m2 = mp.empty_match();  BOOST_TEST(!!m2);

    m1 = mp.create_match(5, 100, 0, 0);
    m2 = mp.create_match(5, 10.5, 0, 0);

    mp.concat_match(m1, m2);
    BOOST_TEST(m1.length() == 10);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    match_tests();
    match_policy_tests();
    return boost::report_errors();
}

