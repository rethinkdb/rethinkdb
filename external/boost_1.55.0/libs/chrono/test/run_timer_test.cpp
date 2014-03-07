//  boost run_timer_test.cpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2006, 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#include <boost/chrono/process_times.hpp>
#include <boost/chrono/timer.hpp>
#include <cstdlib> // for atol()
#include <iostream>
#include <sstream>
#include <locale>
#include <ctime>
#include <cmath>  // for sqrt(), used to burn time

using boost::chrono::run_timer;
using boost::system::error_code;

#include <boost/detail/lightweight_test.hpp>

//#define BOOST_TEST(expr) if (!(expr)) std::cout << "*****ERROR*****\n"

#define CHECK_REPORT(Timer,String_Stream,R,U,S,Expected_String) \
  check_report(Timer, String_Stream, R, U, S, Expected_String, __LINE__)


namespace
{
  typedef boost::chrono::nanoseconds ns;

  bool check_report( run_timer & tmr, std::stringstream & ss,
    run_timer::duration r, run_timer::duration u, run_timer::duration s,
    const std::string & expected, int line )
  {
    tmr.test_report(r,u,s);
    bool result(true);
    if ( ss.str() != expected )
    {
      std::cout << "run_timer_test.cpp(" << line << ") : error: actual output \""
        << ss.str() << "\" != expected \"" << expected << "\"\n";
      result = false;
    }
    return result;
  }

  void run_timer_constructor_overload_test()
  {
    // exercise each supported combination of constructor arguments

    std::ostream &             os   = std::cout;
    const int                  pl   = 9;
    boost::system::error_code  ec;

    run_timer t1;
    run_timer t2( os );
    run_timer t3( ec );
    run_timer t4( os, ec );
    run_timer t5( pl );
    run_timer t6( os, pl );
    run_timer t7( pl, ec );
    run_timer t8( os, pl, ec );
    run_timer t9( "t9, default places,  r %r, c %c, p %p, u %u, s %s\n" );
    run_timer t10( os, "t10, default places,  r %r, c %c, p %p, u %u, s %s\n" );
    run_timer t11( "t11, default places,  r %r, c %c, p %p, u %u, s %s\n", ec );
    run_timer t12( os, "t12, default places,  r %r, c %c, p %p, u %u, s %s\n", ec );
    run_timer t13( pl, "t13, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n" );
    run_timer t14( "t14, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", pl );
    run_timer t15( os, pl, "t15, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n" );
    run_timer t16( os, "t16, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", pl );
    run_timer t17( pl, "t17, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", ec );
    run_timer t18( "t18, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", pl, ec );
    run_timer t19( os, pl, "t19, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", ec );
    run_timer t20( os, "t20, explicitly code places,  r %r, c %c, p %p, u %u, s %s\n", pl, ec );

    std::cout << "Burn some time so run_timers have something to report...";
    boost::chrono::timer<boost::chrono::high_resolution_clock> t;
    while ( t.elapsed() < boost::chrono::seconds(1) ) {}
    std::cout << "\n";
    std::cout << run_timer::default_places() << " default places\n";
    std::cout << pl << " explicitly coded places\n";
  }

  // accuracy test
  void accuracy_test( int argc, char * argv[] )
  {
    long timeout_in_secs = 1;
    if ( argc > 1 ) timeout_in_secs = std::atol( argv[1] );
    std::cout << "accuracy test for " << timeout_in_secs << " second(s)...";

    std::clock_t timeout_in_clock_t = std::clock();
    timeout_in_clock_t += (timeout_in_secs * CLOCKS_PER_SEC);

    boost::chrono::system_timer           sys;
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    boost::chrono::steady_timer        mono;
#endif
    boost::chrono::high_resolution_timer  hires;
    boost::chrono::process_timer          process;

    std::clock_t now;
    do
    {
      now = std::clock();
    } while ( now < timeout_in_clock_t );

    boost::chrono::system_timer::duration sys_dur = sys.elapsed();
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    boost::chrono::steady_timer::duration mono_dur = mono.elapsed();
#endif
    boost::chrono::high_resolution_timer::duration hires_dur = hires.elapsed();
    boost::chrono::process_times times;
    process.elapsed( times );

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
    std::cout << mono_dur.count() << " mono_dur\n";

    BOOST_TEST( mono_dur > timeout_in_nanoseconds - maximum_delta
      && mono_dur < timeout_in_nanoseconds + maximum_delta );
#endif

    std::cout << hires_dur.count() << " hires_dur\n";

    BOOST_TEST( hires_dur > timeout_in_nanoseconds - maximum_delta
      && hires_dur < timeout_in_nanoseconds + maximum_delta );

    std::cout << times.real.count() << " times.real\n";

    BOOST_TEST( times.real > timeout_in_nanoseconds - maximum_delta
      && times.real < timeout_in_nanoseconds + maximum_delta );
  }

  //  report test

  void report_test()
  {
    {
      std::stringstream ss;
      run_timer t(ss);
      BOOST_TEST( CHECK_REPORT(t, ss, ns(0), ns(0), ns(0),
        "\nreal 0.000s, cpu 0.000s (0.0%), user 0.000s, system 0.000s\n" ) );
    }

    {
      std::stringstream ss;
      run_timer t(ss);
      BOOST_TEST( CHECK_REPORT(t, ss, ns(3000000000LL), ns(2000000000LL), ns(1000000000LL),
        "\nreal 3.000s, cpu 3.000s (100.0%), user 2.000s, system 1.000s\n" ) );
    }

    {
      std::stringstream ss;
      run_timer t( ss, "9 places: r %r, c %c, p %p, u %u, s %s", 9 );
      BOOST_TEST( CHECK_REPORT(t, ss, ns(3000000003LL), ns(2000000002LL), ns(1000000001LL),
        "9 places: "
        "r 3.000000003, c 3.000000003, p 100.0, u 2.000000002, s 1.000000001" ) );
    }
  }

  //  process_timer_test

  void process_timer_test()
  {
    std::cout << "process_timer_test..." << std::flush;

    boost::chrono::process_timer t;
    double res=0; // avoids optimization
    for (long i = 0; i < 10000000L; ++i)
    {
      res+=std::sqrt( static_cast<double>(i) ); // avoids optimization
    }

    boost::chrono::process_times times;
    times.real = times.system = times.user = ns(0);

    BOOST_TEST( times.real == ns(0) );
    BOOST_TEST( times.user == ns(0) );
    BOOST_TEST( times.system == ns(0) );

    t.elapsed( times );
    std::cout << "\n";

    std::cout << times.real.count() << " times.real\n";
    std::cout << times.user.count() << " times.user\n";
    std::cout << times.system.count() << " times.system\n";
    std::cout << (times.user+times.system).count() << " times.user+system\n";
    BOOST_TEST( times.real > ns(1) );

    BOOST_TEST( times.user+times.system  > ns(1) );

    std::cout << "complete " << res << std::endl;
  }
}

int main( int argc, char * argv[] )
{
  std::locale loc( "" );     // test with appropriate locale
  std::cout.imbue( loc );

  accuracy_test( argc, argv );
  run_timer_constructor_overload_test();
  process_timer_test();
  report_test();

  return boost::report_errors();
}

