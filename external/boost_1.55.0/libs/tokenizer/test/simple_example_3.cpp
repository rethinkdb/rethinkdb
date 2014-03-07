// (c) Copyright John R. Bandela 2001. 

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/tokenizer for documenation

// simple_example_3.cpp
#include<iostream>
#include<boost/tokenizer.hpp>
#include<string>

int main(){
   using namespace std;
   using namespace boost;
   string s = "12252001";
   int offsets[] = {2,2,4};
   offset_separator f(offsets, offsets+3);
   tokenizer<offset_separator> tok(s,f);
   for(tokenizer<offset_separator>::iterator beg=tok.begin(); beg!=tok.end();++beg){
       cout << *beg << "\n";
   }
   return 0;
}
