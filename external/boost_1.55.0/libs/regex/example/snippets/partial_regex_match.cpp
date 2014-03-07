/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         partial_regex_match.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_match example using partial matches.
  */

#include <string>
#include <iostream>
#include <boost/regex.hpp>

boost::regex e("(\\d{3,4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})");

bool is_possible_card_number(const std::string& input)
{
   //
   // return false for partial match, true for full match, or throw for
   // impossible match based on what we have so far...
   boost::match_results<std::string::const_iterator> what;
   if(0 == boost::regex_match(input, what, e, boost::match_default | boost::match_partial))
   {
      // the input so far could not possibly be valid so reject it:
      throw std::runtime_error("Invalid data entered - this could not possibly be a valid card number");
   }
   // OK so far so good, but have we finished?
   if(what[0].matched)
   {
      // excellent, we have a result:
      return true;
   }
   // what we have so far is only a partial match...
   return false;
}

int main(int argc, char* argv[])
{
   try{
      std::string input;
      if(argc > 1)
         input = argv[1];
      else
         std::cin >> input;
      if(is_possible_card_number(input))
      {
         std::cout << "Matched OK..." << std::endl;
      }
      else
         std::cout << "Got a partial match..." << std::endl;
   }
   catch(const std::exception& e)
   {
      std::cout << e.what() << std::endl;
   }
   return 0;
}



