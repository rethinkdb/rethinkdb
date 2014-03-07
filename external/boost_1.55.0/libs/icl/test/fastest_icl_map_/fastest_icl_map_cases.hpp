/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_ICL_MAP_CASES_HPP_JOFA_090701
#define BOOST_ICL_TEST_ICL_MAP_CASES_HPP_JOFA_090701

BOOST_AUTO_TEST_CASE
(fastest_icl_icl_map_contains_4_bicremental_types)
{            icl_map_contains_4_bicremental_types<discrete_type_1, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_icl_map_find_4_bicremental_types)
{            icl_map_find_4_bicremental_types<discrete_type_1, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_icl_icl_map_move_4_discrete_types)
{            icl_map_move_4_discrete_types<discrete_type_1, int, partial_absorber, INTERVAL_MAP>();}

#endif // BOOST_ICL_TEST_ICL_MAP_CASES_HPP_JOFA_090701
