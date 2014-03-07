/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_ICL_interval_shared_hpp_JOFA_100306__
#define LIBS_ICL_TEST_TEST_ICL_interval_shared_hpp_JOFA_100306__

#include <boost/icl/interval_set.hpp>

template <class DomainT, ICL_COMPARE Compare, 
          ICL_INTERVAL(ICL_COMPARE)  Interval>
void test_inner_complement(const ICL_INTERVAL_TYPE(Interval,DomainT,Compare)& itv1,
                           const ICL_INTERVAL_TYPE(Interval,DomainT,Compare)& itv2)
{
    typedef interval_set<DomainT,Compare,Interval> ItvSetT;
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) IntervalT;

    BOOST_CHECK_EQUAL(icl::length(inner_complement(itv1,itv2)), icl::distance(itv1,itv2));
    BOOST_CHECK_EQUAL(icl::length(inner_complement(itv1,itv2)), icl::distance(itv2,itv1));
    BOOST_CHECK_EQUAL(icl::length(inner_complement(itv2,itv1)), icl::distance(itv1,itv2));
    BOOST_CHECK_EQUAL(icl::length(inner_complement(itv2,itv1)), icl::distance(itv2,itv1));

    IntervalT in_comp = inner_complement(itv1,itv2);
    ItvSetT itvset, inner_comp;
    itvset.add(itv1).add(itv2);
    ItvSetT hullset = ItvSetT(hull(itvset));
    inner_comp = hullset - itvset;
    IntervalT inner_comp_itv;
    if(inner_comp.begin() != inner_comp.end())
        inner_comp_itv = *inner_comp.begin();

    BOOST_CHECK_EQUAL(inner_complement(itv1,itv2), inner_comp_itv);
    BOOST_CHECK_EQUAL(inner_complement(itv2,itv1), inner_comp_itv);
    BOOST_CHECK_EQUAL(icl::length(inner_comp), icl::distance(itv1,itv2));
    BOOST_CHECK_EQUAL(icl::length(inner_comp), icl::distance(itv2,itv1));

    BOOST_CHECK(icl::disjoint(itv1, in_comp));
    BOOST_CHECK(icl::disjoint(itv2, in_comp));

    IntervalT itv1_comp = hull(itv1, in_comp);
    IntervalT itv2_comp = hull(itv2, in_comp);

    if(!icl::is_empty(in_comp))
    {
        BOOST_CHECK(icl::intersects(itv1_comp, in_comp));
        BOOST_CHECK(icl::intersects(itv2_comp, in_comp));

        BOOST_CHECK_EQUAL(itv1_comp & itv2_comp, in_comp);
        BOOST_CHECK_EQUAL( icl::is_empty(itv1_comp & itv2_comp), icl::disjoint(itv1_comp, itv2_comp));
        BOOST_CHECK_EQUAL(!icl::is_empty(itv1_comp & itv2_comp), icl::intersects(itv1_comp, itv2_comp));
    }
}

template <class IntervalT>
void test_inner_complement_(const IntervalT& itv1, const IntervalT& itv2)
{
    typedef typename interval_traits<IntervalT>::domain_type DomainT;
    // For the test of plain interval types we assume that std::less is
    // the compare functor
    test_inner_complement<DomainT, std::less, IntervalT>(itv1, itv2);
}

#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS

void interval_ctor_specific()
{
    BOOST_CHECK_EQUAL(icl::length(icl::interval<double>::type()), 0.0);
    BOOST_CHECK_EQUAL(icl::cardinality(icl::interval<double>::closed(5.0, 5.0)), 1);
    BOOST_CHECK_EQUAL(icl::cardinality(icl::interval<std::string>::closed("test", "test")), 1);
    BOOST_CHECK_EQUAL(icl::cardinality(icl::interval<std::string>::closed("best","test")), 
                      icl::cardinality(icl::interval<double>::closed(0.0,0.1)));
    BOOST_CHECK_EQUAL(icl::cardinality(icl::interval<std::string>::right_open("best","test")), 
                      icl::infinity<size_type_of<icl::interval<std::string>::type>::type >::value() );
    BOOST_CHECK_EQUAL(icl::cardinality(icl::interval<double>::right_open(0.0, 1.0)), 
                      icl::infinity<size_type_of<icl::interval<double>::type>::type >::value() );
}

#endif // ndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS

template <class T> 
void interval_equal_4_integral_types()
{
    typedef typename icl::interval<T>::type IntervalT;
    T v2 = make<T>(2);
    T v3 = make<T>(3);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    BOOST_CHECK_EQUAL(IntervalT(), IntervalT(v7,v3));

    //I: (I)nside  = closed bound
    //C: left open bound
    //D: right open bound
    IntervalT  I3_7I  = icl::interval<T>::closed(v3,v7);
    IntervalT  I3__8D = icl::interval<T>::right_open(v3,v8);
    IntervalT C2__7I  = icl::interval<T>::left_open(v2,v7);
    IntervalT C2___8D = icl::interval<T>::open(v2,v8);

    BOOST_CHECK_EQUAL(  I3_7I ,  I3_7I  );    
    BOOST_CHECK_EQUAL(  I3_7I ,  I3__8D );    
    BOOST_CHECK_EQUAL(  I3_7I , C2__7I  );    
    BOOST_CHECK_EQUAL(  I3_7I , C2___8D );    

    BOOST_CHECK_EQUAL(  I3__8D,  I3__8D );    
    BOOST_CHECK_EQUAL(  I3__8D, C2__7I  );    
    BOOST_CHECK_EQUAL(  I3__8D, C2___8D );    

    BOOST_CHECK_EQUAL( C2__7I , C2__7I  );    
    BOOST_CHECK_EQUAL( C2__7I , C2___8D );    

    BOOST_CHECK_EQUAL( C2___8D, C2___8D );    
}

template <class T> 
void interval_less_4_integral_types()
{
    typedef typename icl::interval<T>::type IntervalT;
    T v2 = make<T>(2);
    T v3 = make<T>(3);
    T v4 = make<T>(4);
    T v7 = make<T>(7);
    T v8 = make<T>(8);
    BOOST_CHECK_EQUAL(IntervalT() < IntervalT(v7,v3), false);
    BOOST_CHECK_EQUAL(icl::interval<T>::open(v2,v3) < icl::interval<T>::right_open(v7,v7), false);
    BOOST_CHECK_EQUAL(icl::interval<T>::left_open(v3,v3) < icl::interval<T>::closed(v7,v3), false);

    BOOST_CHECK_EQUAL(IntervalT() < IntervalT(v3,v4), true);
    BOOST_CHECK_EQUAL(icl::interval<T>::open(v2,v3) < icl::interval<T>::right_open(v7,v8), true);

    //I: (I)nside  = closed bound
    //C: left open bound
    //D: right open bound
    IntervalT  I3_7I  = icl::interval<T>::closed(v3,v7);
    IntervalT  I4_7I  = icl::interval<T>::closed(v4,v7);

    IntervalT  I3__8D = icl::interval<T>::right_open(v3,v8);
    IntervalT C2__7I  = icl::interval<T>::left_open(v2,v7);
    IntervalT C2___8D = icl::interval<T>::open(v2,v8);

    BOOST_CHECK_EQUAL(  I3_7I <  I3_7I  , false);    
    BOOST_CHECK_EQUAL(  I3_7I <  I3__8D , false);    
    BOOST_CHECK_EQUAL(  I3_7I < C2__7I  , false);    
    BOOST_CHECK_EQUAL(  I3_7I < C2___8D , false);    

    BOOST_CHECK_EQUAL(  I3_7I <  I4_7I  , true);    


    BOOST_CHECK_EQUAL(  I3__8D<  I3__8D , false);    
    BOOST_CHECK_EQUAL(  I3__8D< C2__7I  , false);    
    BOOST_CHECK_EQUAL(  I3__8D< C2___8D , false);    

    BOOST_CHECK_EQUAL( C2__7I < C2__7I  , false);    
    BOOST_CHECK_EQUAL( C2__7I < C2___8D , false);    

    BOOST_CHECK_EQUAL( C2___8D< C2___8D , false);    
}

template <class T> 
void interval_equal_4_bicremental_continuous_types()
{
    typedef typename icl::interval<T>::type IntervalT;
    T v3 = make<T>(3);
    T v7 = make<T>(7);
    BOOST_CHECK_EQUAL(IntervalT(), IntervalT(v7,v3));

    //I: (I)nside  = closed bound
    //O: (O)utside = open bound
    IntervalT I3_7I = icl::interval<T>::closed(v3,v7);
    IntervalT I3_7D = icl::interval<T>::right_open(v3,v7);
    IntervalT C3_7I = icl::interval<T>::left_open(v3,v7);
    IntervalT C3_7D = icl::interval<T>::open(v3,v7);

    BOOST_CHECK_EQUAL( I3_7I ,  I3_7I  );    
    BOOST_CHECK_EQUAL( I3_7I == I3_7D, false  );    
    BOOST_CHECK_EQUAL( I3_7I == C3_7D, false  );    
    BOOST_CHECK_EQUAL( I3_7I == C3_7D, false );    
    BOOST_CHECK_EQUAL( I3_7I != I3_7D, true  );    
    BOOST_CHECK_EQUAL( I3_7I != C3_7D, true  );    
    BOOST_CHECK_EQUAL( I3_7I != C3_7D, true );    

    BOOST_CHECK_EQUAL( I3_7D ,  I3_7D  );    
    BOOST_CHECK_EQUAL( I3_7D == C3_7I, false  );    
    BOOST_CHECK_EQUAL( I3_7D == C3_7D, false );    
    BOOST_CHECK_EQUAL( I3_7D != C3_7I, true  );    
    BOOST_CHECK_EQUAL( I3_7D != C3_7D, true );    

    BOOST_CHECK_EQUAL( C3_7I ,  C3_7I  );    
    BOOST_CHECK_EQUAL( C3_7I == C3_7D, false );    
    BOOST_CHECK_EQUAL( C3_7I != C3_7D, true );    

    BOOST_CHECK_EQUAL( C3_7D,   C3_7D  );    
} 

template <class T> 
void interval_touches_4_bicremental_types()
{
    typedef typename icl::interval<T>::type IntervalT;
    T v3 = make<T>(3);
    T v7 = make<T>(7);
    T v9 = make<T>(9);

    IntervalT I3_7D = icl::interval<T>::right_open(v3,v7);
    IntervalT I7_9I = icl::interval<T>::closed(v7,v9);
    BOOST_CHECK_EQUAL( icl::touches(I3_7D, I7_9I), true );    

    IntervalT I3_7I = icl::interval<T>::closed(v3,v7);
    IntervalT C7_9I = icl::interval<T>::left_open(v7,v9);
    BOOST_CHECK_EQUAL( icl::touches(I3_7I, C7_9I), true );

    BOOST_CHECK_EQUAL( icl::touches(I3_7D, C7_9I), false );    
    BOOST_CHECK_EQUAL( icl::touches(I3_7I, I7_9I), false );    
}

template <class T> 
void interval_touches_4_integral_types()
{
    typedef typename icl::interval<T>::type IntervalT;
    T v3 = make<T>(3);
    T v6 = make<T>(6);
    T v7 = make<T>(7);
    T v9 = make<T>(9);

    IntervalT I3_6I = icl::interval<T>::closed(v3,v6);
    IntervalT I7_9I = icl::interval<T>::closed(v7,v9);
    BOOST_CHECK_EQUAL( icl::touches(I3_6I, I7_9I), true );    

    IntervalT I3_7D = icl::interval<T>::right_open(v3,v7);
    IntervalT C6_9I = icl::interval<T>::left_open(v6,v9);
    BOOST_CHECK_EQUAL( icl::touches(I3_7D, C6_9I), true );
}

template <class T> 
void interval_infix_intersect_4_bicremental_types()
{
    typedef typename icl::interval<T>::type IntervalT;

    IntervalT section;
    IntervalT I3_7D = I_D(3,7);

    IntervalT I0_3D = I_D(0,3);
    section = I3_7D & I0_3D;
    BOOST_CHECK_EQUAL( icl::disjoint(I0_3D, I3_7D), true );
    BOOST_CHECK_EQUAL( icl::is_empty(section), true );
    BOOST_CHECK_EQUAL( section, IntervalT() );

    IntervalT I0_5D = I_D(0,5);
    section = I3_7D & I0_5D;
    BOOST_CHECK_EQUAL( section, I_D(3,5) );

    IntervalT I0_9D = I_D(0,9);
    section = I3_7D & I0_9D;
    BOOST_CHECK_EQUAL( section, I3_7D );

    IntervalT I4_5I = I_I(4,5);
    section = I3_7D & I4_5I;
    BOOST_CHECK_EQUAL( section, I4_5I );

    IntervalT C4_6D = C_D(4,6);
    section = I3_7D & C4_6D;
    BOOST_CHECK_EQUAL( section, C4_6D );

    IntervalT C4_9I = C_I(4,9);
    section = I3_7D & C4_9I;
    BOOST_CHECK_EQUAL( section, C_D(4,7) );

    IntervalT I7_9I = I_I(7,9);
    section = I3_7D & I7_9I;
    BOOST_CHECK_EQUAL( icl::exclusive_less(I3_7D, I7_9I), true );
    BOOST_CHECK_EQUAL( icl::disjoint(I3_7D, I7_9I), true );
    BOOST_CHECK_EQUAL( icl::is_empty(section), true );
}

template <class T> 
void interval_subtract_4_bicremental_types()
{
    typedef typename icl::interval<T>::type IntervalT;

    IntervalT diff_1, diff_2;
    IntervalT I0_3D = I_D(0,3);
    IntervalT I2_6D = I_D(2,6);
    IntervalT I4_7D = I_D(4,7);
    IntervalT I6_7D = I_D(6,7);
    IntervalT I2_4D = I_D(2,4);

    diff_1 = right_subtract(I2_6D, I4_7D);
    BOOST_CHECK_EQUAL( diff_1, I2_4D );

    diff_1 = right_subtract(I0_3D, I4_7D);
    BOOST_CHECK_EQUAL( diff_1, I0_3D );
    
    // ---------------------------------
    diff_1 = left_subtract(I4_7D, I2_6D);
    BOOST_CHECK_EQUAL( diff_1, I6_7D );

    diff_1 = left_subtract(I4_7D, I0_3D);
    BOOST_CHECK_EQUAL( diff_1, I4_7D );
}


#endif // LIBS_ICL_TEST_TEST_ICL_interval_shared_hpp_JOFA_100306__
