//  boost cpu_timer_info.cpp  ----------------------------------------------------------//

//  Copyright Beman Dawes 2011

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/timer for documentation.

#include <boost/timer/timer.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/detail/lightweight_main.hpp>
#include <cstdlib> // for atol()
#include <iostream>
#include <locale>

using boost::timer::nanosecond_type;
using boost::timer::cpu_times;
using boost::timer::cpu_timer;
using boost::timer::auto_cpu_timer;
using std::cout; using std::endl;

int cpp_main( int argc, char * argv[] )
{
  cout << '\n';
  cout << "For cpu_times.wall, the underlying clock "
       << (boost::chrono::high_resolution_clock::is_steady
           ? "is steady. "
           : "is not steady. "
          )
       << "Steady clocks are defined by C++11 as clocks for which values "
          "of time_point never decrease as physical time advances and for "
          "which values of time_point advance at a steady rate relative to "
          "real time. That is, the clock may not be adjusted.\n\n";
  
  cpu_times start_time;
  start_time.clear();
  cpu_times current_time;

  {
    cpu_timer cpu;
    cout << "measure boost::timer::cpu_timer resolution for user time..."
              << endl;
    for (int i = 0; i < 3; ++i)
    {
      cpu.start();
      start_time = cpu.elapsed();
      current_time.user = start_time.user;
      while (current_time.user == start_time.user)
      {
        current_time = cpu.elapsed();
      }
      cout << current_time.user - start_time.user << "ns\n";
    }
  }
 
  {
    cpu_timer cpu;
    cout << "measure boost::timer::cpu_timer resolution for wall-clock time..."
              << endl;
    for (int i = 0; i < 100; ++i)
    {
      cpu.start();
      start_time.wall = cpu.elapsed().wall;
      current_time.wall = start_time.wall;
      while (current_time.wall == start_time.wall)
      {
        current_time.wall = cpu.elapsed().wall;
      }
      cout << current_time.wall - start_time.wall << "ns ";
    }
  }
 return 0;
}

