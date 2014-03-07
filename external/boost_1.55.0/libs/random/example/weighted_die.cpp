// weighted_die.cpp
//
// Copyright (c) 2009
// Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[weighted_die
/*`
    For the source of this example see
    [@boost://libs/random/example/weighted_die.cpp weighted_die.cpp].
*/
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>

boost::mt19937 gen;

/*`
   This time, instead of a fair die, the probability of
   rolling a 1 is 50% (!).  The other five faces are all
   equally likely.

   __discrete_distribution works nicely here by allowing
   us to assign weights to each of the possible outcomes.

   [tip If your compiler supports `std::initializer_list`,
   you can initialize __discrete_distribution directly with
   the weights.]
*/
double probabilities[] = {
    0.5, 0.1, 0.1, 0.1, 0.1, 0.1
};
boost::random::discrete_distribution<> dist(probabilities);

/*`
  Now define a function that simulates rolling this die.
*/
int roll_weighted_die() {
    /*<< Add 1 to make sure that the result is in the range [1,6]
         instead of [0,5].
    >>*/
    return dist(gen) + 1;
}

//]

#include <iostream>

int main() {
    for(int i = 0; i < 10; ++i) {
        std::cout << roll_weighted_die() << std::endl;
    }
}
