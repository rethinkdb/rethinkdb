// (C) Copyright Jeremy Siek 2000-2004.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <boost/config.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/cstdlib.hpp>

int main(int, char*[])
{
  // Example of using counting_iterator
  std::cout << "counting from 0 to 4:" << std::endl;
  boost::counting_iterator<int> first(0), last(4);
  std::copy(first, last, std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;

  // Example of using counting iterator to create an array of pointers.
  int N = 7;
  std::vector<int> numbers;
  typedef std::vector<int>::iterator n_iter;
  // Fill "numbers" array with [0,N)
  std::copy(
      boost::counting_iterator<int>(0)
      , boost::counting_iterator<int>(N)
      , std::back_inserter(numbers));

  std::vector<std::vector<int>::iterator> pointers;

  // Use counting iterator to fill in the array of pointers.
  // causes an ICE with MSVC6
  std::copy(boost::make_counting_iterator(numbers.begin()),
            boost::make_counting_iterator(numbers.end()),
            std::back_inserter(pointers));

  // Use indirect iterator to print out numbers by accessing
  // them through the array of pointers.
  std::cout << "indirectly printing out the numbers from 0 to " 
            << N << std::endl;
  std::copy(boost::make_indirect_iterator(pointers.begin()),
            boost::make_indirect_iterator(pointers.end()),
            std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;
  
  return boost::exit_success;
}
