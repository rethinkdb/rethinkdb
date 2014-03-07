/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_ICL_QUANTIFIER_SHARED_HPP_JOFA_100819
#define LIBS_ICL_TEST_TEST_ICL_QUANTIFIER_SHARED_HPP_JOFA_100819

#include "portability.hpp"

template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void make_3_icl_maps_and_derivatives_1
                   (icl::map<T,U,Trt>& map_a, 
                    icl::map<T,U,Trt>& map_b, 
                    icl::map<T,U,Trt>& map_c, 
                    std::pair<T,U>& map_pair_a,
                    std::pair<T,U>& map_pair_b,
                    ICL_PORT_msvc_7_1_IntervalMap(T,U,Trt)*)
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef    icl::map<T,U,Trt>         MapT;

    map_pair_a = sK_v(5,1);
    map_pair_b = sK_v(9,1);

    add(map_a, sK_v(3,1));
    add(map_a, sK_v(4,1));
    add(map_a, sK_v(5,1));
    add(map_a, sK_v(5,1));
    add(map_a, sK_v(6,1));
    add(map_a, sK_v(7,1));

    add(map_b, sK_v(2,1));
    add(map_b, sK_v(8,1));

    add(map_c, sK_v(0,2));
    add(map_c, sK_v(1,2));
    add(map_c, sK_v(2,2));
    add(map_c, sK_v(3,2));
    add(map_c, sK_v(4,2));
    add(map_c, sK_v(5,2));
    add(map_c, sK_v(6,2));
    add(map_c, sK_v(7,2));
    add(map_c, sK_v(8,2));
               
    add(map_c, sK_v(3,1));
    add(map_c, sK_v(4,1));
    add(map_c, sK_v(5,1));
    add(map_c, sK_v(6,1));

    add(map_c, sK_v(5,1));
    add(map_c, sK_v(6,1));
}


//------------------------------------------------------------------------------
// Monoid EAN
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair_a, map_pair_b);
}


template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, map_pair_a, map_pair_b);
}

//------------------------------------------------------------------------------
// Abelian monoid EANC
//------------------------------------------------------------------------------

template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_abelian_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair_a, map_pair_b);
}


template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_abelian_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, map_pair_a, map_pair_b);
}


//------------------------------------------------------------------------------
// Abelian partial invertive monoid 
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair_a, map_pair_b);
}

//------------------------------------------------------------------------------
// Abelian partial invertive monoid with distinct equality for inversion
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus)(map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe)(map_a, map_b, map_c, map_pair_a, map_pair_b);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(plus)(is_distinct_equal, map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(pipe)(is_distinct_equal, map_a, map_b, map_c, map_pair_a, map_pair_b);
#endif
}


//------------------------------------------------------------------------------
// Abelian group EANIC
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_abelian_group_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_ABELIAN_GROUP_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair_a, map_pair_b);
}

//------------------------------------------------------------------------------
// (0 - x) + x =d= 0 
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types()
{
    // check abelian group wrt. + and inverability wrt. distinct equality =d= :
    // (1) (IntervalMapT, +) is an abelian group and
    // (2) The inverability law: (0 - x) + x =d= 0 holds.
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair_a, map_pair_b);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(plus) (is_distinct_equal, map_a, map_b, map_c, map_pair_a, map_pair_b);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(pipe) (is_distinct_equal, map_a, map_b, map_c, map_pair_a, map_pair_b);
#endif
}

//------------------------------------------------------------------------------
// Containedness
//------------------------------------------------------------------------------
template 
<
    class T, class U, class Trt, 
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,Trt) IntervalMap
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap
#endif
>
void icl_quantifier_check_containedness_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt>    MapT;
    IntervalMapT aux;
    MapT map_a, map_b, map_c;
    std::pair<T,U> map_pair_a, map_pair_b;
    make_3_icl_maps_and_derivatives_1(map_a, map_b, map_c, map_pair_a, map_pair_b, &aux);

    check_intersection_containedness(map_a, map_c);
    check_intersection_containedness(map_c, map_pair_a);

    check_union_containedness(map_a, map_c);
    check_union_containedness(map_c, map_pair_a);

    check_domain_containedness(map_a);
}


#endif // LIBS_ICL_TEST_TEST_ICL_QUANTIFIER_SHARED_HPP_JOFA_100819

