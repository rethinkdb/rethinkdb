/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion {
    struct deque_tag;
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct deque
        :
        detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type,
        sequence_base<deque<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9> >
    {
        typedef deque_tag fusion_tag;
        typedef bidirectional_traversal_tag category;
        typedef typename detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type base;
        typedef typename detail::deque_initial_size<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type size;
        typedef mpl::int_<size::value> next_up;
        typedef mpl::int_<
            mpl::if_<mpl::equal_to<size, mpl::int_<0> >, mpl::int_<0>, mpl::int_<-1> >::type::value> next_down;
        typedef mpl::false_ is_view;
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1)
    : base(detail::deque_keyed_values<T0 , T1>::call(t0 , t1))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2)
    : base(detail::deque_keyed_values<T0 , T1 , T2>::call(t0 , t1 , t2))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3>::call(t0 , t1 , t2 , t3))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4>::call(t0 , t1 , t2 , t3 , t4))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5>::call(t0 , t1 , t2 , t3 , t4 , t5))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6>::call(t0 , t1 , t2 , t3 , t4 , t5 , t6))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7>::call(t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8>::call(t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8))
{}
deque(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9)
    : base(detail::deque_keyed_values<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::call(t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9))
{}
        deque()
            {}
        explicit deque(typename add_reference<typename add_const<T0>::type>::type t0)
            : base(t0, detail::nil_keyed_element())
            {}
        template<typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
            deque(deque<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& seq)
            : base(seq)
            {}
        template<typename Sequence>
            deque(Sequence const& seq, typename disable_if<is_convertible<Sequence, T0> >::type* = 0)
            : base(base::from_iterator(fusion::begin(seq)))
            {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
        deque&
        operator=(deque<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9> const& rhs)
        {
            base::operator=(rhs);
            return *this;
        }
        template <typename T>
        deque&
        operator=(T const& rhs)
        {
            base::operator=(rhs);
            return *this;
        }
    };
}}
