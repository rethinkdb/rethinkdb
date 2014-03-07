/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion { namespace detail
{
    template<typename Key, typename Value, typename Rest>
    struct keyed_element;
    struct nil_keyed_element;
    template<typename N, typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_ , typename T10 = void_ , typename T11 = void_ , typename T12 = void_ , typename T13 = void_ , typename T14 = void_ , typename T15 = void_ , typename T16 = void_ , typename T17 = void_ , typename T18 = void_ , typename T19 = void_ , typename T20 = void_ , typename T21 = void_ , typename T22 = void_ , typename T23 = void_ , typename T24 = void_ , typename T25 = void_ , typename T26 = void_ , typename T27 = void_ , typename T28 = void_ , typename T29 = void_>
    struct deque_keyed_values_impl;
    template<typename N>
    struct deque_keyed_values_impl<N, void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
    {
        typedef nil_keyed_element type;
        static type call()
        {
            return type();
        }
    };
    template<typename N, typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29>
    struct deque_keyed_values_impl
    {
        typedef mpl::int_<mpl::plus<N, mpl::int_<1> >::value> next_index;
        typedef typename deque_keyed_values_impl<
            next_index,
            T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29>::type tail;
        typedef keyed_element<N, T0, tail> type;
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        >::call());
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1
                        >::call(t1));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2
                        >::call(t1 , t2));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3
                        >::call(t1 , t2 , t3));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4
                        >::call(t1 , t2 , t3 , t4));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5
                        >::call(t1 , t2 , t3 , t4 , t5));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6
                        >::call(t1 , t2 , t3 , t4 , t5 , t6));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28));
        }
        static type call(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29
                        >::call(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29));
        }
    };
    template<typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_ , typename T10 = void_ , typename T11 = void_ , typename T12 = void_ , typename T13 = void_ , typename T14 = void_ , typename T15 = void_ , typename T16 = void_ , typename T17 = void_ , typename T18 = void_ , typename T19 = void_ , typename T20 = void_ , typename T21 = void_ , typename T22 = void_ , typename T23 = void_ , typename T24 = void_ , typename T25 = void_ , typename T26 = void_ , typename T27 = void_ , typename T28 = void_ , typename T29 = void_>
    struct deque_keyed_values
        : deque_keyed_values_impl<mpl::int_<0>, T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29>
    {};
}}}
