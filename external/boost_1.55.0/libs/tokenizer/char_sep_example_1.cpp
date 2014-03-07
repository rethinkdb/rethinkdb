// (c) Copyright Jeremy Siek 2002. 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Sample output:
//
// <Hello> <world> <foo> <bar> <yow> <baz> 

// char_sep_example_1.cpp
#include <iostream>
#include <boost/tokenizer.hpp>
#include <string>

int main()
{
  std::string str = ";;Hello|world||-foo--bar;yow;baz|";
  typedef boost::tokenizer<boost::char_separator<char> > 
    tokenizer;
  boost::char_separator<char> sep("-;|");
  tokenizer tokens(str, sep);
  for (tokenizer::iterator tok_iter = tokens.begin();
       tok_iter != tokens.end(); ++tok_iter)
    std::cout << "<" << *tok_iter << "> ";
  std::cout << "\n";
  return EXIT_SUCCESS;
}
