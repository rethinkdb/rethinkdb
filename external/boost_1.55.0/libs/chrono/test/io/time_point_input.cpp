//  Copyright 2011 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/chrono/chrono_io.hpp>
#include <sstream>
#include <boost/detail/lightweight_test.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>

template <typename Clock, typename D>
void test_good(std::string str, D res)
{
  std::istringstream in(str + boost::chrono::clock_string<Clock, char>::since());
  boost::chrono::time_point<Clock, D> tp;
  in >> tp;
  BOOST_TEST(in.eof());
  BOOST_TEST(!in.fail());
  BOOST_TEST( (tp == boost::chrono::time_point<Clock, D>(res)));
}

#if BOOST_CHRONO_VERSION >= 2
template <typename D>
void test_good_system_clock(std::string str, D res)
{
  typedef boost::chrono::system_clock Clock;

  std::istringstream in(str);
  boost::chrono::time_point<Clock, D> tp;
  in >> tp;
  BOOST_TEST(in.eof());
  BOOST_TEST(!in.fail());
  std::cout << "Input=    " << str << std::endl;
  std::cout << "Expected= " << boost::chrono::time_point<Clock, D>(res) << std::endl;
  std::cout << "Obtained= " << tp << std::endl;
  std::cout << "Expected= " << boost::chrono::duration_cast<boost::chrono::nanoseconds>(boost::chrono::time_point<Clock, D>(res).time_since_epoch()).count() << std::endl;
  std::cout << "Obtained= " << boost::chrono::duration_cast<boost::chrono::nanoseconds>(tp.time_since_epoch()).count() << std::endl;
  BOOST_TEST( (tp == boost::chrono::time_point<Clock, D>(res)));
}

template <typename D>
void test_good_utc_fmt_system_clock(std::string str, std::string fmt, D res)
{
  typedef boost::chrono::system_clock Clock;

  std::istringstream in(str);
  boost::chrono::time_point<Clock, D> tp;
  in >> time_fmt(boost::chrono::timezone::utc, fmt);
  in >> tp;
  BOOST_TEST(in.eof());
  BOOST_TEST(!in.fail());
  std::cout << "Input=    " << str << std::endl;
  std::cout << "Expected= " << boost::chrono::time_point<Clock, D>(res) << std::endl;
  std::cout << "Obtained= " << tp << std::endl;
  std::cout << "Expected= " << boost::chrono::duration_cast<boost::chrono::nanoseconds>(boost::chrono::time_point<Clock, D>(res).time_since_epoch()).count() << std::endl;
  std::cout << "Obtained= " << boost::chrono::duration_cast<boost::chrono::nanoseconds>(tp.time_since_epoch()).count() << std::endl;
  BOOST_TEST_EQ( tp , (boost::chrono::time_point<Clock, D>(res)));
}
#endif
template <typename Clock, typename D>
void test_fail(const char* str, D)
{
  std::istringstream in(str + boost::chrono::clock_string<Clock, char>::since());
  boost::chrono::time_point<Clock, D> tp;
  in >> tp;
  BOOST_TEST(in.fail());
  BOOST_TEST( (tp == boost::chrono::time_point<Clock, D>()));
}

template <typename Clock, typename D>
void test_fail_no_epoch(const char* str, D )
{
  std::istringstream in(str);
  boost::chrono::time_point<Clock, D> tp;
  in >> tp;
  BOOST_TEST(in.fail());
  BOOST_TEST( (tp == boost::chrono::time_point<Clock, D>()));
}

template <typename Clock, typename D>
void test_fail_epoch(const char* str, D)
{
  std::istringstream in(str);
  boost::chrono::time_point<Clock, D> tp;
  in >> tp;
  BOOST_TEST(in.fail());
  BOOST_TEST( (tp == boost::chrono::time_point<Clock, D>()));
}

template <typename Clock>
void check_all()
{
  using namespace boost::chrono;
  using namespace boost;

  test_good<Clock> ("5000 hours", hours(5000));
  test_good<Clock> ("5000 minutes", minutes(5000));
  test_good<Clock> ("5000 seconds", seconds(5000));
  test_good<Clock> ("1 seconds", seconds(1));
  test_good<Clock> ("1 second", seconds(1));
  test_good<Clock> ("-1 seconds", seconds(-1));
  test_good<Clock> ("0 second", seconds(0));
  test_good<Clock> ("0 seconds", seconds(0));
  test_good<Clock> ("5000 milliseconds", milliseconds(5000));
  test_good<Clock> ("5000 microseconds", microseconds(5000));
  test_good<Clock> ("5000 nanoseconds", nanoseconds(5000));
  test_good<Clock> ("5000 deciseconds", duration<boost::int_least64_t, deci> (5000));
  test_good<Clock> ("5000 [1/30]seconds", duration<boost::int_least64_t, ratio<1, 30> > (5000));

  test_good<Clock> ("5000 h", hours(5000));
#if BOOST_CHRONO_VERSION >= 2
  test_good<Clock>("5000 min", minutes(5000));
#else
  test_good<Clock> ("5000 m", minutes(5000));
#endif
  test_good<Clock> ("5000 s", seconds(5000));
  test_good<Clock> ("5000 ms", milliseconds(5000));
  test_good<Clock> ("5000 ns", nanoseconds(5000));
  test_good<Clock> ("5000 ds", duration<boost::int_least64_t, deci> (5000));
  test_good<Clock> ("5000 [1/30]s", duration<boost::int_least64_t, ratio<1, 30> > (5000));

  test_good<Clock> ("5000 milliseconds", seconds(5));
  test_good<Clock> ("5 milliseconds", nanoseconds(5000000));
  test_good<Clock> ("4000 ms", seconds(4));
  test_fail<Clock> ("3001 ms", seconds(3));
  test_fail_epoch<Clock> ("3001 ms", seconds(3));
  test_fail_epoch<Clock> ("3001 ms since", seconds(3));

}

#if BOOST_CHRONO_VERSION >= 2
void check_all_system_clock()
{
  using namespace boost::chrono;
  using namespace boost;

  test_good_system_clock ("1970-01-01 02:00:00.000000 +0000", hours(2));
  test_good_system_clock ("1970-07-28 08:00:00.000000 +0000", hours(5000));
  test_good_system_clock ("1970-01-04 11:20:00.000000 +0000", minutes(5000));
  test_good_system_clock ("1970-01-01 01:23:20.000000 +0000", seconds(5000));
  test_good_system_clock ("1970-01-01 00:00:01.000000 +0000", seconds(1));
  test_good_system_clock ("1970-01-01 00:00:01.000000 +0000", seconds(1));
  test_good_system_clock ("1969-12-31 23:59:59.000000 +0000", seconds(-1));
  test_good_system_clock ("1970-01-01 00:00:00.000000 +0000", seconds(0));
  test_good_system_clock ("1970-01-01 00:00:00.000000 +0000", seconds(0));
  test_good_system_clock ("1970-01-01 00:00:05.000000 +0000", milliseconds(5000));
  test_good_system_clock ("1970-01-01 00:00:00.005000 +0000", microseconds(5000));
  test_good_system_clock ("1970-01-01 00:00:00.000005 +0000", nanoseconds(5000));
  test_good_system_clock ("1970-01-01 00:08:20.000000 +0000", duration<boost::int_least64_t, deci> (5000));
  test_good_system_clock ("1970-01-01 00:02:46.666667 +0000", duration<boost::int_least64_t, ratio<1, 30> > (5000));

  test_good_utc_fmt_system_clock ("1970-01-01 02:00:00", "%Y-%m-%d %H:%M:%S", hours(2));
  test_good_utc_fmt_system_clock ("1970-01-01 02:00:00", "%F %H:%M:%S", hours(2));
  test_good_utc_fmt_system_clock ("1970-01-01 02", "%Y-%m-%d %H", hours(2));
  test_good_utc_fmt_system_clock ("1970-01-01 02", "%F %H", hours(2));
  test_good_utc_fmt_system_clock ("1970-01-01 02:00:00", "%Y-%m-%d %T", hours(2));
  test_good_utc_fmt_system_clock ("1970-01-01 02:00", "%Y-%m-%d %R", hours(2));
  test_good_utc_fmt_system_clock ("% 1970-01-01 02:00", "%% %Y-%m-%d %R", hours(2));
  //test_good_utc_fmt_system_clock ("1970-01-01 02:00 Thursday January", "%Y-%m-%d %R %A %B", hours(2));


//  test_fail<Clock> ("3001 ms", seconds(3));
//  test_fail_epoch<Clock> ("3001 ms", seconds(3));
//  test_fail_epoch<Clock> ("3001 ms since", seconds(3));

}
#endif
int main()
{
  std::cout << "high_resolution_clock=" << std::endl;
  check_all<boost::chrono::high_resolution_clock> ();
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
  std::cout << "steady_clock=" << std::endl;
  check_all<boost::chrono::steady_clock> ();
#endif
  std::cout << "system_clock=" << std::endl;
#if BOOST_CHRONO_VERSION >= 2  && defined BOOST_CHRONO_PROVIDES_DATE_IO_FOR_SYSTEM_CLOCK_TIME_POINT
  check_all_system_clock();
#else
  check_all<boost::chrono::system_clock> ();
#endif
#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
  std::cout << "thread_clock="<< std::endl;
  check_all<boost::chrono::thread_clock>();
#endif

#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
  std::cout << "process_real_cpu_clock=" << std::endl;
  check_all<boost::chrono::process_real_cpu_clock> ();
  std::cout << "process_user_cpu_clock=" << std::endl;
  check_all<boost::chrono::process_user_cpu_clock> ();
  std::cout << "process_system_cpu_clock=" << std::endl;
  check_all<boost::chrono::process_system_cpu_clock> ();
  std::cout << "process_cpu_clock=" << std::endl;
  check_all<boost::chrono::process_cpu_clock> ();
#endif

  return boost::report_errors();

}

