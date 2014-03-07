/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_QUANTIFIER_MAP_SHARED_HPP_JOFA_090119
#define LIBS_ICL_TEST_TEST_QUANTIFIER_MAP_SHARED_HPP_JOFA_090119


//------------------------------------------------------------------------------
// Monoid EAN
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, val_pair, map_pair);
}


template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, val_pair, map_pair);
}

//------------------------------------------------------------------------------
// Abelian monoid EANC
//------------------------------------------------------------------------------

template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, val_pair, map_pair);
}


template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, val_pair, map_pair);
}


//------------------------------------------------------------------------------
// Abelian partial invertive monoid 
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, val_pair, map_pair);
}

//------------------------------------------------------------------------------
// Abelian partial invertive monoid with distinct equality for inversion
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus)(map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe)(map_a, map_b, map_c, val_pair, map_pair);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(plus)(is_distinct_equal, map_a, map_b, map_c, val_pair, map_pair);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(pipe)(is_distinct_equal, map_a, map_b, map_c, val_pair, map_pair);
#endif
}


//------------------------------------------------------------------------------
// Abelian group EANIC
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_group_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_ABELIAN_GROUP_INSTANCE_WRT(plus) (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT(pipe) (map_a, map_b, map_c, val_pair, map_pair);
}

//------------------------------------------------------------------------------
// (0 - x) + x =d= 0  |  
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;

    IntervalMapT map_a, map_b, map_c;
    map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    typename IntervalMapT::interval_mapping_type val_pair = IDv(6,9,1);
    mapping_pair<T,U> map_pair = K_v(5,1);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, val_pair, map_pair);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(plus) (is_distinct_equal, map_a, map_b, map_c, val_pair, map_pair);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(pipe) (is_distinct_equal, map_a, map_b, map_c, val_pair, map_pair);
#endif
}

#endif // LIBS_ICL_TEST_TEST_QUANTIFIER_MAP_SHARED_HPP_JOFA_090119

