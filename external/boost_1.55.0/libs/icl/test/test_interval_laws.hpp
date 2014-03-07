/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+
Function templates to call functions in object oriented or namespace glabal 
versions.
+-----------------------------------------------------------------------------*/
#ifndef BOOST_LIBS_ICL_TEST_INTERVAL_LAWS_HPP_JOFA_101011
#define BOOST_LIBS_ICL_TEST_INTERVAL_LAWS_HPP_JOFA_101011

#include <boost/icl/detail/notate.hpp>
#include <boost/icl/detail/design_config.hpp>
#include <boost/icl/type_traits/interval_type_default.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/type_traits/is_interval.hpp>
#include <boost/icl/concept/interval.hpp>

namespace boost{ namespace icl
{

template<class Type>
typename enable_if<is_interval<Type>, void>::type
check_border_containedness(const Type& itv)
{
    typedef typename interval_traits<Type>::domain_type domain_type;
    domain_type lo = icl::lower(itv);
    domain_type up = icl::upper(itv);

    //LAW: The empty set is contained in every set 
    BOOST_CHECK_EQUAL(icl::contains(itv, icl::identity_element<Type>::value()), true);
    //LAW: Reflexivity: Every interval contains itself
    BOOST_CHECK_EQUAL(icl::contains(itv, itv), true);

    if(icl::bounds(itv) == interval_bounds::closed())
    {
        BOOST_CHECK_EQUAL(icl::contains(itv, lo), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, up), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::    closed(lo,up)), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::right_open(lo,up)), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>:: left_open(lo,up)), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::      open(lo,up)), true);
    }
    else if(icl::bounds(itv) == interval_bounds::right_open())
    {
        BOOST_CHECK_EQUAL(icl::contains(itv, lo), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, up), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::    closed(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::right_open(lo,up)), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>:: left_open(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::      open(lo,up)), true);
    }
    else if(icl::bounds(itv) == interval_bounds::left_open())
    {
        BOOST_CHECK_EQUAL(icl::contains(itv, lo), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, up), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::    closed(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::right_open(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>:: left_open(lo,up)), true);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::      open(lo,up)), true);
    }
    else if(icl::bounds(itv) == interval_bounds::open())
    {
        BOOST_CHECK_EQUAL(icl::contains(itv, lo), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, up), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::    closed(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::right_open(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>:: left_open(lo,up)), false);
        BOOST_CHECK_EQUAL(icl::contains(itv, icl::interval<domain_type>::      open(lo,up)), true);
    }
    else
    {
        bool interval_borders_are_open_v_left_open_v_right_open_v_closed = true;
        BOOST_CHECK_EQUAL(interval_borders_are_open_v_left_open_v_right_open_v_closed, false);
    }
}

}} // namespace boost icl

#endif // BOOST_ICL_TEST_INTERVAL_LAWS_HPP_JOFA_100908

