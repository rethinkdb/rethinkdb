/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
// interval_set
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_itl_set_check_monoid_plus_4_bicremental_types, T, discrete_types)
{         itl_set_check_monoid_plus_4_bicremental_types<T, interval_set>();}


BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_itl_set_check_abelian_monoid_plus_4_bicremental_types, T, discrete_types)
{         itl_set_check_abelian_monoid_plus_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_itl_set_check_abelian_monoid_et_4_bicremental_types, T, discrete_types)
{         itl_set_check_abelian_monoid_et_4_bicremental_types<T, interval_set>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_itl_set_check_partial_invertive_monoid_plus_4_bicremental_types, T, discrete_types)
{         itl_set_check_partial_invertive_monoid_plus_4_bicremental_types<T, interval_set>();}

