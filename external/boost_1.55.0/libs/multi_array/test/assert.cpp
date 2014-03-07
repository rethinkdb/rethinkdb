// Copyright 2007 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

//
// Using the BOOST.ASSERT mechanism to replace library assertions 
// with exceptions
//

#include "boost/test/minimal.hpp"

#define BOOST_ENABLE_ASSERT_HANDLER
#include "boost/multi_array.hpp" // includes assert.hpp

#include <stdexcept>

namespace boost {

  void assertion_failed(char const* expr, char const* function,
                        char const* file, long line) {
    throw std::runtime_error(expr);
  }

  void assertion_failed_msg(char const * expr, char const * msg,
                            char const * function,
                            char const * file, long line) {
    throw std::runtime_error(msg);
  }

} // namespace boost

using namespace boost;

int
test_main(int,char*[]) {

  typedef multi_array<int,2> array_t;

  array_t A(extents[2][2]);

  array_t B(extents[3][3]);

  try {
    A = B;
    BOOST_ERROR("did not throw an exception");
  } catch (std::runtime_error&) {
    //...all good
  }

  return boost::exit_success;
}
