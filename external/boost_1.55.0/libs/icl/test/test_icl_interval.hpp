/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TEST_ICL_INTERVAL_HPP_JOFA_100930
#define BOOST_ICL_TEST_ICL_INTERVAL_HPP_JOFA_100930

// NOTE: ordered_types is the largest class of types that is admissable as 
//                     domain parameters for intervals and interval containers.
//   bicremental_types is a smaller class of types used for testing instead of
//                     ordered types, because they can be generated in a simple
//                     way using the functions of test_value_maker.hpp.

template <class IntervalT> 
void interval_ctor_4_ordered_types()
{
    typedef typename domain_type_of<interval_traits<IntervalT> >::type T;
    typedef typename icl::size_type_of<T>::type SizeT;
    T t_0     = icl::identity_element<T>::value();
    T t_1     = icl::unit_element<T>::value();
    SizeT s_0 = icl::identity_element<SizeT>::value();
    SizeT s_1 = icl::unit_element<SizeT>::value();

    // Default ctor and emptieness
    BOOST_CHECK_EQUAL( icl::is_empty(IntervalT()), true );
    BOOST_CHECK_EQUAL( icl::cardinality(IntervalT()), s_0 );
    BOOST_CHECK_EQUAL(        icl::size(IntervalT()), s_0 );

    BOOST_CHECK_EQUAL( IntervalT(), IntervalT() );
    BOOST_CHECK_EQUAL( IntervalT(), IntervalT(IntervalT().lower(), IntervalT().upper()) );
    BOOST_CHECK_EQUAL( IntervalT(), IntervalT(icl::lower(IntervalT()), icl::upper(IntervalT())) );

    BOOST_CHECK_EQUAL(icl::cardinality(IntervalT(t_0, t_1)) >= s_1,  true);
    BOOST_CHECK_EQUAL((    icl::contains(IntervalT(t_0, t_1), t_0)
                       ||  icl::contains(IntervalT(t_0, t_1), t_1)), true);

    BOOST_CHECK_EQUAL(IntervalT(t_0, t_1).lower(),  t_0);
    BOOST_CHECK_EQUAL(IntervalT(t_0, t_1).upper(),  t_1);
    BOOST_CHECK_EQUAL(lower(icl::construct<IntervalT>(t_0, t_1)),  t_0);
    BOOST_CHECK_EQUAL(upper(icl::construct<IntervalT>(t_0, t_1)),  t_1);
}


template <class IntervalT> 
void singelizable_interval_4_ordered_types()
{
    // Singleton ctor and singleness 
    // LAW:  !is_asymmetric_continuous(IntervalT) => size(singleton(x))==1
    // LAW: This law applies to all discrete and to dynamic continuous intervals
    // LAW: No singletons can be constructed for static continuous right_open and left_open intervals
    typedef typename domain_type_of<interval_traits<IntervalT> >::type T;
    typedef typename icl::size_type_of<T>::type SizeT;
    T t_0     = icl::identity_element<T>::value();
    T t_1     = icl::unit_element<T>::value();
    SizeT s_1 = icl::unit_element<SizeT>::value();

    BOOST_CHECK( is_singelizable<IntervalT>::value ); 

    BOOST_CHECK_EQUAL( icl::cardinality(icl::singleton<IntervalT>(t_0)), s_1 ); 
    BOOST_CHECK_EQUAL(        icl::size(icl::singleton<IntervalT>(t_0)), s_1 ); 
    BOOST_CHECK_EQUAL( icl::cardinality(icl::singleton<IntervalT>(t_1)), s_1 ); 
    BOOST_CHECK_EQUAL(        icl::size(icl::singleton<IntervalT>(t_1)), s_1 ); 

    BOOST_CHECK_EQUAL( icl::contains(icl::singleton<IntervalT>(t_0), t_0), true ); 
    BOOST_CHECK_EQUAL( icl::contains(icl::singleton<IntervalT>(t_1), t_1), true ); 
}

template <class IntervalT> 
void singelizable_interval_4_bicremental_types()
{
    typedef typename domain_type_of<interval_traits<IntervalT> >::type T;
    typedef typename icl::size_type_of<T>::type SizeT;
    //T t_0     = icl::identity_element<T>::value();
    SizeT s_1 = icl::unit_element<SizeT>::value();

    BOOST_CHECK( is_singelizable<IntervalT>::value ); 

    BOOST_CHECK_EQUAL( icl::cardinality(IntervalT(MK_v(3))), s_1 ); 
    BOOST_CHECK_EQUAL(        icl::size(IntervalT(MK_v(4))), s_1 ); 
    BOOST_CHECK_EQUAL( icl::singleton<IntervalT>(MK_v(2)), icl::singleton<IntervalT>(MK_v(2)) );
    BOOST_CHECK_EQUAL( icl::contains(IntervalT(MK_v(1)), MK_v(1)), true ); 
}

template <class IntervalT> 
void coverable_asymmetric_interval_4_bicremental_types()
{
    typedef typename domain_type_of<interval_traits<IntervalT> >::type T;
    typedef typename icl::size_type_of<T>::type SizeT;
    typedef typename icl::difference_type_of<T>::type DiffT;
    //T t_0     = icl::identity_element<T>::value();
    SizeT s_1 = icl::unit_element<SizeT>::value();
    DiffT d_1 = icl::unit_element<DiffT>::value();

    //JODO BOOST_CHECK( is_incremental_coverable<IntervalT>::value ); 
    BOOST_CHECK( has_difference<T>::value ); 

    BOOST_CHECK_EQUAL( icl::contains(icl::detail::unit_trail<IntervalT>(MK_v(4)), MK_v(4)), true ); 
    BOOST_CHECK_EQUAL( icl::length  (icl::detail::unit_trail<IntervalT>(MK_v(3))), d_1 ); 
    BOOST_CHECK      ( icl::touches (icl::detail::unit_trail<IntervalT>(MK_v(2)), icl::detail::unit_trail<IntervalT>(MK_v(3))) );
}



#endif // BOOST_ICL_TEST_ICL_INTERVAL_HPP_JOFA_100930
