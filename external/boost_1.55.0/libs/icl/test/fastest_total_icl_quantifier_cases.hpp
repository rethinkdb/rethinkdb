/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_FASTEST_TOTAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090703
#define BOOST_ICL_FASTEST_TOTAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090703

//------------------------------------------------------------------------------
// total_absorber
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_monoid_plus_4_bicremental_types)
{                  icl_quantifier_check_monoid_plus_4_bicremental_types<bicremental_type_1, std::string, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_monoid_et_4_bicremental_types)
{                  icl_quantifier_check_monoid_et_4_bicremental_types<bicremental_type_2, int, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_abelian_monoid_plus_4_bicremental_types)
{                  icl_quantifier_check_abelian_monoid_plus_4_bicremental_types<bicremental_type_3, std::string, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_abelian_monoid_et_4_bicremental_types)
{                  icl_quantifier_check_abelian_monoid_et_4_bicremental_types<bicremental_type_4, float, total_absorber, INTERVAL_MAP>();}

// (0-x) + x = 0 | total absorber
BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_abelian_group_plus_4_bicremental_domain_and_discrete_codomain)
{                  icl_quantifier_check_abelian_group_plus_4_bicremental_types<bicremental_type_5, int, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_abelian_group_plus_4_bicremental_domain_and_continuous_codomain_1)
{                  icl_quantifier_check_abelian_group_plus_4_bicremental_types<bicremental_type_5, double, total_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_icl_quantifier_check_abelian_group_plus_4_bicremental_domain_and_continuous_codomain_2)
{                  icl_quantifier_check_abelian_group_plus_4_bicremental_types<bicremental_type_5, boost::rational<int>, total_absorber, INTERVAL_MAP>();}

//------------------------------------------------------------------------------
// total_enricher
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_monoid_plus_4_bicremental_types)
{                           icl_quantifier_check_monoid_plus_4_bicremental_types<bicremental_type_6, std::string, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_monoid_et_4_bicremental_types)
{                           icl_quantifier_check_monoid_et_4_bicremental_types<bicremental_type_7, int, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_abelian_monoid_plus_4_bicremental_types)
{                           icl_quantifier_check_abelian_monoid_plus_4_bicremental_types<bicremental_type_8, std::string, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_abelian_monoid_et_4_bicremental_types)
{                           icl_quantifier_check_abelian_monoid_et_4_bicremental_types<bicremental_type_1, double, total_enricher, INTERVAL_MAP>();}

// (0-x) + x =d= 0 | total absorber
BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_discrete_codomain)
{                           icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<bicremental_type_2, int, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_continuous_codomain_1)
{                           icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<bicremental_type_3, float, total_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_total_enricher_icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_domain_continuous_codomain_2)
{                           icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types<bicremental_type_4, boost::rational<int>, total_enricher, INTERVAL_MAP>();}


//            absorber      enricher
// partial    x - x == 0    x - x =d= 0   partiality of subtraction   
// total    (-x)+ x == 0  (-x)+ x =d= 0   totality   of subtraction




#endif // BOOST_ICL_FASTEST_TOTAL_INTERVAL_QUANTIFIER_CASES_HPP_JOFA_090703

