//  await_keystroke.cpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2008

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#define _CRT_SECURE_NO_WARNINGS

#include <boost/chrono/chrono.hpp>
#include <iostream>
#include <iomanip>

using namespace boost::chrono;

template< class Clock >
class timer
{
  typename Clock::time_point start;
public:

  timer() : start( Clock::now() ) {}

  typename Clock::duration elapsed() const
  {
    return Clock::now() - start;
  }

  double seconds() const
  {
    return elapsed().count() * ((double)Clock::period::num/Clock::period::den);
  }
};

int main()
{
  timer<system_clock> t1;
  timer<steady_clock> t2;
  timer<high_resolution_clock> t3;

  std::cout << "Strike any key: ";
  std::cin.get();

  std::cout << std::fixed << std::setprecision(9);
  std::cout << "system_clock-----------: "
            << t1.seconds() << " seconds\n";
  std::cout << "steady_clock--------: "
            << t2.seconds() << " seconds\n";
  std::cout << "high_resolution_clock--: "
            << t3.seconds() << " seconds\n";

  system_clock::time_point d4 = system_clock::now();
  system_clock::time_point d5 = system_clock::now();

  std::cout << "\nsystem_clock latency-----------: " << (d5 - d4).count() << std::endl;

  steady_clock::time_point d6 = steady_clock::now();
  steady_clock::time_point d7 = steady_clock::now();

  std::cout << "steady_clock latency--------: " << (d7 - d6).count() << std::endl;

  high_resolution_clock::time_point d8 = high_resolution_clock::now();
  high_resolution_clock::time_point d9 = high_resolution_clock::now();

  std::cout << "high_resolution_clock latency--: " << (d9 - d8).count() << std::endl;

  std::time_t now = system_clock::to_time_t(system_clock::now());

  std::cout << "\nsystem_clock::now() reports UTC is "
    << std::asctime(std::gmtime(&now)) << "\n";

  return 0;
}
