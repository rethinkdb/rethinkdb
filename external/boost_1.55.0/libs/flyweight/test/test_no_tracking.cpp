/* Boost.Flyweight test of no_tracking.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_no_tracking.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/hashed_factory.hpp>
#include <boost/flyweight/no_tracking.hpp> 
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/static_holder.hpp>
#include "test_basic_template.hpp"

using namespace boost::flyweights;

struct no_tracking_flyweight_specifier
{
  template<typename T>
  struct apply
  {
    typedef flyweight<T,no_tracking> type;
  };
};

void test_no_tracking()
{
  test_basic_template<no_tracking_flyweight_specifier>();
}
