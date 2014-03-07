/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_ICL_INTERVAL_CONTINUOUS_HPP_JOFA_100930
#define BOOST_ICL_TEST_ICL_INTERVAL_CONTINUOUS_HPP_JOFA_100930

template <class T, class IntervalT> 
void interval_ctor_4_ordered_types()
{
    BOOST_CHECK_EQUAL(icl::is_empty(IntervalT()), true);
    BOOST_CHECK_EQUAL(icl::cardinality(IntervalT()), icl::identity_element<typename icl::size_type_of<T>::type>::value());
    BOOST_CHECK_EQUAL(icl::size(IntervalT()), icl::identity_element<typename icl::size_type_of<T>::type>::value());

    BOOST_CHECK_EQUAL( IntervalT(), IntervalT() );
    BOOST_CHECK_EQUAL( IntervalT(), IntervalT(IntervalT().lower(), IntervalT().upper()) );
    BOOST_CHECK_EQUAL( IntervalT(), IntervalT(icl::lower(IntervalT()), icl::upper(IntervalT())) );
}


template <class T, ICL_COMPARE Compare>
void distant_intervals_4_numeric_continuous_types()
{
    typedef  right_open_interval<T,Compare> L__D; // L__D for [..)
    typedef   left_open_interval<T,Compare> C__I; // C__I for (..]
    typedef  typename  icl::interval<T,Compare>::type IntervalT;

    BOOST_CHECK( is_interval<L__D>::value ); 
    BOOST_CHECK( has_difference<typename interval_traits<L__D>::domain_type>::value ); 
    BOOST_CHECK( is_continuous<typename interval_traits<L__D>::domain_type>::value  ); 
    BOOST_CHECK( (boost::is_same<typename interval_traits<L__D>::domain_type, T>::value)   ); 

    typedef typename difference_type_of<T>::type DiffT;

    test_inner_complement<T,Compare,L__D>(MK_I(L__D,0,4), MK_I(L__D,8,9));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,7,8), MK_I(L__D,2,3));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,2,4), MK_I(L__D,4,6));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,3,7), MK_I(L__D,5,8));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,7,9), MK_I(L__D,0,4));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,0,0), MK_I(L__D,0,0));
    test_inner_complement<T,Compare,L__D>(MK_I(L__D,1,0), MK_I(L__D,2,0));

    test_inner_complement<T,Compare,C__I>(MK_I(C__I,1,5), MK_I(C__I,4,9));
    test_inner_complement<T,Compare,C__I>(MK_I(C__I,4,6), MK_I(C__I,1,3));
    test_inner_complement<T,Compare,C__I>(MK_I(C__I,0,2), MK_I(C__I,4,6));
    test_inner_complement<T,Compare,C__I>(MK_I(C__I,0,2), MK_I(C__I,0,6));

    //--------------------------------------------------------------------------
#ifndef BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
    test_inner_complement<T,Compare,IntervalT>(I_D(0,4), I_D(8,9));
    test_inner_complement<T,Compare,IntervalT>(I_D(7,8), I_D(2,3));
    test_inner_complement<T,Compare,IntervalT>(I_D(2,4), I_D(4,6));
    test_inner_complement<T,Compare,IntervalT>(I_D(3,7), I_D(5,8));
    test_inner_complement<T,Compare,IntervalT>(I_D(7,9), I_D(0,4));
    test_inner_complement<T,Compare,IntervalT>(I_D(0,0), I_D(0,0));
    test_inner_complement<T,Compare,IntervalT>(I_D(1,0), I_D(2,0));

    test_inner_complement<T,Compare,IntervalT>(C_I(1,5), C_I(4,9));
    test_inner_complement<T,Compare,IntervalT>(C_I(4,6), C_I(1,3));
    test_inner_complement<T,Compare,IntervalT>(C_I(0,2), C_I(4,6));
    test_inner_complement<T,Compare,IntervalT>(C_I(0,2), C_I(0,6));

    test_inner_complement<T,Compare,IntervalT>(I_I(7,9), I_I(0,5));
    test_inner_complement<T,Compare,IntervalT>(I_I(0,9), I_I(2,5));

    test_inner_complement<T,Compare,IntervalT>(C_D(6,9), C_D(1,4));
    test_inner_complement<T,Compare,IntervalT>(C_D(1,3), C_D(1,4));
    test_inner_complement<T,Compare,IntervalT>(C_D(1,3), C_D(6,8));
    test_inner_complement<T,Compare,IntervalT>(C_D(1,7), C_D(1,6));
    test_inner_complement<T,Compare,IntervalT>(C_D(1,1), C_D(1,1));
    test_inner_complement<T,Compare,IntervalT>(C_D(3,0), C_D(4,0));
    test_inner_complement<T,Compare,IntervalT>(C_D(0,2), C_D(4,6));
    test_inner_complement<T,Compare,IntervalT>(C_D(0,2), C_D(0,6));
#endif //BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
}



#endif // BOOST_ICL_TEST_ICL_INTERVAL_CONTINUOUS_HPP_JOFA_100930
