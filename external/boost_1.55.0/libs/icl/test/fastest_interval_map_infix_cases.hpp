/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_FASTEST_INTERVAL_MAP_INFIX_CASES_HPP_JOFA_090702
#define BOOST_ICL_FASTEST_INTERVAL_MAP_INFIX_CASES_HPP_JOFA_090702

BOOST_AUTO_TEST_CASE
(test_icl_interval_map_infix_plus_overload_4_bicremental_types)
{         interval_map_infix_plus_overload_4_bicremental_types<INTERVAL_MAP, bicremental_type_1, int>();}

BOOST_AUTO_TEST_CASE
(test_icl_interval_map_infix_pipe_overload_4_bicremental_types)
{         interval_map_infix_pipe_overload_4_bicremental_types<INTERVAL_MAP, bicremental_type_2, int>();}

BOOST_AUTO_TEST_CASE
(test_icl_interval_map_infix_minus_overload_4_bicremental_types)
{         interval_map_infix_minus_overload_4_bicremental_types<INTERVAL_MAP, bicremental_type_3, int>();}

BOOST_AUTO_TEST_CASE
(test_icl_interval_map_infix_et_overload_4_bicremental_types)
{         interval_map_infix_et_overload_4_bicremental_types<INTERVAL_MAP, bicremental_type_4, int>();}

BOOST_AUTO_TEST_CASE
(test_icl_interval_map_infix_caret_overload_4_bicremental_types)
{         interval_map_infix_caret_overload_4_bicremental_types<INTERVAL_MAP, bicremental_type_1, int>();}

#endif // BOOST_ICL_FASTEST_INTERVAL_MAP_INFIX_CASES_HPP_JOFA_090702

