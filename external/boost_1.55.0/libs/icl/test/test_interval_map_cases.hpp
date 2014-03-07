/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_INTERVAL_MAP_CASES_HPP_JOFA_090701
#define BOOST_ICL_TEST_INTERVAL_MAP_CASES_HPP_JOFA_090701

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_fundamentals_4_ordered_types, T, ordered_types)
{         interval_map_fundamentals_4_ordered_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_ctor_4_bicremental_types, T, bicremental_types)
{         interval_map_ctor_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_add_sub_4_bicremental_types, T, bicremental_types)
{         interval_map_add_sub_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_distinct_4_bicremental_types, T, bicremental_types)
{         interval_map_distinct_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_distinct_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_map_distinct_4_bicremental_continuous_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_isolate_4_bicremental_continuous_types, T, bicremental_continuous_types)
{         interval_map_isolate_4_bicremental_continuous_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_contains_4_bicremental_types, T, bicremental_types)
{         interval_map_contains_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_contains_key_objects_4_bicremental_types, T, bicremental_types)
{         interval_map_contains_key_objects_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_operators_4_bicremental_types, T, bicremental_types)
{         interval_map_operators_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_intersect_4_bicremental_types, T, bicremental_types)
{         interval_map_base_intersect_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_erase_4_bicremental_types, T, bicremental_types)
{         interval_map_base_erase_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_base_is_disjoint_4_bicremental_types, T, bicremental_types)
{         interval_map_base_is_disjoint_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_flip_4_bicremental_types, T, bicremental_types)
{         interval_map_flip_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_find_4_bicremental_types, T, bicremental_types)
{         interval_map_find_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_find_4_numeric_continuous_types, T, numeric_continuous_types)
{         interval_map_find_4_numeric_continuous_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_range_4_bicremental_types, T, bicremental_types)
{         interval_map_range_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_set_4_bicremental_types, T, bicremental_types)
{         interval_map_set_4_bicremental_types<INTERVAL_MAP, T, int>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_inclusion_compare_4_bicremental_types, T, bicremental_types)
{         interval_map_inclusion_compare_4_bicremental_types<T, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_std_copy_via_inserter_4_bicremental_types, T, bicremental_types)
{         interval_map_std_copy_via_inserter_4_bicremental_types<T, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_map_element_iter_4_discrete_types, T, discrete_types)
{         interval_map_element_iter_4_discrete_types<T, int, partial_absorber, INTERVAL_MAP>();}

#endif // BOOST_ICL_TEST_INTERVAL_MAP_CASES_HPP_JOFA_090701

