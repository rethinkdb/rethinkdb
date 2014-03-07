//  boost run_timer_test.cpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2006, 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#include <boost/chrono/chrono.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include "boost/chrono/stopwatches/simple_stopwatch.hpp"
#include <cstdlib> // for atol()
#include <iostream>
#include <sstream>
#include <locale>
#include <ctime>
#include <cmath>  // for sqrt(), used to burn time

//using boost::chrono::run_timer;
using boost::system::error_code;

#include <boost/detail/lightweight_test.hpp>

namespace
{
  typedef boost::chrono::nanoseconds ns;

  // accuracy test
  void accuracy_test( int argc, char * argv[] )
  {
    long timeout_in_secs = 1;
    if ( argc > 1 ) timeout_in_secs = std::atol( argv[1] );
    std::cout << "accuracy test for " << timeout_in_secs << " second(s)...";

    std::clock_t timeout_in_clock_t = std::clock();
    std::cout << "accuracy test. Now=" << timeout_in_clock_t << " ticks...";
    timeout_in_clock_t += (timeout_in_secs * CLOCKS_PER_SEC);
    std::cout << "accuracy test. Timeout=" << timeout_in_clock_t << " ticks...";

    boost::chrono::simple_stopwatch<boost::chrono::system_clock>          sys;
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    boost::chrono::simple_stopwatch<boost::chrono::steady_clock>          steady;
#endif
    boost::chrono::simple_stopwatch<>  hires;
    boost::chrono::simple_stopwatch<boost::chrono::process_cpu_clock>          process;

    std::clock_t now;
    do
    {
      now = std::clock();
    } while ( now < timeout_in_clock_t );

    boost::chrono::simple_stopwatch<boost::chrono::system_clock>::duration sys_dur = sys.elapsed();
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    boost::chrono::simple_stopwatch<boost::chrono::steady_clock>::duration steady_dur = steady.elapsed();
#endif
    boost::chrono::simple_stopwatch<>::duration hires_dur = hires.elapsed();
    boost::chrono::simple_stopwatch<boost::chrono::process_cpu_clock>::duration times;
    times = process.elapsed();
    std::cout << "accuracy test. Now=" << now << " ticks...";

    std::cout << std::endl;

    ns timeout_in_nanoseconds( static_cast<long long>(timeout_in_secs) * 1000000000LL );

    //  Allow 20% leeway. Particularly on Linux, there seems to be a large discrepancy
    //  between std::clock() and higher resolution clocks.
    ns maximum_delta ( static_cast<long long>(timeout_in_nanoseconds.count() * 0.20 ) );

    std::cout << timeout_in_nanoseconds.count() << " timeout_in_nanoseconds\n";
    std::cout << maximum_delta.count() << " maximum_delta\n";

    std::cout << sys_dur.count() << " sys_dur\n";

    BOOST_TEST( sys_dur > timeout_in_nanoseconds - maximum_delta
      && sys_dur < timeout_in_nanoseconds + maximum_delta );

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    std::cout << steady_dur.count() << " steady_dur\n";

    BOOST_TEST( steady_dur > timeout_in_nanoseconds - maximum_delta
      && steady_dur < timeout_in_nanoseconds + maximum_delta );
#endif

    std::cout << hires_dur.count() << " hires_dur\n";

    BOOST_TEST( hires_dur > timeout_in_nanoseconds - maximum_delta
      && hires_dur < timeout_in_nanoseconds + maximum_delta );

    std::cout << times.count().real << " times.real\n";

    BOOST_TEST( ns(times.count().real) > timeout_in_nanoseconds - maximum_delta
      && ns(times.count().real) < timeout_in_nanoseconds + maximum_delta );
  }

}

int main( int argc, char * argv[] )
{
  accuracy_test( argc, argv );

  return boost::report_errors();
}

