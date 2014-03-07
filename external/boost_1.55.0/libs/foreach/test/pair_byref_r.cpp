//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   13 December 2004 : Initial version.
   25 August 2005 : Initial version.
*/

#include <boost/test/minimal.hpp>
#include <boost/foreach.hpp>

///////////////////////////////////////////////////////////////////////////////
// define the container types, used by utility.hpp to generate the helper functions
typedef std::pair<int*,int*> foreach_container_type;
typedef std::pair<int const*,int const*> const foreach_const_container_type;
typedef int foreach_value_type;
typedef int &foreach_reference_type;
typedef int const &foreach_const_reference_type;

#include "./utility.hpp"

///////////////////////////////////////////////////////////////////////////////
// define some containers
//
int my_array[] = { 1,2,3,4,5 };
std::pair<int*,int*> my_pair(my_array,my_array+5);
std::pair<int const*,int const*> const my_const_pair(my_array,my_array+5);

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    // non-const containers by reference
    BOOST_CHECK(sequence_equal_byref_n_r(my_pair, "\5\4\3\2\1"));

    // const containers by reference
    BOOST_CHECK(sequence_equal_byref_c_r(my_const_pair, "\5\4\3\2\1"));

    // mutate the mutable collections
    mutate_foreach_byref_r(my_pair);

    // compare the mutated collections to the actual results
    BOOST_CHECK(sequence_equal_byref_n_r(my_pair, "\6\5\4\3\2"));

    return 0;
}
