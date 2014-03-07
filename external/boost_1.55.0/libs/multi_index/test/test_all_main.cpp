/* Boost.MultiIndex test suite.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <boost/detail/lightweight_test.hpp>
#include "test_basic.hpp"
#include "test_capacity.hpp"
#include "test_comparison.hpp"
#include "test_composite_key.hpp"
#include "test_conv_iterators.hpp"
#include "test_copy_assignment.hpp"
#include "test_hash_ops.hpp"
#include "test_iterators.hpp"
#include "test_key_extractors.hpp"
#include "test_list_ops.hpp"
#include "test_modifiers.hpp"
#include "test_mpl_ops.hpp"
#include "test_observers.hpp"
#include "test_projection.hpp"
#include "test_range.hpp"
#include "test_rearrange.hpp"
#include "test_safe_mode.hpp"
#include "test_serialization.hpp"
#include "test_set_ops.hpp"
#include "test_special_set_ops.hpp"
#include "test_update.hpp"

int main()
{
  test_basic();
  test_capacity();
  test_comparison();
  test_composite_key();
  test_conv_iterators();
  test_copy_assignment();
  test_hash_ops();
  test_iterators();
  test_key_extractors();
  test_list_ops();
  test_modifiers();
  test_mpl_ops();
  test_observers();
  test_projection();
  test_range();
  test_rearrange();
  test_safe_mode();
  test_serialization();
  test_set_ops();
  test_special_set_ops();
  test_update();

  return boost::report_errors();
}
