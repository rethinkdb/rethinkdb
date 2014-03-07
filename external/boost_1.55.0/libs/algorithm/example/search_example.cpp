/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <string>
#include <iostream>     //  for cout, etc.

#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/algorithm/searching/boyer_moore_horspool.hpp>
#include <boost/algorithm/searching/knuth_morris_pratt.hpp>

namespace ba = boost::algorithm;

const std::string haystack ( "ABACAB is it everywhere!" );
const std::string needle1  ( "ACAB" );
const std::string needle2  ( "not ABA" );



int main ( int /*argc*/, char * /*argv*/ [] ) {
//  In search.hpp, there are generic implementations of three classic sequence search
//  algorithms. They all have the same (dual) interface.

//  There is a procedural interface, based on std::search:
    if ( ba::boyer_moore_search ( haystack.begin (), haystack.end (), needle1.begin (), needle1.end ()) != haystack.end ())
        std::cout << "Found '" << needle1 << "'  in '" << haystack << "' (boyer-moore 1)" << std::endl;
    else
        std::cout << "Did NOT find '" << needle1 << "'  in '" << haystack << "' (boyer-moore 1)" << std::endl;

//  If you plan on searching for the same pattern in several different data sets,
//  you can create a search object and use that over and over again - amortizing the setup
//  costs across several searches
    ba::boyer_moore<std::string::const_iterator> search1 ( needle1.begin (), needle1.end ());
    if ( search1 ( haystack.begin (), haystack.end ()) != haystack.end ())
        std::cout << "Found '" << needle1 << "'  in '" << haystack << "' (boyer-moore 2)" << std::endl;
    else
        std::cout << "Did NOT find '" << needle1 << "'  in '" << haystack << "' (boyer-moore 2)" << std::endl;

//  There is also an implementation of boyer-moore-horspool searching
    if ( ba::boyer_moore_horspool_search ( haystack.begin (), haystack.end (), needle1.begin (), needle1.end ()) != haystack.end ())
        std::cout << "Found '" << needle1 << "'  in '" << haystack << "' (boyer-moore-horspool)" << std::endl;
    else
        std::cout << "Did NOT find '" << needle1 << "'  in '" << haystack << "' (boyer-moore-horspool)" << std::endl;

//  And also the knuth-pratt-morris search algorithm
    if ( ba::knuth_morris_pratt_search ( haystack.begin (), haystack.end (), needle1.begin (), needle1.end ()) != haystack.end ())
        std::cout << "Found '" << needle1 << "'  in '" << haystack << "' (knuth_morris_pratt)" << std::endl;
    else
        std::cout << "Did NOT find '" << needle1 << "'  in '" << haystack << "' (knuth_morris_pratt)" << std::endl;

    return 0;
    }
