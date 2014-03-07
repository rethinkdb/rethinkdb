/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_ICL_DISCRETE_INTERVAL_HPP_JOFA_100930
#define BOOST_ICL_TEST_ICL_DISCRETE_INTERVAL_HPP_JOFA_100930


template <class T, class IntervalT> 
void discrete_interval_traits()
{
    BOOST_CHECK( is_interval<IntervalT>::value                        );
    BOOST_CHECK( is_discrete_interval<IntervalT>::value               );
    BOOST_CHECK( is_discrete<typename IntervalT::domain_type>::value  );
    BOOST_CHECK(!is_continuous<typename IntervalT::domain_type>::value);
    BOOST_CHECK( has_dynamic_bounds<IntervalT>::value                 );
    BOOST_CHECK(!has_static_bounds<IntervalT>::value                  );
}

template <class T, class IntervalT> 
void discrete_interval_ctor__dis_4_dyn_v_sta() // discrete && (dynamic or static)
{
    BOOST_CHECK_EQUAL( IntervalT(MK_v(3)), IntervalT(MK_v(3)) );
    BOOST_CHECK_EQUAL( icl::contains(IntervalT(MK_v(1)), MK_v(1)), true ); 
}

template <class T, ICL_COMPARE Compare>
void distant_intervals_4_discrete_types()
{
    typedef right_open_interval<T,Compare> L__D; // L__D for [..)
    typedef  left_open_interval<T,Compare> C__I; // C__I for (..]
    typedef    closed_interval<T,Compare> L__I; // L__I for [..]
    typedef      open_interval<T,Compare> C__D; // C__D for (..)
    typedef typename icl::interval<T,Compare>::type IntervalT;

    BOOST_CHECK( is_interval<L__D>::value ); 
    BOOST_CHECK( has_difference<typename interval_traits<L__D>::domain_type>::value ); 
    BOOST_CHECK( is_discrete<typename interval_traits<L__D>::domain_type>::value    ); 
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

    test_inner_complement<T,Compare,L__I>(MK_I(L__I,7,9), MK_I(L__I,0,5));
    test_inner_complement<T,Compare,L__I>(MK_I(L__I,0,9), MK_I(L__I,2,5));

    test_inner_complement<T,Compare,C__D>(MK_I(C__D,6,9), MK_I(C__D,1,4));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,1,3), MK_I(C__D,1,4));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,1,3), MK_I(C__D,6,8));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,1,7), MK_I(C__D,1,6));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,1,1), MK_I(C__D,1,1));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,3,0), MK_I(C__D,4,0));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,0,2), MK_I(C__D,4,6));
    test_inner_complement<T,Compare,C__D>(MK_I(C__D,0,2), MK_I(C__D,0,6));

    //--------------------------------------------------------------------------
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

}


#endif // BOOST_ICL_TEST_ICL_DISCRETE_INTERVAL_HPP_JOFA_100930
