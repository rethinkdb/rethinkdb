/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_SET_INTERVAL_SET_CASES_HPP_JOFA_090701
#define BOOST_ICL_TEST_SET_INTERVAL_SET_CASES_HPP_JOFA_090701

//------------------------------------------------------------------------------
// interval_set
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{         interval_set_check_monoid_plus_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{         interval_set_check_abelian_monoid_plus_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{         interval_set_check_abelian_monoid_et_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_icl_interval_set_check_partial_invertive_monoid_plus_4_bicremental_types, T, bicremental_types)
{         interval_set_check_partial_invertive_monoid_plus_4_bicremental_types<T, interval_set>();}


//------------------------------------------------------------------------------
// separate_interval_set
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_separate_interval_set_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_monoid_plus_4_bicremental_types<T, separate_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_separate_interval_set_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_abelian_monoid_plus_4_bicremental_types<T, separate_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_separate_interval_set_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_abelian_monoid_et_4_bicremental_types<T, separate_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_separate_interval_set_check_partial_invertive_monoid_plus_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_partial_invertive_monoid_plus_4_bicremental_types<T, separate_interval_set>();}


//------------------------------------------------------------------------------
// split_interval_set
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_split_interval_set_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{               interval_set_check_monoid_plus_4_bicremental_types<T, split_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_split_interval_set_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{               interval_set_check_abelian_monoid_plus_4_bicremental_types<T, split_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_split_interval_set_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{               interval_set_check_abelian_monoid_et_4_bicremental_types<T, split_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_split_interval_set_check_partial_invertive_monoid_plus_4_bicremental_types, T, bicremental_types)
{               interval_set_check_partial_invertive_monoid_plus_4_bicremental_types<T, split_interval_set>();}

//------------------------------------------------------------------------------
// Containedness
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_icl_interval_set_check_containedness_4_bicremental_types, T, bicremental_types)
{            interval_set_check_containedness_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_split_interval_set_check_containedness_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_containedness_4_bicremental_types<T, split_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_separate_interval_set_check_containedness_4_bicremental_types, T, bicremental_types)
{                     interval_set_check_containedness_4_bicremental_types<T, separate_interval_set>();}

//------------------------------------------------------------------------------
// Inner Complement
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_icl_interval_set_check_inner_complementarity_4_bicremental_types, T, bicremental_types)
{            interval_set_check_inner_complementarity_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_separate_interval_set_check_inner_complementarity_4_bicremental_types, T, bicremental_types)
{                     interval_set_check_inner_complementarity_4_bicremental_types<T, separate_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_split_interval_set_check_inner_complementarity_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_inner_complementarity_4_bicremental_types<T, split_interval_set>();}

//------------------------------------------------------------------------------
// Inner Complement and Distance
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_icl_interval_set_check_length_complementarity_4_bicremental_types, T, bicremental_types)
{            interval_set_check_length_complementarity_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_separate_interval_set_check_length_complementarity_4_bicremental_types, T, bicremental_types)
{                     interval_set_check_length_complementarity_4_bicremental_types<T, separate_interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_split_interval_set_check_length_complementarity_4_bicremental_types, T, bicremental_types)
{                  interval_set_check_length_complementarity_4_bicremental_types<T, split_interval_set>();}



#endif // BOOST_ICL_TEST_SET_INTERVAL_SET_CASES_HPP_JOFA_090701

