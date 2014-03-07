//  Copyright 2011 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/chrono/chrono_io.hpp>
#include <sstream>
#include <boost/detail/lightweight_test.hpp>

template<typename D>
void test_good_prefix(const char* str, D d)
{
  std::ostringstream out;
  out << d;
  BOOST_TEST(out.good());

  BOOST_TEST(out.str() == str);
}

template<typename D>
void test_good_symbol(const char* str, D d)
{
  std::ostringstream out;
#if BOOST_CHRONO_VERSION==2
  out << boost::chrono::duration_fmt(boost::chrono::duration_style::symbol) << d;
#else
  out << boost::chrono::duration_short  << d;
#endif
  BOOST_TEST(out.good());
  BOOST_TEST(out.str() == str);
}
#if BOOST_CHRONO_VERSION==2

template<typename D>
void test_good(const char* str, D d, boost::chrono::duration_style style)
{
  std::ostringstream out;

  out << boost::chrono::duration_fmt(style) << d;
  BOOST_TEST(out.good());
  BOOST_TEST(out.str() == str);
}

template<typename D>
void test_state_saver(const char* str, const char* str2, D d, boost::chrono::duration_style style)
{
  std::ostringstream out;
  {
    boost::chrono::duration_style_io_saver ios(out);
    out << boost::chrono::duration_fmt(style) << d;
    BOOST_TEST(out.good());
    BOOST_TEST(out.str() == str);
  }
  out << " " <<  d;
  BOOST_TEST(out.good());
  BOOST_TEST(out.str() == str2);
}

#endif

int main()
{
  using namespace boost::chrono;
  using namespace boost;

  test_good_prefix("5000 hours", hours(5000));
  test_good_prefix("5000 minutes", minutes(5000));
  test_good_prefix("5000 seconds", seconds(5000));
  test_good_prefix("0 seconds", seconds(0));
  test_good_prefix("1 second", seconds(1));
  test_good_prefix("-1 second", seconds(-1));
  test_good_prefix("5000 milliseconds", milliseconds(5000));
  test_good_prefix("5000 microseconds", microseconds(5000));
  test_good_prefix("5000 nanoseconds", nanoseconds(5000));
  test_good_prefix("5000 deciseconds", duration<boost::int_least64_t, deci> (5000));
  test_good_prefix("5000 [1/30]seconds", duration<boost::int_least64_t, ratio<1, 30> > (5000));

  test_good_symbol("5000 h", hours(5000));
#if BOOST_CHRONO_VERSION==2
  test_good_symbol("5000 min", minutes(5000));
#else
  test_good_symbol("5000 m", minutes(5000));
#endif
  test_good_symbol("5000 s", seconds(5000));
  test_good_symbol("5000 ms", milliseconds(5000));
  test_good_symbol("5000 ns", nanoseconds(5000));
  test_good_symbol("5000 ds", duration<boost::int_least64_t, deci> (5000));
  test_good_symbol("5000 [1/30]s", duration<boost::int_least64_t, ratio<1, 30> > (5000));

#if BOOST_CHRONO_VERSION==2
  test_good("5000 hours", hours(5000), duration_style::prefix);
  test_good("5000 h", hours(5000), duration_style::symbol);
  test_state_saver("5000 h", "5000 h 5000 hours", hours(5000), duration_style::symbol);
#endif

  return boost::report_errors();

}

