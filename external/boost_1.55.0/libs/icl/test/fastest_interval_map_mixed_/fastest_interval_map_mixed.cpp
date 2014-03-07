/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::interval_map_mixed unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include <string>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/icl/interval_set.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;

#include "../test_interval_map_mixed.hpp"

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_ctor_4_ordered_types)
{            interval_map_mixed_ctor_4_ordered_types<std::string, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_equal_4_ordered_types)
{            interval_map_mixed_equal_4_ordered_types<double, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_assign_4_ordered_types)
{            interval_map_mixed_assign_4_ordered_types<unsigned int, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_ctor_4_bicremental_types)
{            interval_map_mixed_ctor_4_bicremental_types<bicremental_type_1, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_assign_4_bicremental_types)
{            interval_map_mixed_assign_4_bicremental_types<bicremental_type_2, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_equal_4_bicremental_types)
{            interval_map_mixed_equal_4_bicremental_types<bicremental_type_3, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_partial_interval_map_mixed_inclusion_compare_4_bicremental_types)
{            partial_interval_map_mixed_inclusion_compare_4_bicremental_types<bicremental_type_4, int, partial_absorber>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_partial_interval_map_mixed_contains_4_bicremental_types)
{            partial_interval_map_mixed_contains_4_bicremental_types<int, int, partial_absorber>(); }

