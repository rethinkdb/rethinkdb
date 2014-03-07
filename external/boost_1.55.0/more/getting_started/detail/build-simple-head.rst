.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Build a Simple Program Using Boost
==================================

To keep things simple, let's start by using a header-only library.
The following program reads a sequence of integers from standard
input, uses Boost.Lambda to multiply each number by three, and
writes them to standard output::

  #include <boost/lambda/lambda.hpp>
  #include <iostream>
  #include <iterator>
  #include <algorithm>

  int main() 
  {
      using namespace boost::lambda;
      typedef std::istream_iterator<int> in;

      std::for_each( 
          in(std::cin), in(), std::cout << (_1 * 3) << " " );
  }

Copy the text of this program into a file called ``example.cpp``.

