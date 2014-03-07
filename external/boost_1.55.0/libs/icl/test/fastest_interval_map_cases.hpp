/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_FASTEST_INTERVAL_MAP_CASES_HPP_JOFA_090702
#define BOOST_ICL_FASTEST_INTERVAL_MAP_CASES_HPP_JOFA_090702

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_fundamentals_4_ordered_types)
{            interval_map_fundamentals_4_ordered_types<INTERVAL_MAP, ordered_type_1, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_ctor_4_bicremental_types)
{            interval_map_ctor_4_bicremental_types<INTERVAL_MAP, bicremental_type_1, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_add_sub_4_bicremental_types)
{            interval_map_add_sub_4_bicremental_types<INTERVAL_MAP, bicremental_type_2, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_distinct_4_bicremental_types)
{            interval_map_distinct_4_bicremental_types<INTERVAL_MAP, bicremental_type_3, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_distinct_4_bicremental_continuous_types)
{            interval_map_distinct_4_bicremental_continuous_types<INTERVAL_MAP, continuous_type_1, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_isolate_4_bicremental_continuous_types)
{            interval_map_isolate_4_bicremental_continuous_types<INTERVAL_MAP, continuous_type_2, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_contains_4_bicremental_types)
{            interval_map_contains_4_bicremental_types<INTERVAL_MAP, bicremental_type_4, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_contains_key_objects_4_bicremental_types)
{            interval_map_contains_key_objects_4_bicremental_types<INTERVAL_MAP, bicremental_type_4, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_operators_4_bicremental_types)
{            interval_map_operators_4_bicremental_types<INTERVAL_MAP, bicremental_type_5, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_base_intersect_4_bicremental_types)
{            interval_map_base_intersect_4_bicremental_types<INTERVAL_MAP, bicremental_type_6, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_base_erase_4_bicremental_types)
{            interval_map_base_erase_4_bicremental_types<INTERVAL_MAP, bicremental_type_7, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_base_is_disjoint_4_bicremental_types)
{            interval_map_base_is_disjoint_4_bicremental_types<INTERVAL_MAP, bicremental_type_8, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_flip_4_bicremental_types)
{            interval_map_flip_4_bicremental_types<INTERVAL_MAP, bicremental_type_1, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_find_4_bicremental_types)
{            interval_map_find_4_bicremental_types<INTERVAL_MAP, bicremental_type_2, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_find_4_numeric_continuous_types)
{            interval_map_find_4_numeric_continuous_types<INTERVAL_MAP, numeric_continuous_type_1, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_range_4_bicremental_types)
{            interval_map_range_4_bicremental_types<INTERVAL_MAP, bicremental_type_2, double>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_set_4_bicremental_types)
{            interval_map_set_4_bicremental_types<INTERVAL_MAP, bicremental_type_3, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_inclusion_compare_4_bicremental_types)
{            interval_map_inclusion_compare_4_bicremental_types<discrete_type_4, double, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_std_copy_via_inserter_4_bicremental_types)
{            interval_map_std_copy_via_inserter_4_bicremental_types<bicremental_type_4, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_element_iter_4_discrete_types)
{            interval_map_element_iter_4_discrete_types<discrete_type_2, double, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_intersects_4_bicremental_types)
{            interval_map_intersects_4_bicremental_types<INTERVAL_MAP, bicremental_type_3, int>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_interval_map_move_4_discrete_types)
{            interval_map_move_4_discrete_types<INTERVAL_MAP, discrete_type_1, double>();}


#endif // BOOST_ICL_FASTEST_INTERVAL_MAP_CASES_HPP_JOFA_090702

