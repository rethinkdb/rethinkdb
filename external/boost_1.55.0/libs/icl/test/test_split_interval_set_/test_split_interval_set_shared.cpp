/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_fundamentals_4_ordered_types, T, ordered_types)
{         interval_set_fundamentals_4_ordered_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_ctor_4_bicremental_types, T, bicremental_types)
{         interval_set_ctor_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_add_sub_4_bicremental_types, T, bicremental_types)
{         interval_set_add_sub_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_distinct_4_bicremental_types, T, bicremental_types)
{         interval_set_distinct_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_distinct_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_set_distinct_4_bicremental_continuous_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_isolate_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_set_isolate_4_bicremental_continuous_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_element_compare_4_bicremental_types, T, bicremental_types)
{         interval_set_element_compare_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_contains_4_bicremental_types, T, bicremental_types)
{         interval_set_contains_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_operators_4_bicremental_types, T, bicremental_types)
{         interval_set_operators_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_base_intersect_4_bicremental_types, T, bicremental_types)
{         interval_set_base_intersect_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_infix_plus_overload_4_bicremental_types, T, bicremental_types)
{         interval_set_infix_plus_overload_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_find_4_bicremental_types, T, bicremental_types)
{         interval_set_find_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_find_4_bicremental_types, T, bicremental_types)
{         interval_set_find_4_bicremental_types<INTERVAL_SET, T>();}


