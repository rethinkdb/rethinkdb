//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/chrono/duration.hpp>

#include <boost/detail/lightweight_test.hpp>
#ifdef BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif

int main()
{

  // UNARY PLUS
  {
    boost::chrono::minutes m(3);
    boost::chrono::minutes m2 = +m;
    BOOST_TEST(m.count() == m2.count());
  }
  {
    BOOST_CONSTEXPR boost::chrono::minutes m(3);
    BOOST_CONSTEXPR boost::chrono::minutes m2(+m);
    BOOST_CONSTEXPR_ASSERT(m.count() == m2.count());
  }

  // UNARY MINUS
  {
    boost::chrono::minutes m(3);
    boost::chrono::minutes m2 = -m;
    BOOST_TEST(m2.count() == -m.count());
  }
  {
    BOOST_CONSTEXPR boost::chrono::minutes m(3);
    BOOST_CONSTEXPR boost::chrono::minutes m2 = -m;
    BOOST_CONSTEXPR_ASSERT(m2.count() == -m.count());
  }
  // PRE INCREMENT
  {
    boost::chrono::hours h(3);
    boost::chrono::hours& href = ++h;
    BOOST_TEST(&href == &h);
    BOOST_TEST(h.count() == 4);
  }
  // POST INCREMENT
  {
    boost::chrono::hours h(3);
    boost::chrono::hours h2 = h++;
    BOOST_TEST(h.count() == 4);
    BOOST_TEST(h2.count() == 3);
  }
  // PRE DECREMENT
  {
    boost::chrono::hours h(3);
    boost::chrono::hours& href = --h;
    BOOST_TEST(&href == &h);
    BOOST_TEST(h.count() == 2);
  }
  // POST DECREMENT
  {
    boost::chrono::hours h(3);
    boost::chrono::hours h2 = h--;
    BOOST_TEST(h.count() == 2);
    BOOST_TEST(h2.count() == 3);
  }
  // PLUS ASSIGN
  {
    boost::chrono::seconds s(3);
    s += boost::chrono::seconds(2);
    BOOST_TEST(s.count() == 5);
    s += boost::chrono::minutes(2);
    BOOST_TEST(s.count() == 125);
  }
  // MINUS ASSIGN
  {
    boost::chrono::seconds s(3);
    s -= boost::chrono::seconds(2);
    BOOST_TEST(s.count() == 1);
    s -= boost::chrono::minutes(2);
    BOOST_TEST(s.count() == -119);
  }
  // TIMES ASSIGN
  {
    boost::chrono::nanoseconds ns(3);
    ns *= 5;
    BOOST_TEST(ns.count() == 15);
  }
  // DIVIDE ASSIGN
  {
    boost::chrono::nanoseconds ns(15);
    ns /= 5;
    BOOST_TEST(ns.count() == 3);
  }
  // MODULUS ASSIGN duration
  {
    boost::chrono::microseconds us(11);
    boost::chrono::microseconds us2(3);
    us %= us2;
    BOOST_TEST(us.count() == 2);
    us %= boost::chrono::milliseconds(3);
    BOOST_TEST(us.count() == 2);
  }
  // MODULUS ASSIGN Rep
  {
    boost::chrono::microseconds us(11);
    us %= 3;
    BOOST_TEST(us.count() == 2);
  }
  // PLUS
  {
    boost::chrono::seconds s1(3);
    boost::chrono::seconds s2(5);
    boost::chrono::seconds r = s1 + s2;
    BOOST_TEST(r.count() == 8);
  }
  {
    BOOST_CONSTEXPR boost::chrono::seconds s1(3);
    BOOST_CONSTEXPR boost::chrono::seconds s2(5);
    BOOST_CONSTEXPR boost::chrono::seconds r = s1 + s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 8);
  }
  {
    boost::chrono::seconds s1(3);
    boost::chrono::microseconds s2(5);
    boost::chrono::microseconds r = s1 + s2;
    BOOST_TEST(r.count() == 3000005);
  }
  {
    BOOST_CONSTEXPR boost::chrono::seconds s1(3);
    BOOST_CONSTEXPR boost::chrono::microseconds s2(5);
    BOOST_CONSTEXPR boost::chrono::microseconds r = s1 + s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 3000005);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 + s2;
    BOOST_TEST(r.count() == 75);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 + s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 75);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    boost::chrono::duration<double, boost::ratio<1, 15> > r = s1 + s2;
    BOOST_TEST(r.count() == 75);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<1, 15> > r = s1 + s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 75);
  }

  // MINUS
  {
    boost::chrono::seconds s1(3);
    boost::chrono::seconds s2(5);
    boost::chrono::seconds r = s1 - s2;
    BOOST_TEST(r.count() == -2);
  }
  {
    BOOST_CONSTEXPR boost::chrono::seconds s1(3);
    BOOST_CONSTEXPR boost::chrono::seconds s2(5);
    BOOST_CONSTEXPR boost::chrono::seconds r = s1 - s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == -2);
  }
  {
    boost::chrono::seconds s1(3);
    boost::chrono::microseconds s2(5);
    boost::chrono::microseconds r = s1 - s2;
    BOOST_TEST(r.count() == 2999995);
  }
  {
    BOOST_CONSTEXPR boost::chrono::seconds s1(3);
    BOOST_CONSTEXPR boost::chrono::microseconds s2(5);
    BOOST_CONSTEXPR boost::chrono::microseconds r = s1 - s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 2999995);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 - s2;
    BOOST_TEST(r.count() == -15);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 - s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == -15);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    boost::chrono::duration<double, boost::ratio<1, 15> > r = s1 - s2;
    BOOST_TEST(r.count() == -15);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(3);
    BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<1, 15> > r = s1 - s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == -15);
  }

  // TIMES rep
  {
    boost::chrono::nanoseconds ns(3);
    boost::chrono::nanoseconds ns2 = ns * 5;
    BOOST_TEST(ns2.count() == 15);
    boost::chrono::nanoseconds ns3 = 6 * ns2;
    BOOST_TEST(ns3.count() == 90);
  }
  {
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns(3);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2 = ns * 5;
    BOOST_CONSTEXPR_ASSERT(ns2.count() == 15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns3 = 6 * ns2;
    BOOST_CONSTEXPR_ASSERT(ns3.count() == 90);
  }

  // DIVIDE duration
  {
    boost::chrono::nanoseconds ns1(15);
    boost::chrono::nanoseconds ns2(5);
    BOOST_TEST(ns1 / ns2 == 3);
  }
  {
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns1(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2(5);
    BOOST_CONSTEXPR_ASSERT(ns1 / ns2 == 3);
  }
  {
    boost::chrono::microseconds us1(15);
    boost::chrono::nanoseconds ns2(5);
    BOOST_TEST(us1 / ns2 == 3000);
  }
  {
    BOOST_CONSTEXPR boost::chrono::microseconds us1(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2(5);
    BOOST_CONSTEXPR_ASSERT(us1 / ns2 == 3000);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(30);
    boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    BOOST_TEST(s1 / s2 == 6);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(30);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR_ASSERT(s1 / s2 == 6);
  }
  {
    boost::chrono::duration<int, boost::ratio<2, 3> > s1(30);
    boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    BOOST_TEST(s1 / s2 == 20. / 3);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s1(30);
    BOOST_CONSTEXPR boost::chrono::duration<double, boost::ratio<3, 5> > s2(5);
    BOOST_CONSTEXPR_ASSERT(s1 / s2 == 20. / 3);
  }
  // DIVIDE rep
  {
    boost::chrono::nanoseconds ns(15);
    boost::chrono::nanoseconds ns2 = ns / 5;
    BOOST_TEST(ns2.count() == 3);
  }
  {
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2 = ns / 5;
    BOOST_CONSTEXPR_ASSERT(ns2.count() == 3);
  }

  // MODULUS duration

  {
    boost::chrono::nanoseconds ns1(15);
    boost::chrono::nanoseconds ns2(6);
    boost::chrono::nanoseconds r = ns1 % ns2;
    BOOST_TEST(r.count() == 3);
  }
  {
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns1(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2(6);
    BOOST_CONSTEXPR boost::chrono::nanoseconds r = ns1 % ns2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 3);
  }
  {
    boost::chrono::microseconds us1(15);
    boost::chrono::nanoseconds ns2(28);
    boost::chrono::nanoseconds r = us1 % ns2;
    BOOST_TEST(r.count() == 20);
  }
  {
    BOOST_CONSTEXPR boost::chrono::microseconds us1(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2(28);
    BOOST_CONSTEXPR boost::chrono::nanoseconds r = us1 % ns2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 20);
  }
  {
    boost::chrono::duration<int, boost::ratio<3, 5> > s1(6);
    boost::chrono::duration<int, boost::ratio<2, 3> > s2(3);
    boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 % s2;
    BOOST_TEST(r.count() == 24);
  }
  {
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<3, 5> > s1(6);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<2, 3> > s2(3);
    BOOST_CONSTEXPR boost::chrono::duration<int, boost::ratio<1, 15> > r = s1 % s2;
    BOOST_CONSTEXPR_ASSERT(r.count() == 24);
  }
  // MODULUS rep
  {
    boost::chrono::nanoseconds ns(15);
    boost::chrono::nanoseconds ns2 = ns % 6;
    BOOST_TEST(ns2.count() == 3);
  }
  {
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns(15);
    BOOST_CONSTEXPR boost::chrono::nanoseconds ns2 = ns % 6;
    BOOST_CONSTEXPR_ASSERT(ns2.count() == 3);
  }

  return boost::report_errors();
}
