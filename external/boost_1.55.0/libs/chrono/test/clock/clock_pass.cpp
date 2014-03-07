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

#include <boost/type_traits/is_same.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <boost/system/system_error.hpp>
#include <boost/detail/lightweight_test.hpp>

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

template <typename Clock>
void check_clock_invariants()
{
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<typename Clock::rep, typename Clock::duration::rep>::value), NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<typename Clock::period, typename Clock::duration::period>::value), NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<typename Clock::duration, typename Clock::time_point::duration>::value), NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT(Clock::is_steady || !Clock::is_steady, NOTHING, ());
    // to be replaced by has static member bool is_steady
}

template <typename Clock>
void check_clock_now()
{
    typename Clock::time_point t1 = Clock::now();
    (void)t1;
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING

template <typename Clock>
void check_clock_now_ec()
{
    boost::system::error_code ec;
    typename Clock::time_point t1 = Clock::now(ec);
    (void)t1;
    BOOST_TEST(ec.value()==0);
}

template <typename Clock>
void check_clock_now_throws()
{
    typename Clock::time_point t1 = Clock::now(boost::throws());
    (void)t1;
}

#ifndef BOOST_NO_EXCEPTIONS

template <typename Clock>
void check_clock_now_err(int err)
{
    Clock::set_errno(err);
    try {
        typename Clock::time_point t1 = Clock::now();
    } catch (boost::system::system_error& ex) {
        BOOST_TEST(ex.code().value()==err);
//      BOOST_TEST(ex.code().category() == BOOST_CHRONO_SYSTEM_CATEGORY);
//      BOOST_TEST(std::string(ex.what()) == std::string("errored_clock"));
    }
    Clock::set_errno(0);
}
#endif

template <typename Clock>
void check_clock_now_ec_err(int err)
{
    Clock::set_errno(err);
    boost::system::error_code ec;
    typename Clock::time_point t1 = Clock::now(ec);
    BOOST_TEST(ec.value()==err);
//  BOOST_TEST(ec.category() == BOOST_CHRONO_SYSTEM_CATEGORY);
    Clock::set_errno(0);
}

#ifndef BOOST_NO_EXCEPTIONS
template <typename Clock>
void check_clock_now_throws_err(int err)
{
    Clock::set_errno(err);
    try {
        typename Clock::time_point t1 = Clock::now(boost::throws());
        BOOST_TEST(0&&"exception not thown");
    } catch (boost::system::system_error& ex) {
        BOOST_TEST(ex.code().value()==err);
//      BOOST_TEST(ex.code().category() == BOOST_CHRONO_SYSTEM_CATEGORY);
//      BOOST_TEST(std::string(ex.what()) == std::string("errored_clock"));
    }
    Clock::set_errno(0);
}
#endif
#endif

int main()
{
    check_clock_invariants<boost::chrono::high_resolution_clock>();
    check_clock_now<boost::chrono::high_resolution_clock>();

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    check_clock_invariants<boost::chrono::steady_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::steady_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::steady_clock>();
#endif

    check_clock_invariants<boost::chrono::system_clock>();
    BOOST_CHRONO_STATIC_ASSERT(!boost::chrono::system_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::system_clock>();
    {
        typedef boost::chrono::system_clock C;
        C::time_point t1 = C::from_time_t(C::to_time_t(C::now()));
        (void)t1;
    }
    {
        typedef boost::chrono::system_clock C;
        std::time_t t1 = C::to_time_t(C::now());
        (void)t1;

    }
    {
        BOOST_TEST((boost::chrono::system_clock::duration::min)() <
               boost::chrono::system_clock::duration::zero());

    }

#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
    check_clock_invariants<boost::chrono::thread_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::thread_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::thread_clock>();
#endif

#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
    check_clock_invariants<boost::chrono::process_real_cpu_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::process_real_cpu_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::process_real_cpu_clock>();

    check_clock_invariants<boost::chrono::process_user_cpu_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::process_user_cpu_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::process_user_cpu_clock>();

    check_clock_invariants<boost::chrono::process_system_cpu_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::process_system_cpu_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::process_system_cpu_clock>();

    check_clock_invariants<boost::chrono::process_cpu_clock>();
    BOOST_CHRONO_STATIC_ASSERT(boost::chrono::process_cpu_clock::is_steady, NOTHING, ());
    check_clock_now<boost::chrono::process_cpu_clock>();
#endif


#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
    check_clock_now_ec<boost::chrono::high_resolution_clock>();
    check_clock_now_throws<boost::chrono::high_resolution_clock>();

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    check_clock_now_ec<boost::chrono::steady_clock>();
    check_clock_now_throws<boost::chrono::steady_clock>();
#endif

    check_clock_now_ec<boost::chrono::system_clock>();
    check_clock_now_throws<boost::chrono::system_clock>();

#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
    check_clock_now_ec<boost::chrono::thread_clock>();
    check_clock_now_throws<boost::chrono::thread_clock>();
#endif

#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
    check_clock_now_ec<boost::chrono::process_real_cpu_clock>();
    check_clock_now_throws<boost::chrono::process_real_cpu_clock>();

    check_clock_now_ec<boost::chrono::process_user_cpu_clock>();
    check_clock_now_throws<boost::chrono::process_user_cpu_clock>();

    check_clock_now_ec<boost::chrono::process_system_cpu_clock>();
    check_clock_now_throws<boost::chrono::process_system_cpu_clock>();

    check_clock_now_ec<boost::chrono::process_cpu_clock>();
    check_clock_now_throws<boost::chrono::process_cpu_clock>();
#endif

#endif

    return boost::report_errors();
}
