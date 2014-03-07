/* boost random_demo.cpp profane demo
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: random_demo.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 * A short demo program how to use the random number library.
 */

#include <iostream>
#include <fstream>
#include <ctime>            // std::time

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>

// This is a typedef for a random number generator.
// Try boost::mt19937 or boost::ecuyer1988 instead of boost::minstd_rand
typedef boost::minstd_rand base_generator_type;

// This is a reproducible simulation experiment.  See main().
void experiment(base_generator_type & generator)
{
  // Define a uniform random number distribution of integer values between
  // 1 and 6 inclusive.
  typedef boost::uniform_int<> distribution_type;
  typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
  gen_type die_gen(generator, distribution_type(1, 6));

  // If you want to use an STL iterator interface, use iterator_adaptors.hpp.
  boost::generator_iterator<gen_type> die(&die_gen);
  for(int i = 0; i < 10; i++)
    std::cout << *die++ << " ";
  std::cout << '\n';
}

int main()
{
  // Define a random number generator and initialize it with a reproducible
  // seed.
  base_generator_type generator(42);

  std::cout << "10 samples of a uniform distribution in [0..1):\n";

  // Define a uniform random number distribution which produces "double"
  // values between 0 and 1 (0 inclusive, 1 exclusive).
  boost::uniform_real<> uni_dist(0,1);
  boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(generator, uni_dist);

  std::cout.setf(std::ios::fixed);
  // You can now retrieve random numbers from that distribution by means
  // of a STL Generator interface, i.e. calling the generator as a zero-
  // argument function.
  for(int i = 0; i < 10; i++)
    std::cout << uni() << '\n';

  /*
   * Change seed to something else.
   *
   * Caveat: std::time(0) is not a very good truly-random seed.  When
   * called in rapid succession, it could return the same values, and
   * thus the same random number sequences could ensue.  If not the same
   * values are returned, the values differ only slightly in the
   * lowest bits.  A linear congruential generator with a small factor
   * wrapped in a uniform_smallint (see experiment) will produce the same
   * values for the first few iterations.   This is because uniform_smallint
   * takes only the highest bits of the generator, and the generator itself
   * needs a few iterations to spread the initial entropy from the lowest bits
   * to the whole state.
   */
  generator.seed(static_cast<unsigned int>(std::time(0)));

  std::cout << "\nexperiment: roll a die 10 times:\n";

  // You can save a generator's state by copy construction.
  base_generator_type saved_generator = generator;

  // When calling other functions which take a generator or distribution
  // as a parameter, make sure to always call by reference (or pointer).
  // Calling by value invokes the copy constructor, which means that the
  // sequence of random numbers at the caller is disconnected from the
  // sequence at the callee.
  experiment(generator);

  std::cout << "redo the experiment to verify it:\n";
  experiment(saved_generator);

  // After that, both generators are equivalent
  assert(generator == saved_generator);

  // as a degenerate case, you can set min = max for uniform_int
  boost::uniform_int<> degen_dist(4,4);
  boost::variate_generator<base_generator_type&, boost::uniform_int<> > deg(generator, degen_dist);
  std::cout << deg() << " " << deg() << " " << deg() << std::endl;
  
  {
    // You can save the generator state for future use.  You can read the
    // state back in at any later time using operator>>.
    std::ofstream file("rng.saved", std::ofstream::trunc);
    file << generator;
  }

  return 0;
}
