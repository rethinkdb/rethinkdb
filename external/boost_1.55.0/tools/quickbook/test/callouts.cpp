
// Copyright 2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or move at http://www.boost.org/LICENSE_1_0.txt)

/*< This shouldn't be used. >*/

//[ example1

/*`
 Now we can define a function that simulates an ordinary
 six-sided die.
*/
int roll_die() {
  boost::uniform_int<> dist(1, 6); /*< create a uniform_int distribution >*/
}

//]

//[ example2

int roll_die() {
  /*<< [important test] >>*/
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
}

//]

//[ example3

int roll_die() {
  /*<< [important test]
  >>*/
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
}

//]

//[ example4

int roll_die() {
  /*<< callout 1 >>*/
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
//[ example4a
  /*<< callout 2 >>*/
  boost::uniform_int<> dist(1, 6); /*< create a uniform_int distribution >*/
//]
}

//]
