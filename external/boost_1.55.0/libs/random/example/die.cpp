// die.cpp
//
// Copyright (c) 2009
// Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[die
/*`
    For the source of this example see
    [@boost://libs/random/example/die.cpp die.cpp].
    First we include the headers we need for __mt19937
    and __uniform_int_distribution.
*/
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

/*`
  We use __mt19937 with the default seed as a source of
  randomness.  The numbers produced will be the same
  every time the program is run.  One common method to
  change this is to seed with the current time (`std::time(0)`
  defined in ctime).
*/
boost::random::mt19937 gen;
/*`
  [note We are using a /global/ generator object here.  This
  is important because we don't want to create a new [prng
  pseudo-random number generator] at every call]
*/
/*`
  Now we can define a function that simulates an ordinary
  six-sided die.
*/
int roll_die() {
    /*<< __mt19937 produces integers in the range [0, 2[sup 32]-1].
        However, we want numbers in the range [1, 6].  The distribution
        __uniform_int_distribution performs this transformation.
        [warning Contrary to common C++ usage __uniform_int_distribution
        does not take a /half-open range/.  Instead it takes a /closed range/.
        Given the parameters 1 and 6, __uniform_int_distribution can
        can produce any of the values 1, 2, 3, 4, 5, or 6.]
    >>*/
    boost::random::uniform_int_distribution<> dist(1, 6);
    /*<< A distribution is a function object.  We generate a random
        number by calling `dist` with the generator.
    >>*/
    return dist(gen);
}
//]

#include <iostream>

int main() {
    for(int i = 0; i < 10; ++i) {
        std::cout << roll_die() << std::endl;
    }
}
