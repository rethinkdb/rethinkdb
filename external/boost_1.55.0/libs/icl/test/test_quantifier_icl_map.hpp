/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_QUANTIFIER_ICL_MAP_HPP_JOFA_090119
#define LIBS_ICL_TEST_TEST_QUANTIFIER_ICL_MAP_HPP_JOFA_090119


//------------------------------------------------------------------------------
// Monoid EAN
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair1, map_pair2);
}


template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, map_pair1, map_pair2);
}

//------------------------------------------------------------------------------
// Abelian monoid EANC
//------------------------------------------------------------------------------

template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair1, map_pair2);
}


template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_monoid_et_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(et)   (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(caret)(map_a, map_b, map_c, map_pair1, map_pair2);
}


//------------------------------------------------------------------------------
// Abelian partial invertive monoid 
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_partial_invertive_monoid_plus_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair1, map_pair2);
}

//------------------------------------------------------------------------------
// Abelian partial invertive monoid with distinct equality for inversion
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_partial_invertive_monoid_plus_prot_inv_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus)(map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe)(map_a, map_b, map_c, map_pair1, map_pair2);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(plus)(is_distinct_equal, map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT_EQUAL(pipe)(is_distinct_equal, map_a, map_b, map_c, map_pair1, map_pair2);
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
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_ABELIAN_GROUP_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair1, map_pair2);
}

//------------------------------------------------------------------------------
// (0 - x) + x =d= 0  |  
//------------------------------------------------------------------------------
template <class T, class U, class Trt,
          ICL_IntervalMap_TEMPLATE(_T,_U,Traits,Trt) IntervalMap>
void itv_quantifier_check_abelian_group_plus_prot_inv_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a, itv_map_b, itv_map_c;
    itv_map_a.add(IDv(3,6,1)).add(IIv(5,7,1));
    itv_map_b.add(CDv(1,3,1)).add(IDv(8,9,1));
    itv_map_c.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    MapT map_a, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);

    typename MapT::value_type map_pair1 = sK_v(5,1);
    typename MapT::value_type map_pair2 = sK_v(9,3);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (map_a, map_b, map_c, map_pair1, map_pair2);

#if !defined(_MSC_VER) || (_MSC_VER >= 1400) // 1310==MSVC-7.1  1400 ==MSVC-8.0
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(plus) (is_distinct_equal, map_a, map_b, map_c, map_pair1, map_pair2);
    CHECK_ABELIAN_GROUP_INSTANCE_WRT_EQUAL(pipe) (is_distinct_equal, map_a, map_b, map_c, map_pair1, map_pair2);
#endif
}

#endif // LIBS_ICL_TEST_TEST_QUANTIFIER_ICL_MAP_HPP_JOFA_090119

