/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_ICL_MAP_HPP_JOFA_090119
#define LIBS_ICL_TEST_TEST_ICL_MAP_HPP_JOFA_090119

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
void icl_map_contains_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;
    typedef std::set<T> SetT;
    typedef typename MapT::element_type map_element_type;

    IntervalMapT itv_map_a;
    itv_map_a.add(IDv(2,4,1)).add(IIv(6,7,3));

    MapT map_a;
    segmental::atomize(map_a, itv_map_a);

    BOOST_CHECK_EQUAL( contains(map_a, MK_v(3)), true );
    BOOST_CHECK_EQUAL( within(MK_v(3), map_a),   true );

    map_element_type key_value_pair(MK_v(6), MK_u(3));
    BOOST_CHECK( contains(map_a, key_value_pair) );
    BOOST_CHECK( within(key_value_pair, map_a) );

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
void icl_map_find_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;

    IntervalMapT itv_map_a;
    itv_map_a.add(IDv(2,4,1)).add(IIv(6,7,3));

    MapT map_a;
    segmental::atomize(map_a, itv_map_a);

    typename MapT::const_iterator found = map_a.find(MK_v(6));

    BOOST_CHECK_EQUAL( found->second, MK_u(3) );
    BOOST_CHECK_EQUAL( map_a(MK_v(6)), MK_u(3) );

    found = map_a.find(MK_v(5));

    BOOST_CHECK_EQUAL( found == map_a.end(), true );
    BOOST_CHECK_EQUAL( map_a(MK_v(5)), MK_u(0) );
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
void icl_map_inclusion_compare_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef icl::map<T,U,Trt> MapT;
    typedef std::set<T> SetT;

    IntervalMapT itv_map_sub_a, itv_map_a, itv_map_super_a, 
                 itv_map_b, itv_map_c;
    itv_map_sub_a.add(IDv(2,4,1)).add(IIv(6,7,3));
    itv_map_a = itv_map_sub_a; 
    itv_map_a.add(IIv(9,9,1));
    itv_map_c = itv_map_sub_a;
    itv_map_c.erase(MK_v(7)).add(IIv(11,11,2));
    itv_map_b = itv_map_a;
    itv_map_b.set(IIv(6,7,2));

    MapT map_sub_a, map_a, map_a2, map_b, map_c;
    segmental::atomize(map_a, itv_map_a);
    segmental::atomize(map_b, itv_map_b);
    segmental::atomize(map_c, itv_map_c);
    segmental::atomize(map_sub_a, itv_map_sub_a);

    map_a2 = map_a;
    BOOST_CHECK_EQUAL( inclusion_compare(MapT(), MapT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(map_a, map_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(map_a, map_a2), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(map_a, MapT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(map_a, map_sub_a), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(MapT(), map_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(map_sub_a, map_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(map_a, map_b), inclusion::unrelated );
    BOOST_CHECK_EQUAL( inclusion_compare(map_a, map_c), inclusion::unrelated );

    SetT set_sub_a, set_a, set_a2, set_b, set_c;
    icl::domain(set_a,     map_a);
    icl::domain(set_a2,    map_a2);
    icl::domain(set_sub_a, map_sub_a);

    BOOST_CHECK_EQUAL( inclusion_compare(MapT(), SetT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(SetT(), MapT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(SetT(), SetT()), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(map_a, set_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, map_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, set_a2), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(map_a, SetT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(map_a, set_sub_a), inclusion::superset );

    BOOST_CHECK_EQUAL( inclusion_compare(SetT(), map_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_sub_a, map_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(set_a, SetT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, set_sub_a), inclusion::superset );

    BOOST_CHECK_EQUAL( inclusion_compare(SetT(), set_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_sub_a, set_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(set_a, map_c), inclusion::unrelated );
    BOOST_CHECK_EQUAL( inclusion_compare(map_c, set_a), inclusion::unrelated );

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
void icl_map_move_4_discrete_types()
{
    typedef icl::map<T,U,Trt> MapT;
    typedef typename MapT::element_type map_element_type;

    map_element_type kv_6_3(MK_v(6), MK_u(3));

    MapT map_b;
    map_b.add(kv_6_3);
    MapT mv_cons_1(boost::move(MapT().add(kv_6_3)));
    MapT cp_cons_1(map_b);

    BOOST_CHECK_EQUAL( mv_cons_1, cp_cons_1 );
    BOOST_CHECK_EQUAL( map_b, cp_cons_1 );

    MapT mv_ass_1, cp_ass_1;
    mv_ass_1 = boost::move(MapT().add(kv_6_3));
    cp_ass_1 = map_b;

    BOOST_CHECK_EQUAL( mv_ass_1, cp_ass_1 );
    BOOST_CHECK_EQUAL( map_b, cp_ass_1 );
}


#endif // LIBS_ICL_TEST_TEST_ICL_MAP_HPP_JOFA_090119

