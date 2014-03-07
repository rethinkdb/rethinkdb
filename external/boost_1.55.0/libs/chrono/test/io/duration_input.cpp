//  Distributed under the Boost Software License, Version 1.0.
//  Copyright 2011 Vicente J. Botet Escriba
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/chrono/chrono_io.hpp>
//#include <boost/chrono/io/duration_units.hpp>
#include <sstream>
#include <boost/detail/lightweight_test.hpp>


template<typename D>
void test_good(const char* str, D res)
{
  std::istringstream in(str);
  D d(0);
  in >> d;
  BOOST_TEST(in.eof());
  BOOST_TEST(!in.fail());
  BOOST_TEST(d == res);
  std::cout << str << " " << res << " " << d << std::endl;
}

template<typename DFail>
void test_fail(const char* str, DFail res)
{
  {
    std::istringstream in(str);
    DFail d = DFail::zero();
    in >> d;
    BOOST_TEST(in.fail());
    BOOST_TEST(d == DFail::zero());
    std::cout << str << " " << res << " " << d << std::endl;
  }
}

template<typename D>
void test_not_eof(const char* str, D res)
{
  {
    std::istringstream in(str);
    D d = D::zero();
    in >> d;
    BOOST_TEST(!in.eof());
    BOOST_TEST(d == res);
    std::cout << str << " " << res << " " << d << std::endl;
  }
}
int main()
{
  using namespace boost::chrono;
  using namespace boost;

  test_good("5000", 5000);

  std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
  test_good("5000 hours", hours(5000));
  std::cerr << __FILE__ << "[" << __LINE__ << "]"<< std::endl;
  test_good("5000 minutes", minutes(5000));
  test_good("5000 seconds", seconds(5000));
  test_fail("1.0 second", seconds(1));

  test_good("1.0 second", duration<float,ratio<1> >(1));
  /* BUG with DURATION_GET
../../../boost/math/common_factor_rt.hpp: In function 'RingType boost::math::detail::gcd_euclidean(RingType, RingType) [with RingType = long double]':
../../../boost/math/common_factor_rt.hpp:122:   instantiated from 'IntegerType boost::math::detail::gcd_integer(const IntegerType&, const IntegerType&) [with IntegerType = long double]'
../../../boost/math/common_factor_rt.hpp:240:   instantiated from 'T boost::math::detail::gcd_optimal_evaluator_helper_t<T, true, true>::operator()(const T&, const T&) [with T = long double]'
../../../boost/math/common_factor_rt.hpp:290:   instantiated from 'T boost::math::detail::gcd_optimal_evaluator<T>::operator()(const T&, const T&) [with T = long double]'
../../../boost/math/common_factor_rt.hpp:442:   instantiated from 'T boost::math::detail::gcd_optimal(const T&, const T&) [with T = long double]'
../../../boost/math/common_factor_rt.hpp:473:   instantiated from 'typename boost::math::gcd_evaluator<IntegerType>::result_type boost::math::gcd_evaluator<IntegerType>::operator()(const IntegerType&, const IntegerType&) const [with IntegerType = long double]'
../../../boost/math/common_factor_rt.hpp:505:   instantiated from 'IntegerType boost::math::gcd(const IntegerType&, const IntegerType&) [with IntegerType = long double]'
../../../boost/chrono/io/duration_get.hpp:239:   instantiated from 'InputIterator boost::chrono::duration_get<CharT, InputIterator>::get(InputIterator, InputIterator, std::ios_base&, std::_Ios_Iostate&, boost::chrono::duration<Rep2, Period2>&, const CharT*, const CharT*) const [with Rep = double, Period = boost::ratio<1l, 1l>, CharT = char, InputIterator = std::istreambuf_iterator<char, std::char_traits<char> >]'
../../../boost/chrono/io/duration_get.hpp:294:   instantiated from 'InputIterator boost::chrono::duration_get<CharT, InputIterator>::get(InputIterator, InputIterator, std::ios_base&, std::_Ios_Iostate&, boost::chrono::duration<Rep2, Period2>&) const [with Rep = double, Period = boost::ratio<1l, 1l>, CharT = char, InputIterator = std::istreambuf_iterator<char, std::char_traits<char> >]'
../../../boost/chrono/io/duration_io.hpp:593:   instantiated from 'std::basic_istream<_CharT, _Traits>& boost::chrono::operator>>(std::basic_istream<_CharT, _Traits>&, boost::chrono::duration<Rep2, Period2>&) [with CharT = char, Traits = std::char_traits<char>, Rep = double, Period = boost::ratio<1l, 1l>]'
io/duration_input.cpp:15:   instantiated from 'void test_good(const char*, D) [with D = boost::chrono::duration<double, boost::ratio<1l, 1l> >]'
io/duration_input.cpp:52:   instantiated from here
../../../boost/math/common_factor_rt.hpp:102: error: invalid operands of types 'long double' and 'long double' to binary 'operator%'
../../../boost/math/common_factor_rt.hpp:102: error:   in evaluation of 'operator%=(long double, long double)'
../../../boost/math/common_factor_rt.hpp:106: error: invalid operands of types 'long double' and 'long double' to binary 'operator%'
../../../boost/math/common_factor_rt.hpp:106: error:   in evaluation of 'operator%=(long double, long double)'
   *
   */
  test_good("1 second", seconds(1));
  test_not_eof("1 second ", seconds(1));
  test_not_eof("1 seconde", seconds(1));
  test_good("1 seconds", seconds(1));
  test_good("0 seconds", seconds(0));
  test_good("-1 seconds", seconds(-1));
  test_good("5000 milliseconds", milliseconds(5000));
  test_good("5000 microseconds", microseconds(5000));
  test_good("5000 nanoseconds", nanoseconds(5000));
  test_good("5000 deciseconds", duration<boost::int_least64_t, deci> (5000));
  test_good("5000 [1/30]seconds", duration<boost::int_least64_t, ratio<1, 30> > (5000));
  test_good("5000 [1/30]second", duration<boost::int_least64_t, ratio<1, 30> > (5000));
  test_good("5000 h", hours(5000));
#if BOOST_CHRONO_VERSION==2
  test_good("5000 min", minutes(5000));
#else
  test_good("5000 m", minutes(5000));
#endif
  test_good("5000 s", seconds(5000));
  test_good("5000 ms", milliseconds(5000));
  test_good("5000 ns", nanoseconds(5000));
  test_good("5000 ds", duration<boost::int_least64_t, deci> (5000));
  test_good("5000 [1/30]s", duration<boost::int_least64_t, ratio<1, 30> > (5000));
  test_not_eof("5000 [1/30]ss", duration<boost::int_least64_t, ratio<1, 30> > (5000));
  std::cout << __LINE__<< "*****" << std::endl;
  test_good("5000 milliseconds", seconds(5));
  test_good("5000 millisecond", seconds(5));
  test_good("5 milliseconds", nanoseconds(5000000));
  std::cout << __LINE__<< "*****" << std::endl;
  test_good("4000 ms", seconds(4));
  std::cout << __LINE__<< "*****" << std::endl;
  test_fail("3001 ms", seconds(3));
  std::cout << __LINE__<< "*****" << std::endl;
  test_fail("3001 ", milliseconds(3001));
  std::cout << __LINE__<< "*****" << std::endl;
  test_fail("one ms", milliseconds(1));
  test_fail("5000 millisecon", seconds(5));
  test_not_eof("3001 ms ", milliseconds(3001));

  return boost::report_errors();

}

