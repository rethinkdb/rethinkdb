// (c) Copyright Jeremy Siek 2002. 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Sample output:
// <This> <is> <,> <a> <test> 

// char_sep_example_3.cpp
#include <iostream>
#include <boost/tokenizer.hpp>
#include <string>

int main()
{
   std::string str = "This is,  a test";
   typedef boost::tokenizer<boost::char_separator<char> > Tok;
   boost::char_separator<char> sep; // default constructed
   Tok tok(str, sep);
   for(Tok::iterator tok_iter = tok.begin(); tok_iter != tok.end(); ++tok_iter)
     std::cout << "<" << *tok_iter << "> ";
   std::cout << "\n";
   return EXIT_SUCCESS;
}

