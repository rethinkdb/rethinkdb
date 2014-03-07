/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_INTERVAL_SET_CASES_HPP_JOFA_090701
#define BOOST_ICL_TEST_INTERVAL_SET_CASES_HPP_JOFA_090701

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
(test_icl_interval_set_flip_4_bicremental_types, T, bicremental_types)
{         interval_set_flip_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_find_4_bicremental_types, T, bicremental_types)
{         interval_set_find_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_element_iter_4_discrete_types, T, discrete_types)
{         interval_set_element_iter_4_discrete_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_intersects_4_bicremental_types, T, bicremental_types)
{         interval_set_intersects_4_bicremental_types<INTERVAL_SET, T>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_range_4_discrete_types, T, discrete_types)
{         interval_set_range_4_discrete_types<INTERVAL_SET, T>();}


#endif // BOOST_ICL_TEST_INTERVAL_SET_CASES_HPP_JOFA_090701



