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
(fastest_icl_interval_map_mixed_add_4_bicremental_types)
{            interval_map_mixed_add_4_bicremental_types<bicremental_type_1, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_add2_4_bicremental_types)
{            interval_map_mixed_add2_4_bicremental_types<bicremental_type_2, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_subtract_4_bicremental_types)
{            interval_map_mixed_subtract_4_bicremental_types<bicremental_type_3, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_erase_4_bicremental_types)
{            interval_map_mixed_erase_4_bicremental_types<bicremental_type_4, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_erase2_4_bicremental_types)
{            interval_map_mixed_erase2_4_bicremental_types<bicremental_type_5, int>(); }


BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_insert_erase_4_bicremental_types)
{            interval_map_mixed_insert_erase_4_bicremental_types<bicremental_type_6, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_insert_erase2_4_bicremental_types)
{            interval_map_mixed_insert_erase2_4_bicremental_types<bicremental_type_7, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_basic_intersect_4_bicremental_types)
{            interval_map_mixed_basic_intersect_4_bicremental_types<bicremental_type_8, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_basic_intersect2_4_bicremental_types)
{            interval_map_mixed_basic_intersect2_4_bicremental_types<bicremental_type_1, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_intersect_4_bicremental_types)
{            interval_map_mixed_intersect_4_bicremental_types<bicremental_type_2, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_intersect2_4_bicremental_types)
{            interval_map_mixed_intersect2_4_bicremental_types<bicremental_type_3, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_disjoint_4_bicremental_types)
{            interval_map_mixed_disjoint_4_bicremental_types<bicremental_type_4, int>(); }

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_mixed_erase_if_4_integral_types)
{            interval_map_mixed_erase_if_4_integral_types<int, int>(); }

