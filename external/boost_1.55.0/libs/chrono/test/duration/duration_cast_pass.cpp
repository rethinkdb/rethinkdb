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
#include <boost/type_traits.hpp>
#include <boost/detail/lightweight_test.hpp>
#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

#ifdef BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif


template <class ToDuration, class FromDuration>
void
test(const FromDuration& f, const ToDuration& d)
{
//~ #if defined(BOOST_NO_CXX11_DECLTYPE)
    //~ typedef BOOST_TYPEOF_TPL(boost::chrono::duration_cast<ToDuration>(f)) R;
//~ #else
    //~ typedef decltype(boost::chrono::duration_cast<ToDuration>(f)) R;
//~ #endif
    //~ BOOST_CHRONO_STATIC_ASSERT((boost::is_same<R, ToDuration>::value), NOTHING, (R, ToDuration));
    BOOST_TEST(boost::chrono::duration_cast<ToDuration>(f) == d);
}

int main()
{
    test(boost::chrono::milliseconds(7265000), boost::chrono::hours(2));
    test(boost::chrono::milliseconds(7265000), boost::chrono::minutes(121));
    test(boost::chrono::milliseconds(7265000), boost::chrono::seconds(7265));
    test(boost::chrono::milliseconds(7265000), boost::chrono::milliseconds(7265000));
    test(boost::chrono::milliseconds(7265000), boost::chrono::microseconds(7265000000LL));
    test(boost::chrono::milliseconds(7265000), boost::chrono::nanoseconds(7265000000000LL));
    test(boost::chrono::milliseconds(7265000),
         boost::chrono::duration<double, boost::ratio<3600> >(7265./3600));
    test(boost::chrono::duration<int, boost::ratio<2, 3> >(9),
         boost::chrono::duration<int, boost::ratio<3, 5> >(10));
    {
      BOOST_CONSTEXPR boost::chrono::hours h = boost::chrono::duration_cast<boost::chrono::hours>(boost::chrono::milliseconds(7265000));
      BOOST_CONSTEXPR_ASSERT(h.count() == 2);
    }
    return boost::report_errors();
}
