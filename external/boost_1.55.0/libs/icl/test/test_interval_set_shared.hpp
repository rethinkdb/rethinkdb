/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_INTERVAL_SET_SHARED_HPP_JOFA_080920
#define LIBS_ICL_TEST_TEST_INTERVAL_SET_SHARED_HPP_JOFA_080920

#include "portability.hpp"

template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_fundamentals_4_ordered_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSet<T>::size_type       size_T;
    typedef typename IntervalSet<T>::difference_type diff_T;

    // ordered types is the largest set of instance types.
    // Because we can not generate values via incrementation for e.g. string,
    // we are able to test operations only for the most basic values
    // identity_element (0, empty, T() ...) and unit_element.

    T v0 = boost::icl::identity_element<T>::value();
    T v1 = unit_element<T>::value();
    IntervalT I0_0I(v0);
    IntervalT I1_1I(v1);
    IntervalT I0_1I(v0, v1, interval_bounds::closed());

    //-------------------------------------------------------------------------
    //empty set
    //-------------------------------------------------------------------------
    BOOST_CHECK_EQUAL(IntervalSet<T>().empty(), true);
    BOOST_CHECK_EQUAL(icl::is_empty(IntervalSet<T>()), true);
    BOOST_CHECK_EQUAL(cardinality(IntervalSet<T>()), boost::icl::identity_element<size_T>::value());
    BOOST_CHECK_EQUAL(IntervalSet<T>().size(), boost::icl::identity_element<size_T>::value());
    BOOST_CHECK_EQUAL(interval_count(IntervalSet<T>()), 0);
    BOOST_CHECK_EQUAL(IntervalSet<T>().iterative_size(), 0);
    BOOST_CHECK_EQUAL(iterative_size(IntervalSet<T>()), 0);
    BOOST_CHECK_EQUAL(IntervalSet<T>(), IntervalSet<T>());

    IntervalT mt_interval = boost::icl::identity_element<IntervalT>::value();
    BOOST_CHECK_EQUAL(mt_interval, IntervalT());
    IntervalSet<T> mt_set = boost::icl::identity_element<IntervalSet<T> >::value();
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());

    //adding emptieness to emptieness yields emptieness ;)
    mt_set.add(mt_interval).add(mt_interval);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    mt_set.insert(mt_interval).insert(mt_interval);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    (mt_set += mt_interval) += mt_interval;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    BOOST_CHECK_EQUAL(hull(mt_set), boost::icl::identity_element<IntervalT >::value());

    //subtracting emptieness
    mt_set.subtract(mt_interval).subtract(mt_interval);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    mt_set.erase(mt_interval).erase(mt_interval);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    (mt_set -= mt_interval) -= mt_interval;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());

    //subtracting elements form emptieness
    mt_set.subtract(v0).subtract(v1);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    mt_set.erase(v0).erase(v1);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    (mt_set -= v1) -= v0;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());

    //subtracting intervals form emptieness
    mt_set.subtract(I0_1I).subtract(I1_1I);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    mt_set.erase(I0_1I).erase(I1_1I);
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    (mt_set -= I1_1I) -= I0_1I;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());

    //insecting emptieness
    //mt_set.insect(mt_interval).insect(mt_interval);
    //BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    (mt_set &= mt_interval) &= mt_interval;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    //insecting emptieness with elements
    (mt_set &= v1) &= v0;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());
    //insecting emptieness with intervals
    (mt_set &= I1_1I) &= I0_1I;
    BOOST_CHECK_EQUAL(mt_set, IntervalSet<T>());

    //-------------------------------------------------------------------------
    //unary set
    //-------------------------------------------------------------------------
    IntervalSet<T> single_I0_0I_from_element(v0);
    IntervalSet<T> single_I0_0I_from_interval(I0_0I);
    IntervalSet<T> single_I0_0I(single_I0_0I_from_interval);

    BOOST_CHECK_EQUAL(single_I0_0I_from_element, single_I0_0I_from_interval);
    BOOST_CHECK_EQUAL(single_I0_0I_from_element, single_I0_0I);
    BOOST_CHECK_EQUAL(icl::hull(single_I0_0I).lower(), I0_0I.lower());
    BOOST_CHECK_EQUAL(icl::hull(single_I0_0I).upper(), I0_0I.upper());

    IntervalSet<T> single_I1_1I_from_element(v1);
    IntervalSet<T> single_I1_1I_from_interval(I1_1I);
    IntervalSet<T> single_I1_1I(single_I1_1I_from_interval);

    BOOST_CHECK_EQUAL(single_I1_1I_from_element, single_I1_1I_from_interval);
    BOOST_CHECK_EQUAL(single_I1_1I_from_element, single_I1_1I);

    IntervalSet<T> single_I0_1I_from_interval(I0_1I);
    IntervalSet<T> single_I0_1I(single_I0_1I_from_interval);

    BOOST_CHECK_EQUAL(single_I0_1I_from_interval, single_I0_1I);
    BOOST_CHECK_EQUAL(hull(single_I0_1I), I0_1I);
    BOOST_CHECK_EQUAL(hull(single_I0_1I).lower(), I0_1I.lower());
    BOOST_CHECK_EQUAL(hull(single_I0_1I).upper(), I0_1I.upper());

    //contains predicate
    BOOST_CHECK_EQUAL(icl::contains(single_I0_0I, v0), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_0I, I0_0I), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I1_1I, v1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I1_1I, I1_1I), true);

    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, v0), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, I0_1I), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, v1), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, I1_1I), true);

    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, single_I0_0I), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, single_I1_1I), true);
    BOOST_CHECK_EQUAL(icl::contains(single_I0_1I, single_I0_1I), true);

    BOOST_CHECK_EQUAL(cardinality(single_I0_0I), unit_element<size_T>::value());
    BOOST_CHECK_EQUAL(single_I0_0I.size(), unit_element<size_T>::value());
    BOOST_CHECK_EQUAL(interval_count(single_I0_0I), 1);
    BOOST_CHECK_EQUAL(single_I0_0I.iterative_size(), 1);
    BOOST_CHECK_EQUAL(iterative_size(single_I0_0I), 1);
}



template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_ctor_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    T v4 = make<T>(4);
    IntervalT I4_4I(v4);

    IntervalSet<T> _I4_4I;
    BOOST_CHECK_EQUAL( _I4_4I.empty(), true );
    IntervalSet<T> _I4_4I_1;
    IntervalSet<T> _I4_4I_2;
    IntervalSet<T> _I4_4I_3;
    _I4_4I   += v4;
    _I4_4I_1 += I4_4I;
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_1 );
    _I4_4I_2.add(v4);
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_2 );
    _I4_4I_3.add(I4_4I);
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_3 );
    _I4_4I_1.add(v4).add(I4_4I);
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_1 );
    _I4_4I_1.insert(v4).insert(I4_4I);
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_1 );
    (_I4_4I_1 += v4) += I4_4I;
    BOOST_CHECK_EQUAL( _I4_4I,                    _I4_4I_1 );
    
    BOOST_CHECK_EQUAL( cardinality(_I4_4I),      unit_element<typename IntervalSet<T>::size_type>::value()  );
    BOOST_CHECK_EQUAL( _I4_4I.size(),             unit_element<typename IntervalSet<T>::size_type>::value()  );
    BOOST_CHECK_EQUAL( interval_count(_I4_4I),   1  );
    BOOST_CHECK_EQUAL( _I4_4I.iterative_size(),   1  );
    BOOST_CHECK_EQUAL( iterative_size(_I4_4I),   1  );
    BOOST_CHECK_EQUAL( hull(_I4_4I).lower(),      v4 );
    BOOST_CHECK_EQUAL( hull(_I4_4I).upper(),      v4 );

    IntervalSet<T> _I4_4I_copy(_I4_4I);
    IntervalSet<T> _I4_4I_assigned;
    _I4_4I_assigned = _I4_4I;
    BOOST_CHECK_EQUAL( _I4_4I, _I4_4I_copy );
    BOOST_CHECK_EQUAL( _I4_4I, _I4_4I_assigned );
    _I4_4I_assigned.clear();
    BOOST_CHECK_EQUAL( true,   _I4_4I_assigned.empty() );

    _I4_4I_assigned.swap(_I4_4I_copy);
    BOOST_CHECK_EQUAL( true,   _I4_4I_copy.empty() );
    BOOST_CHECK_EQUAL( _I4_4I, _I4_4I_assigned );

}

template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_add_sub_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    T v0 = make<T>(0);
    T v5 = make<T>(5);
    T v6 = make<T>(6);
    T v9 = make<T>(9);
    IntervalT I5_6I(v5,v6,interval_bounds::closed());
    IntervalT I5_9I(v5,v9,interval_bounds::closed());
    IntervalT I0_9I = IntervalT::closed(v0, v9);

    BOOST_CHECK_EQUAL( IntervalSet<T>(I5_6I).add(v0).add(v9), 
                       IntervalSet<T>().insert(v9).insert(I5_6I).insert(v0) );

    IntervalSet<T> set_A = IntervalSet<T>(I5_6I).add(v0).add(v9);
    IntervalSet<T> set_B = IntervalSet<T>().insert(v9).insert(I5_6I).insert(v0);
    BOOST_CHECK_EQUAL( set_A, set_B );
    BOOST_CHECK_EQUAL( hull(set_A), I0_9I );
    BOOST_CHECK_EQUAL( hull(set_A).lower(), I0_9I.lower() );
    BOOST_CHECK_EQUAL( hull(set_A).upper(), I0_9I.upper() );

    IntervalSet<T> set_A1 = set_A, set_B1 = set_B,
                   set_A2 = set_A, set_B2 = set_B;

    set_A1.subtract(I5_6I).subtract(v9);
    set_B1.erase(v9).erase(I5_6I);
    BOOST_CHECK_EQUAL( set_A1, set_B1 );

    set_A2.subtract(I5_9I);
    set_B2.erase(I5_9I);
    BOOST_CHECK_EQUAL( set_A1, set_B1 );
    BOOST_CHECK_EQUAL( set_A1, set_A2 );
    BOOST_CHECK_EQUAL( set_B1, set_B2 );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_distinct_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSet<T>::size_type       size_T;
    typedef typename IntervalSet<T>::difference_type diff_T;
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);

    size_T s3 = make<size_T>(3);
    

    IntervalSet<T> is_1_3_5;
    is_1_3_5.add(v1).add(v3).add(v5);

    BOOST_CHECK_EQUAL( cardinality(is_1_3_5),       s3 );
    BOOST_CHECK_EQUAL( is_1_3_5.size(),             s3 );
    BOOST_CHECK_EQUAL( interval_count(is_1_3_5),   3 );
    BOOST_CHECK_EQUAL( iterative_size(is_1_3_5),   3 );
    BOOST_CHECK_EQUAL( is_1_3_5.iterative_size(),   3 );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_distinct_4_bicremental_continuous_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSet<T>::size_type       size_T;
    typedef typename IntervalSet<T>::difference_type diff_T;
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);

    size_T s3 = make<size_T>(3);
    diff_T d0 = make<diff_T>(0);
    diff_T d2 = make<diff_T>(2);

    IntervalSet<T> is_1_3_5;
    is_1_3_5.add(v1).add(v3).add(v5);

    BOOST_CHECK_EQUAL( cardinality(is_1_3_5),      s3 );
    BOOST_CHECK_EQUAL( is_1_3_5.size(),             s3 );
    BOOST_CHECK_EQUAL( icl::length(is_1_3_5),       d0 );
    BOOST_CHECK_EQUAL( interval_count(is_1_3_5),   3 );
    BOOST_CHECK_EQUAL( is_1_3_5.iterative_size(),   3 );
    BOOST_CHECK_EQUAL( iterative_size(is_1_3_5),   3 );

    

    IntervalSet<T> is_123_5;
    is_123_5 = is_1_3_5;
    is_123_5 += IntervalT::open(v1,v3);

    BOOST_CHECK_EQUAL( cardinality(is_123_5),      icl::infinity<size_T>::value() );
    BOOST_CHECK_EQUAL( is_123_5.size(),             icl::infinity<size_T>::value() );
    BOOST_CHECK_EQUAL( icl::length(is_123_5),           d2 );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_isolate_4_bicremental_continuous_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSet<T>::size_type       size_T;
    typedef typename IntervalSet<T>::difference_type diff_T;

    T v0 = make<T>(0);
    T v2 = make<T>(2);
    T v4 = make<T>(4);
    IntervalT I0_4I = IntervalT::closed(v0,v4);
    IntervalT C0_2D = IntervalT::open(v0,v2);
    IntervalT C2_4D = IntervalT::open(v2,v4);
    //   {[0               4]}
    // - {   (0,2)   (2,4)   }
    // = {[0]     [2]     [4]}
    IntervalSet<T> iso_set = IntervalSet<T>(I0_4I);
    IntervalSet<T> gap_set;
    gap_set.add(C0_2D).add(C2_4D);
    BOOST_CHECK_EQUAL( true, true );
    iso_set -= gap_set;
    
    BOOST_CHECK_EQUAL( cardinality(iso_set), static_cast<size_T>(3) );
    BOOST_CHECK_EQUAL( iso_set.iterative_size(), static_cast<std::size_t>(3) );
    BOOST_CHECK_EQUAL( iterative_size(iso_set), static_cast<std::size_t>(3) );

    IntervalSet<T> iso_set2;
    iso_set2.add(I0_4I);
    iso_set2.subtract(C0_2D).subtract(C2_4D);
    
    IntervalSet<T> iso_set3(I0_4I);
    (iso_set3 -= C0_2D) -= C2_4D;

    IntervalSet<T> iso_set4;
    iso_set4.insert(I0_4I);
    iso_set4.erase(C0_2D).erase(C2_4D);
    
    BOOST_CHECK_EQUAL( iso_set, iso_set2 );
    BOOST_CHECK_EQUAL( iso_set, iso_set3 );
    BOOST_CHECK_EQUAL( iso_set, iso_set4 );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_element_compare_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef IntervalSet<T> ISet;

    BOOST_CHECK_EQUAL( is_element_equal( ISet(),         ISet()),         true );    
    BOOST_CHECK_EQUAL( is_element_equal( ISet(),         ISet(I_D(0,1))), false );    
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,1)), ISet()),         false );
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,1)), ISet(I_D(0,1))), true );

    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,5)), ISet(I_D(3,8))), false );    
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(3,8)), ISet(I_D(0,5))), false );    

    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,1)),          ISet(I_D(0,1))          ), true  );    
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,1)),          ISet(I_D(0,1))+I_D(1,2) ), false );
    BOOST_CHECK_EQUAL( is_element_equal( I_D(1,2)+ISet(I_D(0,1)), ISet(I_D(0,1))          ), false );
    BOOST_CHECK_EQUAL( is_element_equal( I_D(1,2)+ISet(I_D(0,1)), ISet(I_D(0,1))+I_D(1,2) ), true  );

    //[0   1)[1   2)
    //[0          2)
    BOOST_CHECK_EQUAL( is_element_equal( I_D(0,1)+ISet(I_D(1,2)), ISet(I_D(0,2))          ), true );
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,2)),          ISet(I_D(0,1))+I_D(1,2) ), true );

    //[0   1)  [2   3)
    //[0            3)
    BOOST_CHECK_EQUAL( is_element_equal( I_D(0,1)+ISet(I_D(2,3)), ISet(I_D(0,3))          ), false );
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(0,3)),          ISet(I_D(0,1))+I_D(2,3) ), false );

    //[0   2)[2       4)
    //  [1            4)
    BOOST_CHECK_EQUAL( is_element_equal( I_D(0,2)+ISet(I_D(2,4)), ISet(I_D(1,4))          ), false );
    BOOST_CHECK_EQUAL( is_element_equal( ISet(I_D(1,4)),          ISet(I_D(0,2))+I_D(2,4) ), false );

    //[0     2)[2       4)
    //[0 1)[1     3)[3  4)
    BOOST_CHECK_EQUAL( is_element_equal( I_D(0,2)+ISet(I_D(2,4)), I_D(0,1)+ISet(I_D(1,4))+I_D(3,4) ), true );
    BOOST_CHECK_EQUAL( is_element_equal( I_D(0,1)+ISet(I_D(1,4))+I_D(3,4), I_D(0,2)+ISet(I_D(2,4)) ), true );
}

template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_contains_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    //LAW: x.add(e).contains(e);
    //LAW: z = x + y => contains(z, x) && contains(z, y);
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    T v9 = make<T>(9);
    T v11 = make<T>(11);
    IntervalSet<T> is(v1);    
    BOOST_CHECK_EQUAL( icl::contains(is, v1), true );

    BOOST_CHECK_EQUAL( icl::contains(IntervalSet<T>().add(make<T>(2)), make<T>(2)), true );
    BOOST_CHECK_EQUAL( icl::contains(IntervalSet<T>().insert(make<T>(2)), make<T>(2)), true );
    BOOST_CHECK_EQUAL( icl::contains((is += IntervalT(v3,v7)), IntervalT(v3,v7)), true );

    IntervalSet<T> is0 = is;    

    IntervalSet<T> is2(IntervalT::closed(v5,v8));
    is2.add(v9).add(v11);
    is += is2;
    BOOST_CHECK_EQUAL( contains(is, is2), true );    

    is = is0;
    IntervalSet<T> is3(IntervalT::closed(v5,v8));
    is3.insert(v9).insert(v11);
    is += is3;
    BOOST_CHECK_EQUAL( contains(is, is3), true );    
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_operators_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v3 = make<T>(3);
    T v5 = make<T>(5);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    IntervalSet<T> left, left2, right, all, all2, section, complement, naught;
    left.add(IntervalT::closed(v0,v1)).add(IntervalT::closed(v3,v5));
    (right += IntervalT::closed(v3,v5)) += IntervalT::closed(v7,v8);

    BOOST_CHECK_EQUAL( disjoint(left, right), false );

    (all += left) += right;
    (section += left) &= right;
    (complement += all) -= section;
    (all2 += section) += complement; 

    BOOST_CHECK_EQUAL( disjoint(section, complement), true );
    BOOST_CHECK_EQUAL( all, all2 );

    BOOST_CHECK_EQUAL( icl::contains(all, left), true );
    BOOST_CHECK_EQUAL( icl::contains(all, right), true );
    BOOST_CHECK_EQUAL( icl::contains(all, complement), true );

    BOOST_CHECK_EQUAL( icl::contains(left, section), true );
    BOOST_CHECK_EQUAL( icl::contains(right, section), true );

    BOOST_CHECK_EQUAL( within(left, all), true );
    BOOST_CHECK_EQUAL( within(right, all), true );
    BOOST_CHECK_EQUAL( within(complement, all), true );
    BOOST_CHECK_EQUAL( within(section, left), true );
    BOOST_CHECK_EQUAL( within(section, right), true );
}


// Test for nontrivial intersection of interval sets with intervals and values
template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_base_intersect_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    T v0 = make<T>(0);
    T v1 = make<T>(1);
    T v2 = make<T>(2);
    T v3 = make<T>(3);
    T v6 = make<T>(6);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    T v9 = make<T>(9);

    IntervalT I0_3D = IntervalT::right_open(v0,v3);
    IntervalT I1_3D = IntervalT::right_open(v1,v3);
    IntervalT I1_8D = IntervalT::right_open(v1,v8);
    IntervalT I2_7D = IntervalT::right_open(v2,v7);
    IntervalT I2_3D = IntervalT::right_open(v2,v3);
    IntervalT I6_7D = IntervalT::right_open(v6,v7);
    IntervalT I6_8D = IntervalT::right_open(v6,v8);
    IntervalT I6_9D = IntervalT::right_open(v6,v9);

    //--------------------------------------------------------------------------
    // IntervalSet
    //--------------------------------------------------------------------------
    //split_A      [0       3)       [6    9)
    //         &=      [1                8)
    //split_AB ->      [1   3)       [6  8)
    //         &=        [2             7)     
    //         ->        [2 3)       [6 7)
    IntervalSet<T>    split_A, split_B, split_AB, split_ab, split_ab2;

    split_A.add(I0_3D).add(I6_9D);
    split_AB = split_A;
    split_AB &= I1_8D;
    split_ab.add(I1_3D).add(I6_8D);

    BOOST_CHECK_EQUAL( split_AB, split_ab );

    split_AB = split_A;
    (split_AB &= I1_8D) &= I2_7D;
    split_ab2.add(I2_3D).add(I6_7D);

    BOOST_CHECK_EQUAL( split_AB, split_ab2 );


    //--------------------------------------------------------------------------
    //split_A      [0       3)       [6    9)
    //         &=       1
    //split_AB ->      [1]
    //         +=         (1             7)     
    //         ->      [1](1             7)
    split_A.add(I0_3D).add(I6_9D);
    split_AB = split_A;
    split_AB &= v1;
    split_ab.clear();
    split_ab.add(v1);

    BOOST_CHECK_EQUAL( split_AB, split_ab );

    split_AB = split_A;
    (split_AB &= v1) += IntervalT::open(v1,v7);
    split_ab2.clear();
    split_ab2 += IntervalT::right_open(v1,v7);

    BOOST_CHECK_EQUAL( is_element_equal(split_AB, split_ab2), true );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_flip_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef IntervalSetT ISet;

    IntervalSetT set_a, set_b, lhs, rhs;
    //[0     2)
    //    [1     3)
    //[0 1)   [2 3) : {[0 2)} ^= [2 3)
    //gcc seed ambiguities with std::_Ios_Iostate& std::operator^= here:
    // BOOST_CHECK_EQUAL(ISet(I_D(0,2)) ^= I_D(1,3), ISet(I_D(0,1)) + I_D(2,3));
    set_a = ISet(I_D(0,2));
    BOOST_CHECK_EQUAL(set_a ^= I_D(1,3), ISet(I_D(0,1)) + I_D(2,3));

    //    [1     3)
    //[0     2)    
    //[0 1)   [2 3) : {[1 3)} ^= [0 2)
    set_a = ISet(I_D(1,3));
    BOOST_CHECK_EQUAL(set_a ^= I_D(0,2), ISet(I_D(0,1)) + I_D(2,3));

    //[0     2)      (3  5]
    //    [1      3)
    //[0 1)   [2  3) (3  5] : a ^= b
    set_a.clear();
    set_a.add(I_D(0,2)).add(C_I(3,5));
    set_b.add(I_D(1,3));
    lhs = set_a;
    lhs ^= set_b;
    rhs.add(I_D(0,1)).add(I_D(2,3)).add(C_I(3,5));
    BOOST_CHECK_EQUAL(lhs, rhs);
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_infix_plus_overload_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    IntervalT itv = I_D(3,5);

    IntervalSetT set_a, set_b;
    set_a.add(C_D(1,3)).add(I_D(8,9)).add(I_I(6,11));
    set_b.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    BOOST_CHECK_EQUAL(set_a + set_b, set_b + set_a);
    // This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(set_a + itv, itv + set_a);
    BOOST_CHECK_EQUAL(set_b + MK_v(4), MK_v(4) + set_b);
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_infix_pipe_overload_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a, set_b;
    set_a.add(C_D(1,3)).add(I_D(8,9)).add(I_I(6,11));
    set_b.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    BOOST_CHECK_EQUAL(set_a | set_b, set_b | set_a);
    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(set_a | itv, itv | set_a);
    BOOST_CHECK_EQUAL(set_b | MK_v(4), MK_v(4) | set_b);
}



template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_infix_minus_overload_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a, set_b;
    set_a.add(C_D(1,3)).add(I_D(8,9)).add(I_I(6,11));
    set_b.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    BOOST_CHECK_EQUAL(set_a - set_b, (set_b + set_a) - set_b);
    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(set_a - itv, (itv + set_a)  - itv);
    BOOST_CHECK_EQUAL(set_b - MK_v(4), (MK_v(4) + set_b) - MK_v(4));
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_infix_et_overload_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a, set_b;
    set_a.add(C_D(1,3)).add(I_D(8,9)).add(I_I(6,11));
    set_b.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    BOOST_CHECK_EQUAL(set_a & set_b, set_b & set_a);
    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(set_a & itv, itv & set_a);
    BOOST_CHECK_EQUAL(set_b & MK_v(4), MK_v(4) & set_b);
}



template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_infix_caret_overload_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a, set_b;
    set_a.add(C_D(1,3)).add(I_D(8,9)).add(I_I(6,11));
    set_b.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    BOOST_CHECK_EQUAL(set_a ^ set_b, set_b ^ set_a);
    //This checks all cases of is_interval_set_derivative<T>
    BOOST_CHECK_EQUAL(set_a ^ itv, itv ^ set_a);
    BOOST_CHECK_EQUAL(set_b ^ MK_v(4), MK_v(4) ^ set_b);
}



template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_find_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSetT::const_iterator c_iterator;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a;
    set_a.add(C_D(1,3)).add(I_I(6,11));

    typename IntervalSetT::const_iterator found = set_a.find(MK_v(6));

    BOOST_CHECK_EQUAL( *found, I_I(6,11) );

    found = set_a.find(MK_v(5));

    BOOST_CHECK_EQUAL( found == set_a.end(), true );

    c_iterator found1 = set_a.find(MK_v(6));
    c_iterator found2 = icl::find(set_a, MK_v(6));

    BOOST_CHECK      ( found1 == found2 );
    BOOST_CHECK_EQUAL( *found1, *found2 );
    BOOST_CHECK_EQUAL( *found1, I_I(6,11) );

    found1 = set_a.find(MK_v(5));

    BOOST_CHECK_EQUAL( found1 == set_a.end(), true );

    //LAW map c; key k: k in dom(c) => contains(c, *find(c, k))
    BOOST_CHECK( icl::contains(set_a, *icl::find(set_a, MK_v(2))) );
    BOOST_CHECK( icl::contains(set_a, *set_a.find(MK_v(11))) );

    BOOST_CHECK(  icl::contains(set_a, MK_v(2)) );
    BOOST_CHECK(  icl::contains(set_a, MK_v(10)) );
    BOOST_CHECK( !icl::contains(set_a, MK_v(1)) );
    BOOST_CHECK( !icl::contains(set_a, MK_v(3)) );

    BOOST_CHECK(  icl::intersects(set_a, MK_v(2)) );
    BOOST_CHECK(  icl::intersects(set_a, MK_v(10)) );
    BOOST_CHECK( !icl::intersects(set_a, MK_v(1)) );
    BOOST_CHECK( !icl::intersects(set_a, MK_v(3)) );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_intersects_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSetT::key_type      KeyT;

    IntervalT between = I_D(3,5);

    IntervalSetT set_a;
    set_a.add(C_D(1,3)).add(I_I(6,11));
    //         (1   3)         [6    11]
    BOOST_CHECK( icl::intersects(set_a, MK_v(2)) );
    BOOST_CHECK( icl::intersects(set_a, MK_v(11)) );
    BOOST_CHECK( icl::disjoint(set_a, MK_v(3)) );
    BOOST_CHECK( icl::disjoint(set_a, MK_v(5)) );

    BOOST_CHECK( icl::intersects(set_a, C_D(1,3)) );
    BOOST_CHECK( icl::intersects(set_a, I_D(8,10)) );
    BOOST_CHECK( icl::disjoint(set_a, between) );
    BOOST_CHECK( icl::disjoint(set_a, I_I(0,1)) );

    IntervalSetT to_12 = IntervalSetT(I_D(0, 13));
    IntervalSetT complement_a = to_12 - set_a;
    BOOST_CHECK( icl::disjoint(set_a, complement_a) );
    BOOST_CHECK( icl::intersects(to_12, set_a) );

    BOOST_CHECK_EQUAL( icl::lower(set_a), icl::lower(*(set_a.begin())) );
    BOOST_CHECK_EQUAL( icl::lower(set_a), MK_v(1) );
    BOOST_CHECK_EQUAL( icl::upper(set_a), icl::upper(*(set_a.rbegin())) );
    BOOST_CHECK_EQUAL( icl::upper(set_a), MK_v(11) );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_range_4_discrete_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSetT::key_type      KeyT;

    IntervalT between = I_D(3,5);

    IntervalSetT set_a;
    set_a.add(C_D(1,3)).add(I_I(6,11));
    //         (1   3)         [6    11]
    BOOST_CHECK_EQUAL( icl::first(set_a), icl::first(*(set_a.begin())) );
    BOOST_CHECK_EQUAL( icl::first(set_a), MK_v(2) );
    BOOST_CHECK_EQUAL( icl::last(set_a), icl::last(*(set_a.rbegin())) );
    BOOST_CHECK_EQUAL( icl::last(set_a), MK_v(11) );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_bitset_find_4_integral_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef typename IntervalSetT::bitset_type BitsT;

    IntervalT itv = I_D(3,5);

    IntervalSetT set_a;
    set_a.add(C_D(1,3)).add(I_I(6,11));

    typename IntervalSetT::const_iterator found = set_a.find(MK_v(6));

    BOOST_CHECK( (found->second).contains(6) );

    found = set_a.find(MK_v(5));
    BOOST_CHECK( found == set_a.end() );

    set_a.add(MK_v(64));
    found = set_a.find(MK_v(64));
    BOOST_CHECK( (found->second).contains(0) );

    set_a.add(MK_v(65));
    found = set_a.find(MK_v(65));
    BOOST_CHECK( (found->second).contains(1) );

    found = set_a.find(MK_v(66));
    BOOST_CHECK( found == set_a.end() );
}

template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_element_iter_4_discrete_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::vector<T> VectorT;

    IntervalSetT set_a;
    set_a.add(I_I(1,3)).add(I_I(6,7));

    VectorT vec(5), cev(5);
    vec[0]=MK_v(1);vec[1]=MK_v(2);vec[2]=MK_v(3);vec[3]=MK_v(6);vec[4]=MK_v(7);
    cev[0]=MK_v(7);cev[1]=MK_v(6);cev[2]=MK_v(3);cev[3]=MK_v(2);cev[4]=MK_v(1);

    VectorT dest;
    std::copy(elements_begin(set_a), elements_end(set_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( vec == dest, true );

    dest.clear();
    std::copy(elements_rbegin(set_a), elements_rend(set_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( cev == dest, true );

    dest.clear();
    std::reverse_copy(elements_begin(set_a), elements_end(set_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( cev == dest, true );

    dest.clear();
    std::reverse_copy(elements_rbegin(set_a), elements_rend(set_a), std::back_inserter(dest));
    BOOST_CHECK_EQUAL( vec == dest, true );
}


template <ICL_IntervalSet_TEMPLATE(_T) IntervalSet, class T>
void interval_set_move_4_discrete_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::vector<T> VectorT;

    //JODO static_cast fails for gcc compilers
    //IntervalSetT set_A(boost::move(static_cast<IntervalSetT&>(IntervalSetT(I_D(0,4)))));
    IntervalSetT set_A(boost::move(static_cast<IntervalSetT&>(IntervalSetT(I_D(0,4)).add(I_D(0,0)) )));
    IntervalSetT set_B(boost::move(static_cast<IntervalSetT&>(IntervalSetT(I_D(0,2)).add(I_D(2,4)).add(I_D(0,4)))));

    BOOST_CHECK( icl::is_element_equal(set_A, set_B) );
    BOOST_CHECK_EQUAL( set_A, join(set_B) );

    //JODO static_cast fails for gcc compilers
    //set_A = boost::move(static_cast<IntervalSetT&>(IntervalSetT(I_I(1,4))));
    set_A = boost::move(static_cast<IntervalSetT&>(IntervalSetT(I_I(1,4)).add(I_D(0,0))));
    set_B = boost::move(static_cast<IntervalSetT&>(IntervalSetT(C_I(0,2)).insert(I_D(3,5)).add(C_D(0,5))));

    BOOST_CHECK( icl::is_element_equal(set_A, set_B) );
    BOOST_CHECK_EQUAL( set_A, join(set_B) );
}



#endif // LIBS_ICL_TEST_TEST_INTERVAL_SET_SHARED_HPP_JOFA_080920

