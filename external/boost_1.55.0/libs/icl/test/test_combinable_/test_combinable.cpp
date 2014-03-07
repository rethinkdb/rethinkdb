/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#define BOOST_TEST_MODULE icl::casual unit test
#include <libs/icl/test/disable_test_warnings.hpp>
#include <string>
#include <boost/mpl/list.hpp>
#include "../unit_test_unwarned.hpp"


// interval instance types
#include "../test_type_lists.hpp"
#include "../test_value_maker.hpp"

#include <boost/icl/interval_set.hpp>
#include <boost/icl/separate_interval_set.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost;
using namespace unit_test;
using namespace boost::icl;


template<template<class, class>class IsCombinable,
         class LeftT, class RightT>
void check_combinable(bool expected, const char* type_combi, const char* combi_text)
{
    std::string type_combination = type_combi;
    std::string is_combi_text = combi_text;
    bool is_combinable = IsCombinable<LeftT,RightT>::value;
    std::string combination_result = is_combinable 
        ? (is_combinable == expected ? type_combination : "expected: NOT "+is_combi_text+"<"+type_combination+">")
        : (is_combinable == expected ? type_combination : "expected:  IS "+is_combi_text+"<"+type_combination+">");

    //BOOST_CHECK_EQUAL(expected, is_combinable);
    BOOST_CHECK_EQUAL(type_combination, combination_result);
}

template<template<class, class>class IsCombinable>
void check_combine_pattern(const char* text,
    bool jS_e, bool jS_i, bool jS_b, bool jS_p, bool jS_jS, bool jS_zS, bool jS_sS, bool jS_jM, bool jS_sM, 
    bool zS_e, bool zS_i, bool zS_b, bool zS_p, bool zS_jS, bool zS_zS, bool zS_sS, bool zS_jM, bool zS_sM, 
    bool sS_e, bool sS_i, bool sS_b, bool sS_p, bool sS_jS, bool sS_zS, bool sS_sS, bool sS_jM, bool sS_sM, 
    bool jM_e, bool jM_i, bool jM_b, bool jM_p, bool jM_jS, bool jM_zS, bool jM_sS, bool jM_jM, bool jM_sM, 
    bool sM_e, bool sM_i, bool sM_b, bool sM_p, bool sM_jS, bool sM_zS, bool sM_sS, bool sM_jM, bool sM_sM,
    bool check_base_class = true
    )
{
    typedef interval_set<int>                jS;
    typedef separate_interval_set<int>       zS;
    typedef split_interval_set<int>          sS;
    typedef interval_map<int,double>         jM;
    typedef split_interval_map<int,double>   sM;

    typedef interval_base_set<jS,int>        jT;
    typedef interval_base_set<zS,int>        zT;
    typedef interval_base_set<sS,int>        sT;
    typedef interval_base_map<jM,int,double> jN;
    typedef interval_base_map<sM,int,double> sN;

    typedef interval_set<int>::element_type  S_e;
    typedef interval_set<int>::segment_type  S_i;

    typedef interval_map<int,double>::element_type M_b;
    typedef interval_map<int,double>::segment_type M_p;

    //--------------------------------------------------------------------------
    check_combinable<IsCombinable, jS, S_e>(jS_e , "jS_e ", text);
    check_combinable<IsCombinable, jS, S_i>(jS_i , "jS_i ", text);
    check_combinable<IsCombinable, jS, M_b>(jS_b , "jS_b ", text);
    check_combinable<IsCombinable, jS, M_p>(jS_p , "jS_p ", text);
    check_combinable<IsCombinable, jS, jS >(jS_jS, "jS_jS", text);
    check_combinable<IsCombinable, jS, zS >(jS_zS, "jS_zS", text);
    check_combinable<IsCombinable, jS, sS >(jS_sS, "jS_sS", text);
    check_combinable<IsCombinable, jS, jM >(jS_jM, "jS_jM", text);
    check_combinable<IsCombinable, jS, sM >(jS_sM, "jS_sM", text);
    //--------------------------------------------------------------------------
    check_combinable<IsCombinable, zS, S_e>(zS_e , "zS_e ", text);
    check_combinable<IsCombinable, zS, S_i>(zS_i , "zS_i ", text);
    check_combinable<IsCombinable, zS, M_b>(zS_b , "zS_b ", text);
    check_combinable<IsCombinable, zS, M_p>(zS_p , "zS_p ", text);
    check_combinable<IsCombinable, zS, jS >(zS_jS, "zS_jS", text);
    check_combinable<IsCombinable, zS, zS >(zS_zS, "zS_zS", text);
    check_combinable<IsCombinable, zS, sS >(zS_sS, "zS_sS", text);
    check_combinable<IsCombinable, zS, jM >(zS_jM, "zS_jM", text);
    check_combinable<IsCombinable, zS, sM >(zS_sM, "zS_sM", text);
    //--------------------------------------------------------------------------
    check_combinable<IsCombinable, sS, S_e>(sS_e , "sS_e ", text);
    check_combinable<IsCombinable, sS, S_i>(sS_i , "sS_i ", text);
    check_combinable<IsCombinable, sS, M_b>(sS_b , "sS_b ", text);
    check_combinable<IsCombinable, sS, M_p>(sS_p , "sS_p ", text);
    check_combinable<IsCombinable, sS, jS >(sS_jS, "sS_jS", text);
    check_combinable<IsCombinable, sS, zS >(sS_zS, "sS_zS", text);
    check_combinable<IsCombinable, sS, sS >(sS_sS, "sS_sS", text);
    check_combinable<IsCombinable, sS, jM >(sS_jM, "sS_jM", text);
    check_combinable<IsCombinable, sS, sM >(sS_sM, "sS_sM", text);
    //--------------------------------------------------------------------------
    check_combinable<IsCombinable, jM, S_e>(jM_e , "jM_e ", text);
    check_combinable<IsCombinable, jM, S_i>(jM_i , "jM_i ", text);
    check_combinable<IsCombinable, jM, M_b>(jM_b , "jM_b ", text);
    check_combinable<IsCombinable, jM, M_p>(jM_p , "jM_p ", text);
    check_combinable<IsCombinable, jM, jS >(jM_jS, "jM_jS", text);
    check_combinable<IsCombinable, jM, zS >(jM_zS, "jM_zS", text);
    check_combinable<IsCombinable, jM, sS >(jM_sS, "jM_sS", text);
    check_combinable<IsCombinable, jM, jM >(jM_jM, "jM_jM", text);
    check_combinable<IsCombinable, jM, sM >(jM_sM, "jM_sM", text);
    //--------------------------------------------------------------------------
    check_combinable<IsCombinable, sM, S_e>(sM_e , "sM_e ", text);
    check_combinable<IsCombinable, sM, S_i>(sM_i , "sM_i ", text);
    check_combinable<IsCombinable, sM, M_b>(sM_b , "sM_b ", text);
    check_combinable<IsCombinable, sM, M_p>(sM_p , "sM_p ", text);
    check_combinable<IsCombinable, sM, jS >(sM_jS, "sM_jS", text);
    check_combinable<IsCombinable, sM, zS >(sM_zS, "sM_zS", text);
    check_combinable<IsCombinable, sM, sS >(sM_sS, "sM_sS", text);
    check_combinable<IsCombinable, sM, jM >(sM_jM, "sM_jM", text);
    check_combinable<IsCombinable, sM, sM >(sM_sM, "sM_sM", text);
    //--------------------------------------------------------------------------

    if(check_base_class)
    {
        //--------------------------------------------------------------------------
        check_combinable<IsCombinable, jT, S_e>(jS_e , "jT_e ", text);
        check_combinable<IsCombinable, jT, S_i>(jS_i , "jT_i ", text);
        check_combinable<IsCombinable, jT, M_b>(jS_b , "jT_b ", text);
        check_combinable<IsCombinable, jT, M_p>(jS_p , "jT_p ", text);
        check_combinable<IsCombinable, jT, jS >(jS_jS, "jT_jS", text);
        check_combinable<IsCombinable, jT, zS >(jS_zS, "jT_zS", text);
        check_combinable<IsCombinable, jT, sS >(jS_sS, "jT_sS", text);
        check_combinable<IsCombinable, jT, jM >(jS_jM, "jT_jM", text);
        check_combinable<IsCombinable, jT, sM >(jS_sM, "jT_sM", text);
        check_combinable<IsCombinable, jT, jT >(jS_jS, "jT_jT", text);
        check_combinable<IsCombinable, jT, zT >(jS_zS, "jT_zT", text);
        check_combinable<IsCombinable, jT, sT >(jS_sS, "jT_sT", text);
        check_combinable<IsCombinable, jT, jN >(jS_jM, "jT_jN", text);
        check_combinable<IsCombinable, jT, sN >(jS_sM, "jT_sN", text);
        //--------------------------------------------------------------------------
        check_combinable<IsCombinable, zT, S_e>(zS_e , "zT_e ", text);
        check_combinable<IsCombinable, zT, S_i>(zS_i , "zT_i ", text);
        check_combinable<IsCombinable, zT, M_b>(zS_b , "zT_b ", text);
        check_combinable<IsCombinable, zT, M_p>(zS_p , "zT_p ", text);
        check_combinable<IsCombinable, zT, jS >(zS_jS, "zT_jS", text);
        check_combinable<IsCombinable, zT, zS >(zS_zS, "zT_zS", text);
        check_combinable<IsCombinable, zT, sS >(zS_sS, "zT_sS", text);
        check_combinable<IsCombinable, zT, jM >(zS_jM, "zT_jM", text);
        check_combinable<IsCombinable, zT, sM >(zS_sM, "zT_sM", text);
        check_combinable<IsCombinable, zT, jT >(zS_jS, "zT_jT", text);
        check_combinable<IsCombinable, zT, zT >(zS_zS, "zT_zT", text);
        check_combinable<IsCombinable, zT, sT >(zS_sS, "zT_sT", text);
        check_combinable<IsCombinable, zT, jN >(zS_jM, "zT_jN", text);
        check_combinable<IsCombinable, zT, sN >(zS_sM, "zT_sN", text);
        //--------------------------------------------------------------------------
        check_combinable<IsCombinable, sT, S_e>(sS_e , "sT_e ", text);
        check_combinable<IsCombinable, sT, S_i>(sS_i , "sT_i ", text);
        check_combinable<IsCombinable, sT, M_b>(sS_b , "sT_b ", text);
        check_combinable<IsCombinable, sT, M_p>(sS_p , "sT_p ", text);
        check_combinable<IsCombinable, sT, jS >(sS_jS, "sT_jS", text);
        check_combinable<IsCombinable, sT, zS >(sS_zS, "sT_zS", text);
        check_combinable<IsCombinable, sT, sS >(sS_sS, "sT_sS", text);
        check_combinable<IsCombinable, sT, jM >(sS_jM, "sT_jM", text);
        check_combinable<IsCombinable, sT, sM >(sS_sM, "sT_sM", text);
        check_combinable<IsCombinable, sT, jT >(sS_jS, "sT_jT", text);
        check_combinable<IsCombinable, sT, zT >(sS_zS, "sT_zT", text);
        check_combinable<IsCombinable, sT, sT >(sS_sS, "sT_sT", text);
        check_combinable<IsCombinable, sT, jN >(sS_jM, "sT_jN", text);
        check_combinable<IsCombinable, sT, sN >(sS_sM, "sT_sN", text);
        //--------------------------------------------------------------------------
        check_combinable<IsCombinable, jN, S_e>(jM_e , "jN_e ", text);
        check_combinable<IsCombinable, jN, S_i>(jM_i , "jN_i ", text);
        check_combinable<IsCombinable, jN, M_b>(jM_b , "jN_b ", text);
        check_combinable<IsCombinable, jN, M_p>(jM_p , "jN_p ", text);
        check_combinable<IsCombinable, jN, jS >(jM_jS, "jN_jS", text);
        check_combinable<IsCombinable, jN, zS >(jM_zS, "jN_zS", text);
        check_combinable<IsCombinable, jN, sS >(jM_sS, "jN_sS", text);
        check_combinable<IsCombinable, jN, jM >(jM_jM, "jN_jM", text);//
        check_combinable<IsCombinable, jN, sM >(jM_sM, "jN_sM", text);//
        check_combinable<IsCombinable, jN, jT >(jM_jS, "jN_jT", text);
        check_combinable<IsCombinable, jN, zT >(jM_zS, "jN_zT", text);
        check_combinable<IsCombinable, jN, sT >(jM_sS, "jN_sT", text);
        check_combinable<IsCombinable, jN, jN >(jM_jM, "jN_jN", text);//
        check_combinable<IsCombinable, jN, sN >(jM_sM, "jN_sN", text);//
        //--------------------------------------------------------------------------
        check_combinable<IsCombinable, sN, S_e>(sM_e , "sN_e ", text);
        check_combinable<IsCombinable, sN, S_i>(sM_i , "sN_i ", text);
        check_combinable<IsCombinable, sN, M_b>(sM_b , "sN_b ", text);
        check_combinable<IsCombinable, sN, M_p>(sM_p , "sN_p ", text);
        check_combinable<IsCombinable, sN, jS >(sM_jS, "sN_jS", text);
        check_combinable<IsCombinable, sN, zS >(sM_zS, "sN_zS", text);
        check_combinable<IsCombinable, sN, sS >(sM_sS, "sN_sS", text);
        check_combinable<IsCombinable, sN, jM >(sM_jM, "sN_jM", text);
        check_combinable<IsCombinable, sN, sM >(sM_sM, "sN_sM", text);
        check_combinable<IsCombinable, sN, jT >(sM_jS, "sN_jT", text);
        check_combinable<IsCombinable, sN, zT >(sM_zS, "sN_zT", text);
        check_combinable<IsCombinable, sN, sT >(sM_sS, "sN_sT", text);
        check_combinable<IsCombinable, sN, jN >(sM_jM, "sN_jN", text);
        check_combinable<IsCombinable, sN, sN >(sM_sM, "sN_sN", text);
    }
}


BOOST_AUTO_TEST_CASE(test_icl_is_derivative)
{
    //--------------------------------------------------------------------------
    // 1.1
    check_combine_pattern<is_intra_derivative>(
        "is_intra_derivative",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 0, 0, // jM
        0, 0, 1, 1, 0, 0, 0, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 1.2
    check_combine_pattern<is_cross_derivative>(
        "is_cross_derivative",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jM
        1, 1, 0, 0, 0, 0, 0, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 1.3
    check_combine_pattern<is_inter_derivative>(
        "is_inter_derivative",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // sS
        1, 1, 1, 1, 0, 0, 0, 0, 0, // jM
        1, 1, 1, 1, 0, 0, 0, 0, 0  // sM
        );
}

BOOST_AUTO_TEST_CASE(test_icl_is_combinable)
{
    //--------------------------------------------------------------------------
    // 2.1
    check_combine_pattern<is_intra_combinable>(
        "is_intra_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 1, 1, 1, 0, 0, // jS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // zS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // sS
        0, 0, 0, 0, 0, 0, 0, 1, 1, // jM
        0, 0, 0, 0, 0, 0, 0, 1, 1  // sM
        );

    //--------------------------------------------------------------------------
    // 2.2
    check_combine_pattern<is_cross_combinable>(
        "is_cross_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 1, 1, // jS
        0, 0, 0, 0, 0, 0, 0, 1, 1, // zS
        0, 0, 0, 0, 0, 0, 0, 1, 1, // sS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // jM
        0, 0, 0, 0, 1, 1, 1, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 2.3
    check_combine_pattern<is_inter_combinable>(
        "is_inter_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 1, 1, 1, 1, 1, // jS
        0, 0, 0, 0, 1, 1, 1, 1, 1, // zS
        0, 0, 0, 0, 1, 1, 1, 1, 1, // sS
        0, 0, 0, 0, 1, 1, 1, 1, 1, // jM
        0, 0, 0, 0, 1, 1, 1, 1, 1  // sM
        );

}

BOOST_AUTO_TEST_CASE(test_icl_is_container_right_combinable)
{
    //--------------------------------------------------------------------------
    // 3.1
    // LeftT is an interval_set: 
    // is_interval_set_right_combinable<LeftT, RightT> determines what can
    // be combined as RightT argument type.
    check_combine_pattern<is_interval_set_right_combinable>(
        "is_interval_set_right_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // sS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jM
        0, 0, 0, 0, 0, 0, 0, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 3.2
    check_combine_pattern<is_interval_map_right_intra_combinable>(
        "is_interval_map_right_intra_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 1, 1, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 1  // sM
        );

    //--------------------------------------------------------------------------
    // 3.3
    check_combine_pattern<is_interval_map_right_cross_combinable>(
        "is_interval_map_right_cross_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jM
        1, 1, 0, 0, 1, 1, 1, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 3.4
    check_combine_pattern<is_interval_map_right_inter_combinable>(
        "is_interval_map_right_inter_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        1, 1, 1, 1, 1, 1, 1, 1, 1, // jM
        1, 1, 1, 1, 1, 1, 1, 1, 1  // sM
        );

}

BOOST_AUTO_TEST_CASE(test_icl_is_right_combinable)
{
    //--------------------------------------------------------------------------
    // 4.1
    check_combine_pattern<is_right_intra_combinable>(
        "is_right_intra_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 1, 1, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 1  // sM
        );

    //--------------------------------------------------------------------------
    // 4.2
    check_combine_pattern<is_right_inter_combinable>(
        "is_right_inter_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // sS
        1, 1, 1, 1, 1, 1, 1, 1, 1, // jM
        1, 1, 1, 1, 1, 1, 1, 1, 1  // sM
        );
}

BOOST_AUTO_TEST_CASE(test_icl_combines_right_to)
{
    //--------------------------------------------------------------------------
    // 5.1
    check_combine_pattern<combines_right_to_interval_set>(
        "combines_right_to_interval_set",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 1, 1, 1, 0, 0, // jS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // zS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // sS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // jM
        0, 0, 0, 0, 1, 1, 1, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 5.2
    check_combine_pattern<combines_right_to_interval_map>(
        "combines_right_to_interval_map",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 0, 0, 0, 0, 0, 1, 1, // jM
        0, 0, 0, 0, 0, 0, 0, 1, 1  // sM
        );

    //--------------------------------------------------------------------------
    // 5.3
    check_combine_pattern<combines_right_to_interval_container>(
        "combines_right_to_interval_container",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 1, 1, 1, 0, 0, // jS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // zS
        0, 0, 0, 0, 1, 1, 1, 0, 0, // sS
        0, 0, 0, 0, 1, 1, 1, 1, 1, // jM
        0, 0, 0, 0, 1, 1, 1, 1, 1  // sM
        );
}

BOOST_AUTO_TEST_CASE(test_icl_is_companion)
{
    //--------------------------------------------------------------------------
    // 6.1
    check_combine_pattern<is_interval_set_companion>(
        "is_interval_set_companion",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // sS
        1, 1, 0, 0, 1, 1, 1, 0, 0, // jM
        1, 1, 0, 0, 1, 1, 1, 0, 0  // sM
        );

    //--------------------------------------------------------------------------
    // 6.2 
    check_combine_pattern<is_interval_map_companion>(
        "is_interval_map_companion",
    //  e  i  b  p jS zS sS jM sM       
        0, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 1, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 1, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 1, 1, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 1  // sM
        );
}

BOOST_AUTO_TEST_CASE(test_icl_is_coarser_combinable)
{
    //--------------------------------------------------------------------------
    // 7.1
    check_combine_pattern<is_coarser_interval_set_companion>(
        "is_coarser_interval_set_companion",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 1, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 0, 0, 0, // sS
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jM
        1, 1, 0, 0, 1, 1, 0, 0, 0, // sM
        false
        );

    //--------------------------------------------------------------------------
    // 7.2
    check_combine_pattern<is_coarser_interval_map_companion>(
        "is_coarser_interval_map_companion",
    //  e  i  b  p jS zS sS jM sM       
        0, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 1, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 1, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 0, 0, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 0, // sM
        false
        );

    //--------------------------------------------------------------------------
    // 8.1
    check_combine_pattern<is_binary_interval_set_combinable>(
        "is_binary_interval_set_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 1, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 0, 0, 0, // sS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jM
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sM
        false
        );

    //--------------------------------------------------------------------------
    // 8.2
    check_combine_pattern<is_binary_interval_map_combinable>(
        "is_binary_interval_map_combinable",
    //  e  i  b  p jS zS sS jM sM       
        0, 0, 0, 0, 0, 0, 0, 0, 0, // jS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // zS
        0, 0, 0, 0, 0, 0, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 0, 0, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 0, // sM
        false
        );
}

BOOST_AUTO_TEST_CASE(test_icl_is_binary_combinable)
{
    //--------------------------------------------------------------------------
    // 9.1
    check_combine_pattern<is_binary_intra_combinable>(
        "is_binary_intra_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 1, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 0, 0, 0, // sS
        0, 0, 1, 1, 0, 0, 0, 0, 0, // jM
        0, 0, 1, 1, 0, 0, 0, 1, 0, // sM
        false
        );

    //--------------------------------------------------------------------------
    // 9.2
    check_combine_pattern<is_binary_inter_combinable>(
        "is_binary_inter_combinable",
    //  e  i  b  p jS zS sS jM sM       
        1, 1, 0, 0, 0, 0, 0, 0, 0, // jS
        1, 1, 0, 0, 1, 0, 0, 0, 0, // zS
        1, 1, 0, 0, 1, 1, 0, 0, 0, // sS
        1, 1, 1, 1, 1, 1, 1, 0, 0, // jM
        1, 1, 1, 1, 1, 1, 1, 1, 0, // sM
        false
        );
}
