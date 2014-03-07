/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/algorithm/searching/boyer_moore.hpp>

int main( int , char* [] )
{
    std::vector<boost::uint8_t> cv;
    std::vector<boost:: int8_t> iv;
    
//  Should fail to compile because the underlying types are different
//  They are the same size, but one is signed, and the other is not.
    (void) boost::algorithm::boyer_moore_search (
        cv.begin (), cv.end (), iv.begin (), iv.end ());

   return 0;
}
