/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef LIBS_ICL_TEST_TEST_SET_ICL_SET_HPP_JOFA_090119
#define LIBS_ICL_TEST_TEST_SET_ICL_SET_HPP_JOFA_090119

#include "portability.hpp"

//------------------------------------------------------------------------------
// Monoid EAN
//------------------------------------------------------------------------------
template <class T, ICL_IntervalSet_TEMPLATE(_T) IntervalSet>
void itl_set_check_monoid_plus_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::set<T> SetT;

    IntervalSetT itv_set_a, itv_set_b, itv_set_c;
    itv_set_a.add(I_D(3,6)).add(I_I(5,7));
    itv_set_b.add(C_D(1,3)).add(I_D(8,9));
    itv_set_c.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    SetT set_a, set_b, set_c;
    segmental::atomize(set_a, itv_set_a);
    segmental::atomize(set_b, itv_set_b);
    segmental::atomize(set_c, itv_set_c);

    T val1 = MK_v(7);
    T val2 = MK_v(5);

    CHECK_MONOID_INSTANCE_WRT(plus) (set_a, set_b, set_c, val1, val2);
    CHECK_MONOID_INSTANCE_WRT(pipe) (set_a, set_b, set_c, val1, val2);
}

template <class T, ICL_IntervalSet_TEMPLATE(_T) IntervalSet>
void itl_set_check_monoid_et_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::set<T> SetT;

    IntervalSetT itv_set_a, itv_set_b, itv_set_c;
    itv_set_a.add(I_D(3,6)).add(I_I(5,7));
    itv_set_b.add(C_D(1,3)).add(I_D(8,9));
    itv_set_c.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    SetT set_a, set_b, set_c;
    segmental::atomize(set_a, itv_set_a);
    segmental::atomize(set_b, itv_set_b);
    segmental::atomize(set_c, itv_set_c);

    T val1 = MK_v(7);
    T val2 = MK_v(5);

    CHECK_MONOID_INSTANCE_WRT(et)   (set_a, set_b, set_c, val1, val2);
    CHECK_MONOID_INSTANCE_WRT(caret)(set_a, set_b, set_c, val1, val2);
}

//------------------------------------------------------------------------------
// Abelian monoid EANC
//------------------------------------------------------------------------------

template <class T, ICL_IntervalSet_TEMPLATE(_T) IntervalSet>
void itl_set_check_abelian_monoid_plus_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::set<T> SetT;

    IntervalSetT itv_set_a, itv_set_b, itv_set_c;
    itv_set_a.add(I_D(3,6)).add(I_I(5,7));
    itv_set_b.add(C_D(1,3)).add(I_D(8,9));
    itv_set_c.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    SetT set_a, set_b, set_c;
    segmental::atomize(set_a, itv_set_a);
    segmental::atomize(set_b, itv_set_b);
    segmental::atomize(set_c, itv_set_c);

    T val1 = MK_v(7);
    T val2 = MK_v(5);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(plus) (set_a, set_b, set_c, val1, val2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(pipe) (set_a, set_b, set_c, val1, val2);
}

template <class T, ICL_IntervalSet_TEMPLATE(_T) IntervalSet>
void itl_set_check_abelian_monoid_et_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::set<T> SetT;

    IntervalSetT itv_set_a, itv_set_b, itv_set_c;
    itv_set_a.add(I_D(3,6)).add(I_I(5,7));
    itv_set_b.add(C_D(1,3)).add(I_D(8,9));
    itv_set_c.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    SetT set_a, set_b, set_c;
    segmental::atomize(set_a, itv_set_a);
    segmental::atomize(set_b, itv_set_b);
    segmental::atomize(set_c, itv_set_c);

    T val1 = MK_v(7);
    T val2 = MK_v(5);

    CHECK_ABELIAN_MONOID_INSTANCE_WRT(et)   (set_a, set_b, set_c, val1, val2);
    CHECK_ABELIAN_MONOID_INSTANCE_WRT(caret)(set_a, set_b, set_c, val1, val2);
}


//------------------------------------------------------------------------------
// Abelian partial invertive monoid 
//------------------------------------------------------------------------------
template <class T, ICL_IntervalSet_TEMPLATE(_T) IntervalSet>
void itl_set_check_partial_invertive_monoid_plus_4_bicremental_types()
{
    typedef IntervalSet<T> IntervalSetT;
    typedef typename IntervalSetT::interval_type IntervalT;
    typedef std::set<T> SetT;

    IntervalSetT itv_set_a, itv_set_b, itv_set_c;
    itv_set_a.add(I_D(3,6)).add(I_I(5,7));
    itv_set_b.add(C_D(1,3)).add(I_D(8,9));
    itv_set_c.add(I_D(0,9)).add(I_I(3,6)).add(I_D(5,7));

    SetT set_a, set_b, set_c;
    segmental::atomize(set_a, itv_set_a);
    segmental::atomize(set_b, itv_set_b);
    segmental::atomize(set_c, itv_set_c);

    T val1 = MK_v(7);
    T val2 = MK_v(5);

    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(plus) (set_a, set_b, set_c, val1, val2);
    CHECK_PARTIAL_INVERTIVE_MONOID_INSTANCE_WRT(pipe) (set_a, set_b, set_c, val1, val2);
}

#endif // LIBS_ICL_TEST_TEST_SET_ICL_SET_HPP_JOFA_090119

