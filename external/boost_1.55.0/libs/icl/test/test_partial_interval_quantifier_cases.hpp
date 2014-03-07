/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_PARTIAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090701
#define BOOST_ICL_TEST_PARTIAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090701

//------------------------------------------------------------------------------
// partial_absorber
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_itv_quantifier_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{                 itv_quantifier_check_monoid_plus_4_bicremental_types<T, std::string, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_itv_quantifier_check_monoid_et_4_bicremental_types, T, bicremental_types)
{                 itv_quantifier_check_monoid_et_4_bicremental_types<T, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_itv_quantifier_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{                 itv_quantifier_check_abelian_monoid_plus_4_bicremental_types<T, std::string, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_itv_quantifier_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{                 itv_quantifier_check_abelian_monoid_et_4_bicremental_types<T, int, partial_absorber, INTERVAL_MAP>();}

// x - x = 0 | partial absorber
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_itv_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types, T, bicremental_types)
{                 itv_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types<T, int, partial_absorber, INTERVAL_MAP>();}

//------------------------------------------------------------------------------
// partial_enricher
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_enricher_itv_quantifier_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{                          itv_quantifier_check_monoid_plus_4_bicremental_types<T, std::string, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_enricher_itv_quantifier_check_monoid_et_4_bicremental_types, T, bicremental_types)
{                          itv_quantifier_check_monoid_et_4_bicremental_types<T, int, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_enricher_itv_quantifier_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{                          itv_quantifier_check_abelian_monoid_plus_4_bicremental_types<T, std::string, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_enricher_itv_quantifier_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{                          itv_quantifier_check_abelian_monoid_et_4_bicremental_types<T, int, partial_enricher, INTERVAL_MAP>();}

// x - x =d= 0 | partial enricher
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_partial_enricher_itv_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types, T, bicremental_types)
{                          itv_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types<T, int, partial_enricher, INTERVAL_MAP>();}

//            absorber      enricher
// partial    x - x == 0    x - x =d= 0   partiality of subtraction   
// total    (-x)+ x == 0  (-x)+ x =d= 0   totality   of subtraction


//------------------------------------------------------------------------------
// Inner complement
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_partial_enricher_itv_quantifier_check_inner_complementarity_4_bicremental_types, T, bicremental_types)
{                             itv_quantifier_check_inner_complementarity_4_bicremental_types<T, int, partial_enricher, interval_map>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_partial_enricher_itv_quantifier_check_length_complementarity_4_bicremental_types, T, bicremental_types)
{                             itv_quantifier_check_length_complementarity_4_bicremental_types<T, double, partial_enricher, split_interval_map>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_partial_absorber_itv_quantifier_check_inner_complementarity_4_bicremental_types, T, bicremental_types)
{                             itv_quantifier_check_inner_complementarity_4_bicremental_types<T, int, partial_absorber, split_interval_map>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(fastest_itl_partial_absorber_itv_quantifier_check_length_complementarity_4_bicremental_types, T, bicremental_types)
{                             itv_quantifier_check_length_complementarity_4_bicremental_types<T, double, partial_absorber, interval_map>();}



#endif // BOOST_ICL_TEST_PARTIAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090701

