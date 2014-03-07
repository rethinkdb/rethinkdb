/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_FASTEST_PARTIAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819
#define BOOST_ICL_FASTEST_PARTIAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819

//------------------------------------------------------------------------------
// partial_absorber
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_monoid_plus_4_bicremental_types)
{                    icl_quantifier_check_monoid_plus_4_bicremental_types<bicremental_type_1, std::string, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_monoid_et_4_bicremental_types)
{                    icl_quantifier_check_monoid_et_4_bicremental_types<bicremental_type_2, int, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_abelian_monoid_plus_4_bicremental_types)
{                    icl_quantifier_check_abelian_monoid_plus_4_bicremental_types<bicremental_type_3, std::string, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_abelian_monoid_et_4_bicremental_types)
{                    icl_quantifier_check_abelian_monoid_et_4_bicremental_types<bicremental_type_4, int, partial_absorber, INTERVAL_MAP>();}

// x - x = 0 | partial absorber
BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types)
{                    icl_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types<bicremental_type_5, int, partial_absorber, INTERVAL_MAP>();}

//------------------------------------------------------------------------------
// partial_enricher
//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_monoid_plus_4_bicremental_types)
{                             icl_quantifier_check_monoid_plus_4_bicremental_types<bicremental_type_6, std::string, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_monoid_et_4_bicremental_types)
{                             icl_quantifier_check_monoid_et_4_bicremental_types<bicremental_type_7, int, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_abelian_monoid_plus_4_bicremental_types)
{                             icl_quantifier_check_abelian_monoid_plus_4_bicremental_types<bicremental_type_8, std::string, partial_enricher, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_abelian_monoid_et_4_bicremental_types)
{                             icl_quantifier_check_abelian_monoid_et_4_bicremental_types<bicremental_type_1, int, partial_enricher, INTERVAL_MAP>();}

// x - x =d= 0 | partial enricher
BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types)
{                             icl_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types<bicremental_type_2, int, partial_enricher, INTERVAL_MAP>();}

//            absorber      enricher
// partial    x - x == 0    x - x =d= 0   partiality of subtraction   
// total    (-x)+ x == 0  (-x)+ x =d= 0   totality   of subtraction

//------------------------------------------------------------------------------
// Containedness
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_icl_quantifier_check_containedness_4_bicremental_types)
{                    icl_quantifier_check_containedness_4_bicremental_types<bicremental_type_1, mono, partial_absorber, INTERVAL_MAP>();}

BOOST_AUTO_TEST_CASE
(fastest_itl_partial_enricher_icl_quantifier_check_containedness_4_bicremental_types)
{                             icl_quantifier_check_containedness_4_bicremental_types<bicremental_type_1, mono, partial_enricher, INTERVAL_MAP>();}


#endif // BOOST_ICL_FASTEST_PARTIAL_ICL_QUANTIFIER_CASES_HPP_JOFA_100819

