// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

//
// fail_ref_criterator.cpp
//   const_reverse_iterator/reverse_iterator conversion  test
//

#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"


int test_main(int,char*[]) {
    typedef boost::multi_array_ref<int,3> array_ref;

    typedef array_ref::reverse_iterator riterator1;
    typedef array_ref::const_reverse_iterator criterator1;

    // Fail! ILLEGAL conversion from const_reverse_iterator to reverse_iterator
    riterator1 in = criterator1();

    return boost::exit_success;
}
