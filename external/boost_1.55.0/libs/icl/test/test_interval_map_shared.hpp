/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2012: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_INTERVAL_MAP_SHARED_HPP_JOFA_081005
#define LIBS_ICL_TEST_TEST_INTERVAL_MAP_SHARED_HPP_JOFA_081005

#include "portability.hpp"

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_fundamentals_4_ordered_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::size_type       size_T;
    typedef typename IntervalMapT::difference_type diff_T;

    // ordered types is the largest set of instance types.
    // Because we can not generate values via incrementation for e.g. string,
    // we are able to test operations only for the most basic values
    // identity_element (0, empty, T() ...) and unit_element.

    T v0 = boost::icl::identity_element<T>::value();
    T v1 = unit_element<T>::value();
    IntervalT I0_0I(v0);
    IntervalT I1_1I(v1);
#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
    IntervalT I0_1I(v0, v1, interval_bounds::closed());
#else
    IntervalT I0_1I = icl::interval<T>::closed(v0, v1);
#endif
    U u1 = unit_element<U>::value();

    //-------------------------------------------------------------------------
    //empty set
    //-------------------------------------------------------------------------
    BOOST_CHECK_EQUAL(IntervalMapT().empty(), true);
    BOOST_CHECK_EQUAL(icl::is_empty(IntervalMapT()), true);
    BOOST_CHECK_EQUAL(cardinality(IntervalMapT()), boost::icl::identity_element<size_T>::value());
    BOOST_CHECK_EQUAL(IntervalMapT().size(), boost::icl::identity_element<size_T>::value());
    BOOST_CHECK_EQUAL(icl::size(IntervalMapT()), boost::icl::identity_element<size_T>::value());
    BOOST_CHECK_EQUAL(interval_count(IntervalMapT()), 0);
    BOOST_CHECK_EQUAL(IntervalMapT().iterative_size(), 0);
    BOOST_CHECK_EQUAL(iterative_size(IntervalMapT()), 0);
    BOOST_CHECK_EQUAL(IntervalMapT(), IntervalMapT());

    IntervalT mt_interval = boost::icl::identity_element<IntervalT >::value();
    BOOST_CHECK_EQUAL(mt_interval, IntervalT());
    typename IntervalMapT::value_type mt_u1 = make_pair(mt_interval, u1);
    IntervalMapT mt_map = boost::icl::identity_element<IntervalMapT >::value();
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    //adding emptieness to emptieness yields emptieness ;)
    mt_map.add(mt_u1).add(mt_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    mt_map.insert(mt_u1).insert(mt_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    (mt_map += mt_u1) += mt_u1;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    BOOST_CHECK_EQUAL(hull(mt_map), boost::icl::identity_element<IntervalT >::value());

    //subtracting emptieness
    mt_map.subtract(mt_u1).subtract(mt_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    mt_map.erase(mt_interval).erase(mt_interval);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    (mt_map -= mt_u1) -= mt_u1;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    //subtracting elements form emptieness
    typename IntervalMapT::domain_mapping_type v0_u1 = make_pair(v0,u1);
    typename IntervalMapT::domain_mapping_type v1_u1 = make_pair(v1,u1);
    //mt_map.subtract(make_pair(v0,u1)).subtract(make_pair(v1,u1));
    mt_map.subtract(v0_u1).subtract(v1_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    mt_map.erase(v0_u1).erase(v1_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    (mt_map -= v0_u1) -= v1_u1;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    //subtracting intervals form emptieness
    typename IntervalMapT::segment_type I0_0I_u1 = make_pair(I0_0I,u1);
    typename IntervalMapT::segment_type I0_1I_u1 = make_pair(I0_1I,u1);
    typename IntervalMapT::segment_type I1_1I_u1 = make_pair(I1_1I,u1);
    mt_map.subtract(I0_1I_u1).subtract(I1_1I_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    mt_map.erase(I0_1I_u1).erase(I1_1I_u1);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    (mt_map -= I0_1I_u1) -= I1_1I_u1;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    mt_map.erase(I0_1I).erase(I1_1I);
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    //insecting emptieness
    (mt_map &= mt_u1) &= mt_u1;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());
    (mt_map &= mt_interval) &= mt_interval;
    BOOST_CHECK_EQUAL(mt_map, IntervalMapT());

    

    //-------------------------------------------------------------------------
    //unary set
    //-------------------------------------------------------------------------
    IntervalMapT single_I0_0I_u1_from_element(v0_u1);
    IntervalMapT single_I0_0I_u1_from_interval(I0_0I_u1);
    IntervalMapT single_I0_0I_u1(single_I0_0I_u1_from_interval);

    BOOST_CHECK_EQUAL(single_I0_0I_u1_from_element, single_I0_0I_u1_from_interval);
    BOOST_CHECK_EQUAL(single_I0_0I_u1_from_element, single_I0_0I_u1);
    BOOST_CHECK_EQUAL(hull(single_I0_0I_u1), I0_0I);
    BOOST_CHECK_EQUAL(hull(single_I0_0I_u1).lower(), I0_0I.lower());
    BOOST_CHECK_EQUAL(hull(single_I0_0I_u1).upper(), I0_0I.upper());

    IntervalMapT single_I1_1I_u1_from_element(v1_u1);
    IntervalMapT single_I1_1I_u1_from_interval(I1_1I_u1);
    IntervalMapT single_I1_1I_u1(single_I1_1I_u1_from_interval);

    BOOST_CHECK_EQUAL(single_I1_1I_u1_from_element, single_I1_1I_u1_from_interval);
    BOOST_CHECK_EQUAL(single_I1_1I_u1_from_element, single_I1_1I_u1);

    IntervalMapT single_I0_1I_u1_from_interval(I0_1I_u1);
    IntervalMapT single_I0_1I_u1(single_I0_1I_u1_from_interval);

    BOOST_CHECK_EQUAL(single_I0_1I_u1_from_interval, single_I0_1I_u1);
    BOOST_CHECK_EQUAL(hull(single_I0_1I_u1), I0_1I);
    BOOST_CHECK_EQUAL(hull(single_I0_1I_u1).lower(), I0_1I.lower());
    BOOST_CHECK_EQUAL(hull(single_I0_1I_u1).upper(), I0_1I.upper());

    //contains predicate
    BOOST_CHECK_EQUAL(icl::contains(single_I0_0I_u1, v0), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_0I_u1, v0_u1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_0I_u1, I0_0I_u1), true);

    BOOST_CHECK_EQUAL(icl::contains(single_I1_1I_u1, v1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I1_1I_u1, v1_u1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I1_1I_u1, I1_1I_u1), true);

    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, v0), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, I0_1I_u1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, v1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, I1_1I_u1), true);

    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, single_I0_0I_u1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, single_I1_1I_u1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I_u1, single_I0_1I_u1), true);

    BOOST_CHECK_EQUAL(cardinality(single_I0_0I_u1), unit_element<size_T>::value());
    BOOST_CHECK_EQUAL(single_I0_0I_u1.size(), unit_element<size_T>::value());
    BOOST_CHECK_EQUAL(interval_count(single_I0_0I_u1), 1);
    BOOST_CHECK_EQUAL(single_I0_0I_u1.iterative_size(), 1);
    BOOST_CHECK_EQUAL(iterative_size(single_I0_0I_u1), 1);

}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_ctor_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    T v4 = make<T>(4);
    U u2 = make<U>(2);
    IntervalT I4_4I(v4);
    typename IntervalMapT::domain_mapping_type v4_u2(v4,u2);
    typename IntervalMapT::value_type I4_4I_u2(I4_4I,u2);

    IntervalMapT _I4_4I_u2;
    BOOST_CHECK_EQUAL( _I4_4I_u2.empty(), true );
    IntervalMapT _I4_4I_u2_1;
    IntervalMapT _I4_4I_u2_2;
    IntervalMapT _I4_4I_u2_3;
    _I4_4I_u2   += v4_u2;
    _I4_4I_u2_1 += I4_4I_u2;
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_1 );
    _I4_4I_u2_2.add(v4_u2);
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_2 );
    _I4_4I_u2_3.add(I4_4I_u2);
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_3 );

    _I4_4I_u2.clear();
    _I4_4I_u2.add(I4_4I_u2).add(I4_4I_u2);
    IntervalMapT _I4_4I_u4(make_pair(I4_4I, make<U>(4)));
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u4 );

    _I4_4I_u2.clear();
    _I4_4I_u2.insert(I4_4I_u2).insert(I4_4I_u2);
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_1 );

    BOOST_CHECK_EQUAL( cardinality(_I4_4I_u2), unit_element<typename IntervalMapT::size_type>::value()  );
    BOOST_CHECK_EQUAL( _I4_4I_u2.size(),       unit_element<typename IntervalMapT::size_type>::value()  );
    BOOST_CHECK_EQUAL( interval_count(_I4_4I_u2),   1  );
    BOOST_CHECK_EQUAL( _I4_4I_u2.iterative_size(),  1  );
    BOOST_CHECK_EQUAL( iterative_size(_I4_4I_u2),   1  );

    if(has_dynamic_bounds<IntervalT>::value)
    {
        BOOST_CHECK_EQUAL( hull(_I4_4I_u2).lower(), v4 );
        BOOST_CHECK_EQUAL( hull(_I4_4I_u2).upper(), v4 );
    }

    IntervalMapT _I4_4I_u2_copy(_I4_4I_u2);
    IntervalMapT _I4_4I_u2_assigned;
    _I4_4I_u2_assigned = _I4_4I_u2;
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_copy );
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_assigned );

    _I4_4I_u2_assigned.clear();
    BOOST_CHECK_EQUAL( true,   _I4_4I_u2_assigned.empty() );

    _I4_4I_u2_assigned.swap(_I4_4I_u2_copy);
    BOOST_CHECK_EQUAL( true,   _I4_4I_u2_copy.empty() );
    BOOST_CHECK_EQUAL( _I4_4I_u2, _I4_4I_u2_assigned );
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_add_sub_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    T v0 = make<T>(0);
    T v5 = make<T>(5);
    T v6 = make<T>(6);
    T v9 = make<T>(9);
    U u1 = make<U>(1);
#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
    IntervalT I5_6I(v5,v6, interval_bounds::closed());
    IntervalT I5_9I(v5,v9, interval_bounds::closed());
    IntervalT I0_9I = IntervalT::closed(v0, v9);
#else
    IntervalT I5_6I = icl::interval<T>::closed(v5,v6); 
    IntervalT I5_9I = icl::interval<T>::closed(v5,v9); 
    IntervalT I0_9I = icl::interval<T>::closed(v0,v9);
#endif
    typename IntervalMapT::domain_mapping_type v0_u1 = make_pair(v0, u1);
    typename IntervalMapT::domain_mapping_type v9_u1 = make_pair(v9, u1);
    typename IntervalMapT::value_type I5_6I_u1 = make_pair(I5_6I, u1);
    typename IntervalMapT::value_type I5_9I_u1 = make_pair(I5_9I, u1);
    typename IntervalMapT::value_type I0_9I_u1 = make_pair(icl::interval<T>::closed(v0, v9), u1);

    BOOST_CHECK_EQUAL( IntervalMapT(I5_6I_u1).add(v0_u1).add(v9_u1), 
                       IntervalMapT().add(v9_u1).add(I5_6I_u1).add(v0_u1) );

    IntervalMapT map_A = IntervalMapT(I5_6I_u1).add(v0_u1).add(v9_u1);
    IntervalMapT map_B = IntervalMapT().insert(v9_u1).insert(I5_6I_u1).insert(v0_u1);
    BOOST_CHECK_EQUAL( map_A, map_B );
    BOOST_CHECK_EQUAL( hull(map_A), I0_9I );
    BOOST_CHECK_EQUAL( hull(map_A).lower(), I0_9I.lower() );
    BOOST_CHECK_EQUAL( hull(map_A).upper(), I0_9I.upper() );

    IntervalMapT map_A1 = map_A, map_B1 = map_B,
                 map_A2 = map_A, map_B2 = map_B;

    map_A1.subtract(I5_6I_u1).subtract(v9_u1);
    map_B1.erase(v9_u1).erase(I5_6I_u1);
    BOOST_CHECK_EQUAL( map_A1, map_B1 );

    map_B1 = map_B;
    map_B2.erase(v9).erase(I5_6I);
    BOOST_CHECK_EQUAL( map_A1, map_B2 );

    map_A2.subtract(I5_9I_u1);
    map_B2.erase(I5_9I);
    BOOST_CHECK_EQUAL( map_A2, map_B2 );
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_distinct_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMap<T,U>::size_type       size_T;
    typedef typename IntervalMap<T,U>::difference_type diff_T;
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    U u1 = make<U>(1);
    typename IntervalMapT::domain_mapping_type v1_u1(v1,u1);
    typename IntervalMapT::domain_mapping_type v3_u1(v3,u1);
    typename IntervalMapT::domain_mapping_type v5_u1(v5,u1);

    size_T s3 = make<size_T>(3);

    IntervalMapT is_1_3_5;
    is_1_3_5.add(v1_u1).add(v3_u1).add(v5_u1);

    BOOST_CHECK_EQUAL( cardinality(is_1_3_5),      s3 );
    BOOST_CHECK_EQUAL( is_1_3_5.size(),             s3 );
    BOOST_CHECK_EQUAL( interval_count(is_1_3_5),   3 );
    BOOST_CHECK_EQUAL( is_1_3_5.iterative_size(),   3 );
    BOOST_CHECK_EQUAL( iterative_size(is_1_3_5),   3 );
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_distinct_4_bicremental_continuous_types()
{
#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS

    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::size_type       size_T;
    typedef typename IntervalMapT::difference_type diff_T;
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    U u1 = make<U>(1);
    typename IntervalMapT::domain_mapping_type v1_u1(v1,u1);
    typename IntervalMapT::domain_mapping_type v3_u1(v3,u1);
    typename IntervalMapT::domain_mapping_type v5_u1(v5,u1);

    size_T s3 = make<size_T>(3);
    diff_T d0 = make<diff_T>(0);
    diff_T d2 = make<diff_T>(2);

    IntervalMapT is_1_3_5;
    is_1_3_5.add(v1_u1).add(v3_u1).add(v5_u1);

    BOOST_CHECK_EQUAL( cardinality(is_1_3_5),      s3 );
    BOOST_CHECK_EQUAL( is_1_3_5.size(),            s3 );
    icl::length(is_1_3_5);
    BOOST_CHECK_EQUAL( icl::length(is_1_3_5),      d0 );
    BOOST_CHECK_EQUAL( interval_count(is_1_3_5),   3 );
    BOOST_CHECK_EQUAL( is_1_3_5.iterative_size(),  3 );
    BOOST_CHECK_EQUAL( iterative_size(is_1_3_5),   3 );


    IntervalMapT is_123_5;
    is_123_5 = is_1_3_5;
    //OPROM: open problem: Ambiguity resolving value_type and mapping_type for overloaded o= operators.
    //is_123_5 += make_pair(IntervalT::open(v1,v3),u1);                 //error C2593: 'operator +=' is ambiguous
    //is_123_5 += make_pair<IntervalT, U>(IntervalT::open(v1,v3),u1); //error C2593: 'operator +=' is ambiguous
    //USASO: unsatisfctory solution 1: explicit IntervalMapT::value_type instead of make_pair
    is_123_5 += typename IntervalMapT::value_type(icl::interval<T>::open(v1,v3),u1);
    //USASO: unsatisfctory solution 2: not implementing mapping_type version of o=

    BOOST_CHECK_EQUAL( cardinality(is_123_5),      icl::infinity<size_T>::value() );
    BOOST_CHECK_EQUAL( is_123_5.size(),            icl::infinity<size_T>::value() );
    BOOST_CHECK_EQUAL( icl::length(is_123_5),      d2 );

#endif //BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_isolate_4_bicremental_continuous_types()
{
#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS

    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::size_type       size_T;
    typedef typename IntervalMapT::difference_type diff_T;

    T v0 = make<T>(0);
    T v2 = make<T>(2);
    T v4 = make<T>(4);
    U u1 = make<U>(1);
    IntervalT I0_4I = icl::interval<T>::closed(v0,v4);
    IntervalT C0_2D = icl::interval<T>::open(v0,v2);
    IntervalT C2_4D = icl::interval<T>::open(v2,v4);
    typename IntervalMapT::value_type I0_4I_u1(I0_4I,u1);
    typename IntervalMapT::value_type C0_2D_u1(C0_2D,u1);
    typename IntervalMapT::value_type C2_4D_u1(C2_4D,u1);
    //   {[0               4]}
    // - {   (0,2)   (2,4)   }
    // = {[0]     [2]     [4]}
    IntervalMapT iso_map = IntervalMapT(I0_4I_u1);
    IntervalMapT gap_set;
    gap_set.add(C0_2D_u1).add(C2_4D_u1);
    iso_map -= gap_set;
    
    BOOST_CHECK_EQUAL( cardinality(iso_map), static_cast<size_T>(3) );
    BOOST_CHECK_EQUAL( iso_map.iterative_size(), static_cast<std::size_t>(3) );
    BOOST_CHECK_EQUAL( iterative_size(iso_map), static_cast<std::size_t>(3) );
    BOOST_CHECK_EQUAL( iterative_size(iso_map), static_cast<std::size_t>(3) );

    IntervalMapT iso_map2;
    iso_map2.add(I0_4I_u1);
    iso_map2.subtract(C0_2D_u1).subtract(C2_4D_u1);
    
    IntervalMapT iso_map3(I0_4I_u1);
    (iso_map3 -= C0_2D_u1) -= C2_4D_u1;

    IntervalMapT iso_map4;
    iso_map4.insert(I0_4I_u1);
    iso_map4.erase(C0_2D_u1).erase(C2_4D_u1);
    
    BOOST_CHECK_EQUAL( iso_map, iso_map2 );
    BOOST_CHECK_EQUAL( iso_map, iso_map3 );
    BOOST_CHECK_EQUAL( iso_map, iso_map4 );
#endif //BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_contains_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::set_type IntervalSetT;

    IntervalMapT itv_map; 
    itv_map.add(K_v(3,1));    

    BOOST_CHECK_EQUAL( icl::contains(itv_map, MK_v(3)), true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, K_v(3,1)), true );

    BOOST_CHECK_EQUAL( icl::contains(IntervalMapT().add(K_v(3,1)), K_v(3,1)), true );
    BOOST_CHECK_EQUAL( icl::contains(IntervalMapT().add(K_v(3,1)), MK_v(3)), true );
    BOOST_CHECK_EQUAL( icl::contains(IntervalMapT().insert(K_v(3,1)), K_v(3,1)), true );
    itv_map.clear();
    BOOST_CHECK_EQUAL( icl::contains((itv_map += IIv(3,7,1)), IIv(3,7,1)), true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, IIv(3,7,2)), false );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(3,7)),   true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(4,6)),   true );
    BOOST_CHECK_EQUAL( icl::contains((itv_map += CIv(7,9,1)),IIv(3,9,1)), true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(4,8)),   true );
    BOOST_CHECK_EQUAL( icl::contains((itv_map += IIv(11,12,1)), IIv(3,12,1)), false );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(4,11)),  false );

    IntervalMapT itv_map0 = itv_map;    

    itv_map.clear();
    IntervalMapT itv_map2(IIv(5,8,1));
    itv_map2.add(K_v(9,1)).add(K_v(11,1));
    itv_map += itv_map2;
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_map2), true );    
    IntervalSetT itv_set2;
    icl::domain(itv_set2, itv_map2);
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_set2), true );    
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_contains_key_objects_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::set_type IntervalSetT;
    IntervalMapT itv_map; 

    itv_map.add(IDv(1,3,1));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, MK_v(0)), false );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, MK_v(2)), true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, MK_v(3)), false );

    itv_map.add(IDv(3,6,2));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(0,0)), false );
    contains(itv_map, I_I(2,4));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(2,4)), true );
    BOOST_CHECK_EQUAL( icl::contains(itv_map, I_I(6,6)), false );

    itv_map.add(IDv(8,9,2));

    IntervalSetT itv_set;
    itv_set.add(C_I(1,2)).add(C_D(2,6)).add(I_I(8,8));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_set), true );
    itv_set.add(I_I(1,4));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_set), true );
    itv_set.add(I_I(1,4));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_set), true );
    itv_set.add(I_I(7,7));
    BOOST_CHECK_EQUAL( icl::contains(itv_map, itv_set), false );

}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_operators_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    U u1 = make<U>(1);
    typename IntervalMapT::interval_type I3_5I(icl::interval<T>::closed(v3,v5));
    typename IntervalMapT::value_type I0_1I_u1(icl::interval<T>::closed(v0,v1),u1);
    typename IntervalMapT::value_type I3_5I_u1(icl::interval<T>::closed(v3,v5),u1);
    typename IntervalMapT::value_type I7_8I_u1(icl::interval<T>::closed(v7,v8),u1);
    
    IntervalMapT left, left2, right, all, section, complement;
    left.add(I0_1I_u1).add(I3_5I_u1);
    (right += I3_5I_u1) += I7_8I_u1;
    BOOST_CHECK_EQUAL( disjoint(left, right), false );
    BOOST_CHECK_EQUAL( intersects(left, right), true );

    (all += left) += right;
    (section += left) &= right;
    all -= section;
    complement += all;
    //complement.erase(I3_5I);
    icl::erase(complement, section);
    BOOST_CHECK_EQUAL( disjoint(section, complement), true );
    BOOST_CHECK_EQUAL( intersects(section, complement), false );
}


// Test for nontrivial intersection of interval maps with intervals and values
template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_base_intersect_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v2 = make<T>(2);
    T v3 = make<T>(3);
    T v4 = make<T>(4);
    T v5 = make<T>(5);
    T v6 = make<T>(6);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    T v9 = make<T>(9);

    U u1 = make<U>(1);

    IntervalT I0_3D = icl::interval<T>::right_open(v0,v3);
    IntervalT I1_3D = icl::interval<T>::right_open(v1,v3);
    IntervalT I1_4D = icl::interval<T>::right_open(v1,v4);
    IntervalT I1_8D = icl::interval<T>::right_open(v1,v8);
    IntervalT I2_7D = icl::interval<T>::right_open(v2,v7);
    IntervalT I2_3D = icl::interval<T>::right_open(v2,v3);
    IntervalT I5_8D = icl::interval<T>::right_open(v5,v8);
    IntervalT I6_7D = icl::interval<T>::right_open(v6,v7);
    IntervalT I6_8D = icl::interval<T>::right_open(v6,v8);
    IntervalT I6_9D = icl::interval<T>::right_open(v6,v9);

    typename IntervalMapT::value_type I0_3D_1(I0_3D, u1);
    typename IntervalMapT::value_type I6_9D_1(I6_9D, u1);
    typename IntervalMapT::value_type I1_3D_1(I1_3D, u1);
    typename IntervalMapT::value_type I6_8D_1(I6_8D, u1);
    typename IntervalMapT::value_type I2_3D_1(I2_3D, u1);
    typename IntervalMapT::value_type I6_7D_1(I6_7D, u1);

    //--------------------------------------------------------------------------
    //map_A        [0       3)       [6    9)
    //                     1           1
    //         &=      [1                8)
    //map_AB   ->      [1   3)       [6  8)
    //                     1           1
    //         &=        [2             7)     
    //         ->        [2 3)       [6 7)
    //                     1           1
    IntervalMap<T,U> map_A, map_AB, map_ab, map_ab2;
    interval_set<T>  set_B;
    map_A.add(I0_3D_1).add(I6_9D_1);
    map_AB = map_A;
    map_AB &= I1_8D;
    map_ab.add(I1_3D_1).add(I6_8D_1);

    BOOST_CHECK_EQUAL( map_AB, map_ab );

    map_AB = map_A;
    (map_AB &= I1_8D) &= I2_7D;
    map_ab2.add(I2_3D_1).add(I6_7D_1);

    BOOST_CHECK_EQUAL( map_AB, map_ab2 );

    //--------------------------------------------------------------------------
    //map_A        [0       3)       [6    9)
    //                     1           1
    //         &=      [1     4)  [5     8)
    //map_AB   ->      [1   3)       [6  8)
    //                     1           1
    //         &=        [2   4)  [5    7)     
    //         ->        [2 3)       [6 7)
    //                     1           1
    map_A.clear(); 
    map_A.add(I0_3D_1).add(I6_9D_1);
    set_B.add(I1_4D).add(I5_8D);
    map_AB = map_A;

    map_AB &= set_B;
    map_ab.clear();
    map_ab.add(I1_3D_1).add(I6_8D_1);
    BOOST_CHECK_EQUAL( map_AB, map_ab );

    //--------------------------------------------------------------------------
    //map_A      [0       3)       [6       9)
    //                1                1
    //         &=     1
    //map_AB ->      [1]
    //                1

    map_A.clear();
    map_A.add(I0_3D_1).add(I6_9D_1);
    map_AB = map_A;
    map_AB &= v1;
    map_ab.clear();
    map_ab.add(mapping_pair<T,U>(v1,u1));

    BOOST_CHECK_EQUAL( map_AB, map_ab );
}


// Test for nontrivial erasure of interval maps with intervals and interval sets
template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_base_erase_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v2 = make<T>(2);
    T v3 = make<T>(3);
    T v4 = make<T>(4);
    T v5 = make<T>(5);
    T v6 = make<T>(6);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    T v9 = make<T>(9);

    U u1 = make<U>(1);

    IntervalT I0_1D = icl::interval<T>::right_open(v0,v1);
    IntervalT I0_2D = icl::interval<T>::right_open(v0,v2);
    IntervalT I0_3D = icl::interval<T>::right_open(v0,v3);
    IntervalT I1_3D = icl::interval<T>::right_open(v1,v3);
    IntervalT I1_4D = icl::interval<T>::right_open(v1,v4);
    IntervalT I1_8D = icl::interval<T>::right_open(v1,v8);
    IntervalT I2_4D = icl::interval<T>::right_open(v2,v4);
    IntervalT I2_7D = icl::interval<T>::right_open(v2,v7);
    IntervalT I2_3D = icl::interval<T>::right_open(v2,v3);
    IntervalT I5_7D = icl::interval<T>::right_open(v5,v7);
    IntervalT I5_8D = icl::interval<T>::right_open(v5,v8);
    IntervalT I6_7D = icl::interval<T>::right_open(v6,v7);
    IntervalT I6_8D = icl::interval<T>::right_open(v6,v8);
    IntervalT I6_9D = icl::interval<T>::right_open(v6,v9);
    IntervalT I7_9D = icl::interval<T>::right_open(v7,v9);
    IntervalT I8_9D = icl::interval<T>::right_open(v8,v9);

    typename IntervalMapT::value_type I0_1D_1(I0_1D, u1);
    typename IntervalMapT::value_type I0_3D_1(I0_3D, u1);
    typename IntervalMapT::value_type I0_2D_1(I0_2D, u1);
    typename IntervalMapT::value_type I6_9D_1(I6_9D, u1);
    typename IntervalMapT::value_type I1_3D_1(I1_3D, u1);
    typename IntervalMapT::value_type I6_8D_1(I6_8D, u1);
    typename IntervalMapT::value_type I2_3D_1(I2_3D, u1);
    typename IntervalMapT::value_type I6_7D_1(I6_7D, u1);
    typename IntervalMapT::value_type I7_9D_1(I7_9D, u1);
    typename IntervalMapT::value_type I8_9D_1(I8_9D, u1);

    //--------------------------------------------------------------------------
    //map_A        [0        3)       [6       9)
    //                  1                  1
    //      erase        [2              7)
    //map_A2   ->  [0   2)                [7   9)
    //                 1                    1
    //      erase      [1                   8)     
    //         ->  [0 1)                    [8 9)
    //               1                       1
    IntervalMap<T,U> map_A, map_A2, map_A3, map_check2, map_check3;
    interval_set<T>  set_B;
    map_A.add(I0_3D_1).add(I6_9D_1);
    map_A2 = map_A;
    map_A2.erase(I2_7D);
    map_check2.add(I0_2D_1).add(I7_9D_1);
    BOOST_CHECK_EQUAL( map_A2, map_check2 );

    map_A3 = map_A2;
    map_A3.erase(I1_8D);
    map_check3.add(I0_1D_1).add(I8_9D_1);
    BOOST_CHECK_EQUAL( map_A3, map_check3 );

          
    //--------------------------------------------------------------------------
    //map_A        [0        3)       [6       9)
    //                  1                  1
    //      erase        [2              7)
    //         ->  [0   2)                [7   9)
    //                 1                    1
    //      erase      [1                   8)     
    //         ->  [0 1)                    [8 9)
    //               1                       1
    map_A3 = map_A;
    map_A3.erase(I2_7D).erase(I1_8D);
    BOOST_CHECK_EQUAL( map_A3, map_check3 );

    //--------------------------------------------------------------------------
    //map_A        [0        3)       [6       9)
    //                  1                  1
    //         -=        [2              7)
    //         ->  [0   2)                [7   9)
    //                 1                    1
    //         -=      [1                   8)     
    //         ->  [0 1)                    [8 9)
    //               1                       1
    map_A3 = map_A;
    (map_A3 -= I2_7D) -= I1_8D;
    BOOST_CHECK_EQUAL( map_A3, map_check3 );

    //--------------------------------------------------------------------------
    //map_A        [0        3)       [6       9)
    //                  1                  1
    //      erase        [2    4)   [5   7)
    //         ->  [0   2)                [7   9)
    //                 1                    1
    map_A3 = map_A;
    set_B.add(I2_4D).add(I5_7D);
    map_A3 -= set_B;
    BOOST_CHECK_EQUAL( map_A3, map_check2 );
}


// Test first_collision
template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_base_is_disjoint_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMap<T,U>::interval_set_type IntervalSetT;

    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    T v6 = make<T>(6);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    T v9 = make<T>(9);

    U u1 = make<U>(1);

    IntervalT I0_1D = icl::interval<T>::right_open(v0,v1);
    IntervalT I1_3D = icl::interval<T>::right_open(v1,v3);
    IntervalT I3_6D = icl::interval<T>::right_open(v3,v6);
    IntervalT I5_7D = icl::interval<T>::right_open(v5,v7);
    IntervalT I6_8D = icl::interval<T>::right_open(v6,v8);
    IntervalT I8_9D = icl::interval<T>::right_open(v8,v9);

    typename IntervalMapT::value_type I0_1D_1(I0_1D, u1);
    typename IntervalMapT::value_type I1_3D_1(I1_3D, u1);
    typename IntervalMapT::value_type I3_6D_1(I3_6D, u1);
    typename IntervalMapT::value_type I5_7D_1(I5_7D, u1);
    typename IntervalMapT::value_type I6_8D_1(I6_8D, u1);
    typename IntervalMapT::value_type I8_9D_1(I8_9D, u1);

    //--------------------------------------------------------------------------
    //map_A          [1      3)       [6       8)
    //                  1                  1
    //map_B      [0 1)        [3     6)         [8 9)
    //             1              1               1
    IntervalMapT map_A, map_B;
    IntervalSetT set_A, set_B;

    map_A.add(I1_3D_1).add(I6_8D_1);
    map_B.add(I0_1D_1).add(I3_6D_1).add(I8_9D_1);
    BOOST_CHECK_EQUAL( disjoint(map_A, map_B), true );
    BOOST_CHECK_EQUAL( disjoint(map_B, map_A), true );
    BOOST_CHECK_EQUAL( intersects(map_A, map_B), false );
    BOOST_CHECK_EQUAL( intersects(map_B, map_A), false );

    icl::domain(set_A, map_A);
    icl::domain(set_B, map_B);
    BOOST_CHECK_EQUAL( disjoint(map_A, set_B), true );
    BOOST_CHECK_EQUAL( disjoint(set_B, map_A), true );
    BOOST_CHECK_EQUAL( disjoint(set_A, map_B), true );
    BOOST_CHECK_EQUAL( disjoint(map_B, set_A), true );
    BOOST_CHECK_EQUAL( intersects(map_A, set_B), false );
    BOOST_CHECK_EQUAL( intersects(set_B, map_A), false );
    BOOST_CHECK_EQUAL( intersects(set_A, map_B), false );
    BOOST_CHECK_EQUAL( intersects(map_B, set_A), false );

    map_A += I5_7D_1;

    BOOST_CHECK_EQUAL( disjoint(map_A, map_B), false );
    BOOST_CHECK_EQUAL( disjoint(map_B, map_A), false );
    BOOST_CHECK_EQUAL( intersects(map_A, map_B),  true );
    BOOST_CHECK_EQUAL( intersects(map_B, map_A),  true );

    icl::domain(set_A, map_A);
    icl::domain(set_B, map_B);
    BOOST_CHECK_EQUAL( disjoint(map_A, set_B), false );
    BOOST_CHECK_EQUAL( disjoint(set_B, map_A), false );
    BOOST_CHECK_EQUAL( disjoint(set_A, map_B), false );
    BOOST_CHECK_EQUAL( disjoint(map_B, set_A), false );
    BOOST_CHECK_EQUAL( intersects(map_A, set_B), true );
    BOOST_CHECK_EQUAL( intersects(set_B, map_A), true );
    BOOST_CHECK_EQUAL( intersects(set_A, map_B), true );
    BOOST_CHECK_EQUAL( intersects(map_B, set_A), true );
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_flip_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef IntervalMapT IMap;

    IntervalMapT set_a;
    //[0     2)
    //    1
    //    [1     3)
    //        1
    //[0 1)   [2 3) : {[0 2)->1} ^= ([2 3)->1)
    //  1       1
    //BOOST_CHECK_EQUAL(IMap(IDv(0,2,1)) ^= (IDv(1,3,1)), IMap(IDv(0,1,1)) + IDv(2,3,1));
    set_a = IMap(IDv(0,2,1));
    IntervalMapT set_b = set_a;
    BOOST_CHECK_EQUAL(set_a ^= IDv(1,3,1), IMap(IDv(0,1,1)) + IDv(2,3,1));
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_infix_plus_overload_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a, map_b;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    map_b.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    BOOST_CHECK_EQUAL(map_a + map_b, map_b + map_a);
    //This checks all cases of is_interval_map_derivative<T>
    BOOST_CHECK_EQUAL(map_a + val_pair1, val_pair1 + map_a);
    BOOST_CHECK_EQUAL(map_b + val_pair2, val_pair2 + map_b);
    BOOST_CHECK_EQUAL(map_b + map_pair, map_pair + map_b);
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_infix_pipe_overload_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a, map_b;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    map_b.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    BOOST_CHECK_EQUAL(map_a | map_b, map_b | map_a);
    //This checks all cases of is_interval_map_derivative<T>
    BOOST_CHECK_EQUAL(map_a | val_pair1, val_pair1 | map_a);
    BOOST_CHECK_EQUAL(map_b | val_pair2, val_pair2 | map_b);
    BOOST_CHECK_EQUAL(map_b | map_pair, map_pair | map_b);
}



template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_infix_minus_overload_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalT itv = C_D(4,11);
    typename IntervalMapT::interval_mapping_type itv_v = CDv(4,11,3);

    IntervalMapT map_a, map_b, map_c;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    map_b.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));
    map_c = map_a;

    interval_set<T>          join_set_a;
    separate_interval_set<T> sep_set_a;
    split_interval_set<T>    split_set_a;
    join_set_a .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,9));
    sep_set_a  .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,11));
    split_set_a.add(I_I(0,0)).add(I_D(8,7)).add(I_I(6,11));
    
    //Happy day overloading
    BOOST_CHECK_EQUAL(map_a - map_b, (map_c = map_a) -= map_b);
    BOOST_CHECK_EQUAL(map_a - map_b, map_c);

    //This checks all cases of is_interval_map_derivative<T>
    BOOST_CHECK_EQUAL((map_a - val_pair1) + val_pair1, (map_a + val_pair1) - val_pair1);
    BOOST_CHECK_EQUAL((map_b - val_pair2) + val_pair2, (map_b + val_pair2) - val_pair2);
    BOOST_CHECK_EQUAL((map_b - map_pair)  + map_pair,  (map_b + map_pair)  - map_pair);

    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(map_a - itv,     (map_a + itv_v) - itv);
    BOOST_CHECK_EQUAL(map_b - MK_v(8), (IIv(8,8,3) + map_b) - MK_v(8));

    //This checks all cases of is_interval_set_companion<T>
    BOOST_CHECK_EQUAL(map_a - split_set_a, ((split_set_a & map_a) + map_a) - split_set_a);
    BOOST_CHECK_EQUAL(map_a - sep_set_a,   ((sep_set_a   & map_a) + map_a) - sep_set_a);
    BOOST_CHECK_EQUAL(map_a - join_set_a,  ((join_set_a  & map_a) + map_a) - join_set_a);
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_infix_et_overload_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalT itv = C_D(4,11);

    IntervalMapT map_a, map_b;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    map_b.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    interval_set<T>          join_set_a;
    separate_interval_set<T> sep_set_a;
    split_interval_set<T>    split_set_a;
    join_set_a .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,9));
    sep_set_a  .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,11));
    split_set_a.add(I_I(0,0)).add(I_D(8,7)).add(I_I(6,11));
    
    //Happy day overloading
    BOOST_CHECK_EQUAL(map_a & map_b, map_b & map_a);

    //This checks all cases of is_interval_map_derivative<T>
    BOOST_CHECK_EQUAL(map_a & val_pair1, val_pair1 & map_a);
    BOOST_CHECK_EQUAL(map_b & val_pair2, val_pair2 & map_b);
    BOOST_CHECK_EQUAL(map_b & map_pair, map_pair & map_b);

    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(map_a & itv, itv & map_a);
    BOOST_CHECK_EQUAL(map_b & MK_v(8), MK_v(8) & map_b);

    //This checks all cases of is_interval_set_companion<T>
    BOOST_CHECK_EQUAL(map_a & split_set_a, split_set_a & map_a);
    BOOST_CHECK_EQUAL(map_a & sep_set_a,   sep_set_a   & map_a);
    BOOST_CHECK_EQUAL(map_a & join_set_a,  join_set_a  & map_a);
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_infix_caret_overload_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalT itv = C_D(4,11);

    IntervalMapT map_a, map_b;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    map_b.add(IDv(0,9,2)).add(IIv(3,6,1)).add(IDv(5,7,1));

    interval_set<T>          join_set_a;
    separate_interval_set<T> sep_set_a;
    split_interval_set<T>    split_set_a;
    join_set_a .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,9));
    sep_set_a  .add(I_D(0,4)).add(I_I(4,6)).add(I_D(5,11));
    split_set_a.add(I_I(0,0)).add(I_D(8,7)).add(I_I(6,11));
    
    //Happy day overloading
    BOOST_CHECK_EQUAL(map_a ^ map_b, map_b ^ map_a);

    //This checks all cases of is_interval_map_derivative<T>
    BOOST_CHECK_EQUAL(map_a ^ val_pair1, val_pair1 ^ map_a);
    BOOST_CHECK_EQUAL(map_b ^ val_pair2, val_pair2 ^ map_b);
    BOOST_CHECK_EQUAL(map_b ^ map_pair,  map_pair ^ map_b);
}

template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_find_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type  IntervalT;
    typedef typename IntervalMapT::const_iterator c_iterator;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    // {(1  3)    [6  8)[8 9)[9  11) 
    //     1         3    4     3
    //          5? 6?
    c_iterator found1 = map_a.find(MK_v(6));
    c_iterator found2 = icl::find(map_a, MK_v(6));

    BOOST_CHECK      ( found1 == found2 );
    BOOST_CHECK_EQUAL( found1->second, found2->second );
    BOOST_CHECK_EQUAL( found1->second, MK_u(3) );
    BOOST_CHECK_EQUAL( map_a(MK_v(6)), MK_u(3) );

    found1 = map_a.find(MK_v(5));

    BOOST_CHECK_EQUAL( found1 == map_a.end(), true );
    BOOST_CHECK_EQUAL( map_a(MK_v(5)), MK_u(0) );
    BOOST_CHECK_EQUAL( map_a(MK_v(8)), MK_u(4) );

    //LAW map c; key k: k in dom(c) => contains(c, (k, find(c, k)->second))
    BOOST_CHECK( icl::contains(map_a, K_v(2, icl::find(map_a, MK_v(2))->second)) );
    BOOST_CHECK( icl::contains(map_a, K_v(11, map_a.find(MK_v(11))->second)) );

    BOOST_CHECK(  icl::contains(map_a, MK_v(2)) );
    BOOST_CHECK(  icl::contains(map_a, MK_v(10)) );
    BOOST_CHECK( !icl::contains(map_a, MK_v(1)) );
    BOOST_CHECK( !icl::contains(map_a, MK_v(3)) );
    BOOST_CHECK( !icl::contains(map_a, MK_v(12)) );

    BOOST_CHECK(  icl::intersects(map_a, MK_v(2)) );
    BOOST_CHECK(  icl::intersects(map_a, MK_v(10)) );
    BOOST_CHECK( !icl::intersects(map_a, MK_v(1)) );
    BOOST_CHECK( !icl::intersects(map_a, MK_v(3)) );
    BOOST_CHECK( !icl::intersects(map_a, MK_v(12)) );
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_find_4_numeric_continuous_types()
{
#ifndef BOOST_ICL_TEST_CHRONO
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type  IntervalT;
    typedef typename IntervalMapT::const_iterator c_iterator;

    T q_1_2 = MK_v(1) / MK_v(2);//JODO Doesn't work with chrono
    T q_3_2 = MK_v(3) / MK_v(2);
    T q_1_3 = MK_v(1) / MK_v(3);
    T q_2_3 = MK_v(2) / MK_v(3);
    T q_4_3 = MK_v(4) / MK_v(3);
    T q_5_3 = MK_v(5) / MK_v(3);

    IntervalMapT map_a;
    map_a.add(MK_seg(IntervalT(q_1_3, q_2_3), 1)).add(MK_seg(IntervalT(q_4_3, q_5_3), 2));
    // {[1/3   2/3)    [4/3   5/3)} 
    //       1              2
     
    c_iterator found1 = map_a.find(q_1_2);
    c_iterator found2 = icl::find(map_a, q_1_2);
    BOOST_CHECK      ( found1 == found2 );
    BOOST_CHECK_EQUAL( found1->second, found2->second );
    BOOST_CHECK_EQUAL( found1->second, MK_u(1) );

    found1 = map_a.find(q_3_2);
    found2 = icl::find(map_a, q_3_2);
    BOOST_CHECK      ( found1 == found2 );
    BOOST_CHECK_EQUAL( found1->second, found2->second );
    BOOST_CHECK_EQUAL( found1->second, MK_u(2) );

    if( mpl::or_<mpl::not_<is_static_left_open<IntervalT> >, boost::is_signed<T> >::value )
    {
        found1 = map_a.find(MK_v(0));
        found2 = icl::find(map_a, MK_v(0));
        BOOST_CHECK      ( found1 == found2 );
        BOOST_CHECK      ( found1 == map_a.end() );
    }

    found1 = map_a.find(MK_v(1));
    found2 = icl::find(map_a, MK_v(1));
    BOOST_CHECK      ( found1 == found2 );
    BOOST_CHECK      ( found1 == map_a.end() );

    if( mpl::or_<mpl::not_<is_static_left_open<IntervalT> >, boost::is_signed<T> >::value )
    {
        BOOST_CHECK( !icl::contains(map_a, MK_v(0)) );
    }
    BOOST_CHECK(  icl::contains(map_a, q_1_2) );
    BOOST_CHECK( !icl::contains(map_a, MK_v(1)) );
    BOOST_CHECK(  icl::contains(map_a, q_3_2) );
    BOOST_CHECK( !icl::contains(map_a, MK_v(2)) );
#endif
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_range_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type  IntervalT;
    typedef typename IntervalMapT::const_iterator c_iterator;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));
    // {(1  3)    [6  8)[8 9)[9  11) 
    //     1         3    4     3
    //    [2        7) := itv

    IntervalT itv = I_D(2, 7);
    c_iterator lwb1 = icl::find(map_a, itv);
    c_iterator lwb2 = map_a.lower_bound(itv);

    BOOST_CHECK      ( lwb1 == lwb2 );
    BOOST_CHECK_EQUAL( lwb1->second, lwb2->second );
    BOOST_CHECK_EQUAL( lwb1->second, MK_u(1) );

    c_iterator upb1 = map_a.upper_bound(itv);
    BOOST_CHECK_EQUAL( upb1->second, MK_u(4) );

    std::pair<c_iterator,c_iterator> exterior =  map_a.equal_range(itv);
    BOOST_CHECK      ( lwb1 == exterior.first );
    BOOST_CHECK      ( upb1 == exterior.second );
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_set_4_bicremental_types()
{
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));

    BOOST_CHECK_EQUAL( icl::contains(map_a.set(CDv(2,10,4)), CDv(2,10,4)), true );
    BOOST_CHECK_EQUAL( icl::contains(map_a.set(K_v(4,5)), K_v(4,5)), true );
    BOOST_CHECK_EQUAL( icl::contains(map_a.set(K_v(4,5)).set(CDv(3,5,6)), CDv(3,5,6)), true );
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
void interval_map_inclusion_compare_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMap<T,U,Trt>::set_type IntervalSetT;
    typedef icl::map<T,U,Trt> MapT;
    typedef std::set<T> SetT;

    IntervalMapT itv_map_sub_a, itv_map_a, itv_map_a2, itv_map_super_a, 
                 itv_map_b, itv_map_c;
    itv_map_sub_a.add(IDv(2,4,1)).add(IIv(6,7,3));
    itv_map_a = itv_map_sub_a; 
    itv_map_a.add(IIv(9,9,1));
    itv_map_a2 = itv_map_a;
    itv_map_c = itv_map_sub_a;
    itv_map_c.erase(MK_v(7)).add(IIv(11,11,2));
    itv_map_b = itv_map_a;
    itv_map_b.set(IIv(6,7,2));


    BOOST_CHECK_EQUAL( inclusion_compare(IntervalMapT(), IntervalMapT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, itv_map_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, itv_map_a2), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, IntervalMapT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, itv_map_sub_a), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(IntervalMapT(), itv_map_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_sub_a, itv_map_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, itv_map_b), inclusion::unrelated );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, itv_map_c), inclusion::unrelated );

    IntervalSetT set_sub_a, set_a, set_a2, set_b, set_c;
    icl::domain(set_a, itv_map_a);
    icl::domain(set_a2, itv_map_a2);
    icl::domain(set_sub_a, itv_map_sub_a);

    BOOST_CHECK_EQUAL( inclusion_compare(IntervalMapT(), IntervalSetT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(IntervalSetT(), IntervalMapT()), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(IntervalSetT(), IntervalSetT()), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, set_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, itv_map_a), inclusion::equal );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, set_a2), inclusion::equal );

    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, IntervalSetT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_a, set_sub_a), inclusion::superset );

    BOOST_CHECK_EQUAL( inclusion_compare(IntervalSetT(), itv_map_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_sub_a, itv_map_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(set_a, IntervalSetT()), inclusion::superset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_a, set_sub_a), inclusion::superset );

    BOOST_CHECK_EQUAL( inclusion_compare(IntervalSetT(), set_a), inclusion::subset );
    BOOST_CHECK_EQUAL( inclusion_compare(set_sub_a, set_a), inclusion::subset );

    BOOST_CHECK_EQUAL( inclusion_compare(set_a, itv_map_c), inclusion::unrelated );
    BOOST_CHECK_EQUAL( inclusion_compare(itv_map_c, set_a), inclusion::unrelated );

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
void interval_map_std_copy_via_inserter_4_bicremental_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT; //Nedded for the test value generator
    typedef typename IntervalMapT::interval_type   IntervalT;

    // Check equality of copying using handcoded loop or std::copy via inserter.
    typedef std::pair<IntervalT, U> SegmentT;
    std::vector<SegmentT> seg_vec_a;
    IntervalMapT std_copied_map;

    // For an empty sequence
    test_interval_map_copy_via_inserter(seg_vec_a, std_copied_map);

    // For an singleton sequence
    seg_vec_a.push_back(IDv(0,1,1));
    test_interval_map_copy_via_inserter(seg_vec_a, std_copied_map);

    // Two separate segments
    seg_vec_a.push_back(IDv(3,5,1));
    test_interval_map_copy_via_inserter(seg_vec_a, std_copied_map);

    // Touching case
    seg_vec_a.push_back(IDv(5,7,1));
    test_interval_map_copy_via_inserter(seg_vec_a, std_copied_map);

    // Overlapping case
    seg_vec_a.push_back(IDv(6,9,1));
    test_interval_map_copy_via_inserter(seg_vec_a, std_copied_map);
    
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
void interval_map_element_iter_4_discrete_types()
{
    typedef IntervalMap<T,U,Trt> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;
    typedef typename IntervalMapT::element_iterator ReptatorT;
    typedef std::vector<std::pair<T,U> > VectorT;

    IntervalMapT map_a;
    map_a.insert(IIv(1,3,1)).insert(IIv(6,7,2));

    typename IntervalMapT::atomized_type ato_map_a;
    ReptatorT el_it = elements_begin(map_a);

    VectorT vec(5), cev(5);
    vec[0]=sK_v(1,1);vec[1]=sK_v(2,1);vec[2]=sK_v(3,1);vec[3]=sK_v(6,2);vec[4]=sK_v(7,2);
    cev[0]=sK_v(7,2);cev[1]=sK_v(6,2);cev[2]=sK_v(3,1);cev[3]=sK_v(2,1);cev[4]=sK_v(1,1);

    VectorT dest;
    std::copy(elements_begin(map_a), elements_end(map_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( vec == dest, true );

    dest.clear();
    std::copy(elements_rbegin(map_a), elements_rend(map_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( cev == dest, true );

    dest.clear();
    std::reverse_copy(elements_rbegin(map_a), elements_rend(map_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( vec == dest, true );

    dest.clear();
    std::reverse_copy(elements_begin(map_a), elements_end(map_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( cev == dest, true );

}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_intersects_4_bicremental_types()
{
    // Test of intersects and disjoint for domain_type and interval_type.
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    typename IntervalMapT::interval_mapping_type val_pair1 = IDv(6,9,1);
    std::pair<const IntervalT, U> val_pair2 = IDv(3,5,3);
    mapping_pair<T,U> map_pair = K_v(4,3);

    IntervalMapT map_a;
    map_a.add(CDv(1,3,1)).add(IDv(8,9,1)).add(IIv(6,11,3));

    BOOST_CHECK( icl::is_interval_container<IntervalMapT>::value );
    BOOST_CHECK( icl::has_domain_type<IntervalMapT>::value );
    BOOST_CHECK( (boost::is_same<T, typename domain_type_of<IntervalMapT>::type>::value) );

    BOOST_CHECK( icl::intersects(map_a,  MK_v(2) ) );
    BOOST_CHECK( icl::intersects(map_a,  MK_v(11)) );
    BOOST_CHECK( icl::disjoint(map_a, MK_v(1) ) );
    BOOST_CHECK( icl::disjoint(map_a, MK_v(12)) );

    BOOST_CHECK( icl::intersects(map_a, I_D(2,3)) );
    BOOST_CHECK( icl::intersects(map_a, I_D(6,8)) );
    BOOST_CHECK( icl::disjoint(map_a,   I_D(3,5)) );
    BOOST_CHECK( icl::disjoint(map_a,  I_D(12,14)) );

    //-------------------------------------+
    //   (1   3)      [6   8)[8 9)[9    11]
    //      1            3     4      3
    mapping_pair<T,U> map_pair_2_1  = K_v(2,1);
    BOOST_CHECK( icl::intersects(map_a,  map_pair_2_1 ) );
    BOOST_CHECK( icl::intersects(map_a,  K_v(6,3) ) );
    BOOST_CHECK( icl::intersects(map_a,  IDv(6,8,3) ) );
    BOOST_CHECK( icl::intersects(map_a,  CIv(8,11,3) ) );
    BOOST_CHECK( icl::intersects(map_a,  IIv(6,11,3) ) );
    BOOST_CHECK( icl::intersects(map_a,  IIv(6,11,5) ) );
    BOOST_CHECK(!icl::intersects(map_a,  IDv(4,6,5) ) );

    BOOST_CHECK( icl::disjoint(map_a,  IDv(4,6,5) ) );
    BOOST_CHECK(!icl::disjoint(map_a,  IDv(0,12,1) ) );
}


template 
<
#if (defined(__GNUC__) && (__GNUC__ < 4)) //MEMO Can be simplified, if gcc-3.4 is obsolete
    ICL_IntervalMap_TEMPLATE(T,U,Traits,partial_absorber) IntervalMap,
#else
    ICL_IntervalMap_TEMPLATE(_T,_U,Traits,partial_absorber) IntervalMap,
#endif
    class T, class U
>
void interval_map_move_4_discrete_types()
{
#   ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    typedef IntervalMap<T,U> IntervalMapT;
    typedef typename IntervalMapT::interval_type   IntervalT;

    IntervalMapT map_A(boost::move(IntervalMapT(IDv(0,4,2))));
    IntervalMapT map_B(boost::move(IntervalMapT(IDv(0,2,1)).add(IDv(2,4,1)).add(IDv(0,4,1))));

    BOOST_CHECK( icl::is_element_equal(map_A, map_B) );
    BOOST_CHECK_EQUAL( map_A, join(map_B) );

    map_A = boost::move(IntervalMapT(IIv(1,4,2)));
    map_B = boost::move(IntervalMapT(CIv(0,2,1)).insert(IDv(3,5,1)).add(CDv(0,5,1)));

    BOOST_CHECK( icl::is_element_equal(map_A, map_B) );
    BOOST_CHECK_EQUAL( map_A, join(map_B) );

#   endif // BOOST_NO_CXX11_RVALUE_REFERENCES
}


#endif // LIBS_ICL_TEST_TEST_INTERVAL_MAP_SHARED_HPP_JOFA_081005

