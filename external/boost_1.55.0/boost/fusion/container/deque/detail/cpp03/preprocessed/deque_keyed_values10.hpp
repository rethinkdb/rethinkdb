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
    template<typename N, typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_>
    struct deque_keyed_values_impl;
    template<typename N>
    struct deque_keyed_values_impl<N, void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
    {
        typedef nil_keyed_element type;
        static type call()
        {
            return type();
        }
    };
    template<typename N, typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct deque_keyed_values_impl
    {
        typedef mpl::int_<mpl::plus<N, mpl::int_<1> >::value> next_index;
        typedef typename deque_keyed_values_impl<
            next_index,
            T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type tail;
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
    };
    template<typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_>
    struct deque_keyed_values
        : deque_keyed_values_impl<mpl::int_<0>, T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
    {};
}}}
