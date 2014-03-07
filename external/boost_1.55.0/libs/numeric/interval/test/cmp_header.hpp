/* Boost test/cmp_header.hpp header file
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval/interval.hpp>
#include <boost/numeric/interval/checking.hpp>
#include <boost/numeric/interval/compare.hpp>
#include <boost/numeric/interval/policies.hpp>
#include <boost/test/test_tools.hpp>
#include "bugs.hpp"

struct empty_class {};

typedef boost::numeric::interval_lib::policies
          <empty_class, boost::numeric::interval_lib::checking_base<int> >
  my_policies;

typedef boost::numeric::interval<int, my_policies> I;

#define BOOST_C_EXN(e) \
  BOOST_CHECK_THROW(e, boost::numeric::interval_lib::comparison_error)
