/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_ICL_DYNAMIC_INTERVAL_HPP_JOFA_100930
#define BOOST_ICL_TEST_ICL_DYNAMIC_INTERVAL_HPP_JOFA_100930

template <class IntervalT> 
void dynamic_interval_ctor_4_ordered_types()
{
    typedef typename domain_type_of<interval_traits<IntervalT> >::type T;

    BOOST_CHECK_EQUAL(IntervalT().lower(), icl::identity_element<T>::value());
    BOOST_CHECK_EQUAL(IntervalT().upper(), icl::identity_element<T>::value());
    BOOST_CHECK_EQUAL(icl::lower(IntervalT()), icl::identity_element<T>::value());
    BOOST_CHECK_EQUAL(icl::upper(IntervalT()), icl::identity_element<T>::value());

    IntervalT itv = IntervalT(icl::identity_element<T>::value(), icl::identity_element<T>::value());
    BOOST_CHECK_EQUAL(IntervalT(), IntervalT(icl::identity_element<T>::value(), icl::identity_element<T>::value()));
    BOOST_CHECK_EQUAL(IntervalT(), IntervalT(icl::identity_element<T>::value(), icl::identity_element<T>::value(), interval_bounds::right_open()));
}

template <class T> 
void dynamic_interval_bounds_4_bicremental_types()
{
    typedef typename icl::interval<T>::type IntervalT;

    // BOOST_CHECK_EQUAL( T(), icl::pred(icl::succ(T())));
    BOOST_CHECK_EQUAL( icl::identity_element<T>::value(), icl::pred(icl::succ(icl::identity_element<T>::value())) );
    BOOST_CHECK_EQUAL( icl::unit_element<T>::value(),     icl::succ(icl::identity_element<T>::value())        );
    BOOST_CHECK_EQUAL( length(IntervalT()), icl::identity_element<typename difference_type_of<T>::type>::value() );

    //LAW: I x: borders(x)==closed => contains(x, lower(x)) && contains(x, upper(x))
    check_border_containedness(I_I(0,0));
    check_border_containedness(I_I(2,5));

    check_border_containedness(I_D(0,1));
    check_border_containedness(I_D(2,5));

    check_border_containedness(C_I(0,1));
    check_border_containedness(C_I(2,5));

    check_border_containedness(C_I(0,2));
    check_border_containedness(C_I(2,5));

}

template <class T> 
void discrete_dynamic_interval_bounds_4_bicremental_types()
{
    typedef typename icl::interval<T>::type IntervalT;

    BOOST_CHECK( icl::bounds(I_I(2,4)) == interval_bounds::closed()     );
    BOOST_CHECK( icl::bounds(I_D(2,5)) == interval_bounds::right_open() );
    BOOST_CHECK( icl::bounds(C_I(1,4)) == interval_bounds::left_open()  );
    BOOST_CHECK( icl::bounds(C_D(1,5)) == interval_bounds::open()       );

}



#endif // BOOST_ICL_TEST_ICL_DYNAMIC_INTERVAL_HPP_JOFA_100930
