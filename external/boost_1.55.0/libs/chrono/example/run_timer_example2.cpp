//  run_timer_example.cpp  ---------------------------------------------------//

//  Copyright Beman Dawes 2006, 2008

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#include <boost/chrono/process_times.hpp>
#include <cmath>

int main( int argc, char * argv[] )
{
  const char * format = argc > 1 ? argv[1] : "%t cpu seconds\n";
  int          places = argc > 2 ? std::atoi( argv[2] ) : 2;

  boost::chrono::run_timer t( format, places );

  for ( long i = 0; i < 10000; ++i )
    std::sqrt( 123.456L ); // burn some time

  return 0;
}
