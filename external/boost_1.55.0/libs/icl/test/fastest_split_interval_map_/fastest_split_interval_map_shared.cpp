/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_fundamentals_4_ordered_types, T, ordered_types)
{         interval_map_fundamentals_4_ordered_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_ctor_4_bicremental_types, T, bicremental_types)
{         interval_map_ctor_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_add_sub_4_bicremental_types, T, bicremental_types)
{         interval_map_add_sub_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_distinct_4_bicremental_types, T, bicremental_types)
{         interval_map_distinct_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_distinct_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_map_distinct_4_bicremental_continuous_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_isolate_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_map_isolate_4_bicremental_continuous_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_contains_4_bicremental_types, T, bicremental_types)
{         interval_map_contains_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_operators_4_bicremental_types, T, bicremental_types)
{         interval_map_operators_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_intersect_4_bicremental_types, T, bicremental_types)
{         interval_map_base_intersect_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_erase_4_bicremental_types, T, bicremental_types)
{         interval_map_base_erase_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_is_disjoint_4_bicremental_types, T, bicremental_types)
{         interval_map_base_is_disjoint_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_infix_plus_overload_4_bicremental_types, T, bicremental_types)
{         interval_map_infix_plus_overload_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_infix_pipe_overload_4_bicremental_types, T, bicremental_types)
{         interval_map_infix_pipe_overload_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_infix_et_overload_4_bicremental_types, T, bicremental_types)
{         interval_map_infix_et_overload_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_infix_caret_overload_4_bicremental_types, T, bicremental_types)
{         interval_map_infix_caret_overload_4_bicremental_types<split_interval_map, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_find_4_bicremental_types, T, bicremental_types)
{         interval_map_find_4_bicremental_types<split_interval_map, T, int>();}

