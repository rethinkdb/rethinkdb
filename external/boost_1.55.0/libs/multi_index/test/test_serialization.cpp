/* Boost.MultiIndex test for serialization.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_serialization.hpp"
#include "test_serialization1.hpp"
#include "test_serialization2.hpp"
#include "test_serialization3.hpp"

void test_serialization()
{
  test_serialization1();
  test_serialization2();
  test_serialization3();
}
