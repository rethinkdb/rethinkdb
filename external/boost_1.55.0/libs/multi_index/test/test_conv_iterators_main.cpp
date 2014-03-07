/* Boost.MultiIndex test for interconvertibilty between const and
 * non-const iterators.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <boost/detail/lightweight_test.hpp>
#include "test_conv_iterators.hpp"

int main()
{
  test_conv_iterators();
  return boost::report_errors();
}


