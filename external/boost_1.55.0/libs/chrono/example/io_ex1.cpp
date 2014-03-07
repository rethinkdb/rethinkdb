//  io_ex1.cpp  ----------------------------------------------------------//

//  Copyright 2010 Howard Hinnant
//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was adapted by Vicente J. Botet Escriba from Hinnant's html documentation.
Many thanks to Howard for making his code available under the Boost license.

*/

#include <iostream>
#include <boost/chrono/config.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>

int main()
{
    using std::cout;
    using namespace boost;
    using namespace boost::chrono;

    cout << "milliseconds(1)  = "
         <<  milliseconds(1)  << '\n';
    cout << "milliseconds(3) + microseconds(10) = "
         <<  milliseconds(3) + microseconds(10) << '\n';

    cout << "hours(3) + minutes(10) = "
         <<  hours(3) + minutes(10) << '\n';

    typedef duration<long long, ratio<1, 2500000000ULL> > ClockTick;
    cout << "ClockTick(3) + nanoseconds(10) = "
         <<  ClockTick(3) + nanoseconds(10) << '\n';

    cout << "\nSet cout to use short names:\n";
#if BOOST_CHRONO_VERSION==2
    cout << duration_fmt(duration_style::symbol);
#else
    cout << duration_short;
#endif
    cout << "milliseconds(3) + microseconds(10) = "
         <<  milliseconds(3) + microseconds(10) << '\n';

    cout << "hours(3) + minutes(10) = "
         <<  hours(3) + minutes(10) << '\n';

    cout << "ClockTick(3) + nanoseconds(10) = "
         <<  ClockTick(3) + nanoseconds(10) << '\n';

    cout << "\nsystem_clock::now() = " << system_clock::now() << '\n';
#if defined _MSC_VER && _MSC_VER == 1700
#else
#if BOOST_CHRONO_VERSION==2
    cout << "\nsystem_clock::now() = " << time_fmt(chrono::timezone::local) << system_clock::now() << '\n';
    cout << "\nsystem_clock::now() = " << time_fmt(chrono::timezone::local,"%Y/%m/%d") << system_clock::now() << '\n';
#endif
#endif

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    cout << "steady_clock::now() = " << steady_clock::now() << '\n';
#endif
#if BOOST_CHRONO_VERSION==2
    cout << "\nSet cout to use long names:\n" << duration_fmt(duration_style::prefix)
         << "high_resolution_clock::now() = " << high_resolution_clock::now() << '\n';
#else
    cout << "\nSet cout to use long names:\n" <<  duration_long
         << "high_resolution_clock::now() = " << high_resolution_clock::now() << '\n';
#endif
#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
    cout << "\nthread_clock::now() = " << thread_clock::now() << '\n';
#endif
#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
    cout << "\nprocess_real_cpu_clock::now() = " << process_real_cpu_clock::now() << '\n';
    cout << "\nprocess_user_cpu_clock::now() = " << process_user_cpu_clock::now() << '\n';
    cout << "\nprocess_system_cpu_clock::now() = " << process_system_cpu_clock::now() << '\n';
    cout << "\nprocess_cpu_clock::now() = " << process_cpu_clock::now() << '\n';
#endif
    return 0;
}
