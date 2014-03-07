/* Boost.Flyweight test suite.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/lightweight_test.hpp>
#include "test_assoc_cont_factory.hpp"
#include "test_basic.hpp"
#include "test_custom_factory.hpp"
#include "test_intermod_holder.hpp"
#include "test_init.hpp"
#include "test_multictor.hpp"
#include "test_no_locking.hpp"
#include "test_no_tracking.hpp"
#include "test_set_factory.hpp"

int main()
{
  test_assoc_container_factory();
  test_basic();
  test_custom_factory();
  test_init();
  test_intermodule_holder();
  test_multictor();
  test_no_locking();
  test_no_tracking();
  test_set_factory();

  return boost::report_errors();
}
