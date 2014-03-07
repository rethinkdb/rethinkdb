/* boost nondet_random_speed.cpp performance test
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: nondet_random_speed.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <iostream>
#include <string>
#include <boost/timer.hpp>
#include <boost/random/random_device.hpp>

// set to your CPU frequency
static const double cpu_frequency = 1.87 * 1e9;

static void show_elapsed(double end, int iter, const std::string & name)
{
  double usec = end/iter*1e6;
  double cycles = usec * cpu_frequency/1e6;
  std::cout << name << ": " 
            << usec*1e3 << " nsec/loop = " 
            << cycles << " CPU cycles"
            << std::endl;
}

template<class Result, class RNG>
static void timing(RNG & rng, int iter, const std::string& name)
{
  volatile Result tmp; // make sure we're not optimizing too much
  boost::timer t;
  for(int i = 0; i < iter; i++)
    tmp = rng();
  show_elapsed(t.elapsed(), iter, name);
}

template<class RNG>
void run(int iter, const std::string & name)
{
  RNG rng;
  timing<long>(rng, iter, name);
}

int main(int argc, char*argv[])
{
  if(argc != 2) {
    std::cerr << "usage: " << argv[0] << " iterations" << std::endl;
    return 1;
  }

  int iter = std::atoi(argv[1]);

  boost::random::random_device dev;
  timing<unsigned int>(dev, iter, "random_device");

  return 0;
}
