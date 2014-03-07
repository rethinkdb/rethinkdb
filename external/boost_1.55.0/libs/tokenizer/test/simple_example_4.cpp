// (c) Copyright John R. Bandela 2001. 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/tokenizer for documenation

// simple_example_4.cpp
#include<iostream>
#include<boost/tokenizer.hpp>
#include<string>

int main(){
   using namespace std;
   using namespace boost;
   string s = "This is,  a test";
   tokenizer<char_delimiters_separator<char> > tok(s);
   for(tokenizer<char_delimiters_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg){
       cout << *beg << "\n";
   }
   return 0;
}

