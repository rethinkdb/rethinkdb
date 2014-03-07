//  run_timer_example.cpp  ---------------------------------------------------//

//  Copyright Beman Dawes 2006, 2008
//  Copyright 2009/2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#include <boost/chrono/process_times.hpp>
#include <cmath>

int main()
{
  boost::chrono::run_timer t;

  for ( long i = 0; i < 10000; ++i )
    std::sqrt( 123.456L );  // burn some time

  return 0;
}
