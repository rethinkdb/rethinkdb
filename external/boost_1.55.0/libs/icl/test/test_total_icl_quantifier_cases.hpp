/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_TOTAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819
#define BOOST_ICL_TEST_TOTAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819

//------------------------------------------------------------------------------
// total_absorber
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{               itv_quantifier_check_monoid_plus_4_bicremental_types<T, std::string, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_monoid_et_4_bicremental_types, T, bicremental_types)
{               itv_quantifier_check_monoid_et_4_bicremental_types<T, double, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{               itv_quantifier_check_abelian_monoid_plus_4_bicremental_types<T, std::string, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{               itv_quantifier_check_abelian_monoid_et_4_bicremental_types<T, int, total_absorber, INTERVAL_MAP>();}

// (0-x) + x = 0 | total absorber
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_abelian_group_plus_4_bicremental_domain_and_discrete_codomain, T, bicremental_types)
{               itv_quantifier_check_abelian_group_plus_4_bicremental_types<T, int, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_abelian_group_plus_4_bicremental_domain_and_continuous_codomain_1, T, bicremental_types)
{               itv_quantifier_check_abelian_group_plus_4_bicremental_types<T, float, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_itv_quantifier_check_abelian_group_plus_4_bicremental_domain_and_continuous_codomain_2, T, bicremental_types)
{               itv_quantifier_check_abelian_group_plus_4_bicremental_types<T, boost::rational<int>, total_absorber, INTERVAL_MAP>();}

//------------------------------------------------------------------------------
// total_enricher
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_monoid_plus_4_bicremental_types, T, bicremental_types)
{                        itv_quantifier_check_monoid_plus_4_bicremental_types<T, std::string, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_monoid_et_4_bicremental_types, T, bicremental_types)
{                        itv_quantifier_check_monoid_et_4_bicremental_types<T, int, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_abelian_monoid_plus_4_bicremental_types, T, bicremental_types)
{                        itv_quantifier_check_abelian_monoid_plus_4_bicremental_types<T, std::string, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_abelian_monoid_et_4_bicremental_types, T, bicremental_types)
{                        itv_quantifier_check_abelian_monoid_et_4_bicremental_types<T, float, total_enricher, INTERVAL_MAP>();}

// (0-x) + x =d= 0 | total absorber
BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_and_discrete_codomain, T, bicremental_types)
{                        itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<T, int, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_and_continuous_codomain_1, T, bicremental_types)
{                        itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<T, double, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE_TEMPLATE
(test_itl_total_enricher_itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_and_continuous_codomain_2, T, bicremental_types)
{                        itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<T, boost::rational<int>, total_enricher, INTERVAL_MAP>();}


//            absorber      enricher
// partial    x - x == 0    x - x =d= 0   partiality of subtraction   
// total    (-x)+ x == 0  (-x)+ x =d= 0   totality   of subtraction

#endif // BOOST_ICL_TEST_TOTAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819

