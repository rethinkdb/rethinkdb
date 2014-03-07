/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct void_;
    namespace result_of
    {
        template <
            typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_
          , typename Extra = void_
        >
        struct make_set;
        template <>
        struct make_set<>
        {
            typedef set<> type;
        };
    }
    inline set<>
    make_set()
    {
        return set<>();
    }
    namespace result_of
    {
        template <typename T0>
        struct make_set<T0>
        {
            typedef set<typename detail::as_fusion_element<T0>::type> type;
        };
    }
    template <typename T0>
    inline set<typename detail::as_fusion_element<T0>::type>
    make_set(T0 const& _0)
    {
        return set<typename detail::as_fusion_element<T0>::type>(
            _0);
    }
    namespace result_of
    {
        template <typename T0 , typename T1>
        struct make_set<T0 , T1>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type> type;
        };
    }
    template <typename T0 , typename T1>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type>
    make_set(T0 const& _0 , T1 const& _1)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type>(
            _0 , _1);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2>
        struct make_set<T0 , T1 , T2>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type>(
            _0 , _1 , _2);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3>
        struct make_set<T0 , T1 , T2 , T3>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type>(
            _0 , _1 , _2 , _3);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4>
        struct make_set<T0 , T1 , T2 , T3 , T4>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type>(
            _0 , _1 , _2 , _3 , _4);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5>
        struct make_set<T0 , T1 , T2 , T3 , T4 , T5>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type>(
            _0 , _1 , _2 , _3 , _4 , _5);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6>
        struct make_set<T0 , T1 , T2 , T3 , T4 , T5 , T6>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type>(
            _0 , _1 , _2 , _3 , _4 , _5 , _6);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7>
        struct make_set<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type>(
            _0 , _1 , _2 , _3 , _4 , _5 , _6 , _7);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8>
        struct make_set<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type>(
            _0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8);
    }
    namespace result_of
    {
        template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
        struct make_set<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
        {
            typedef set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type , typename detail::as_fusion_element<T9>::type> type;
        };
    }
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    inline set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type , typename detail::as_fusion_element<T9>::type>
    make_set(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9)
    {
        return set<typename detail::as_fusion_element<T0>::type , typename detail::as_fusion_element<T1>::type , typename detail::as_fusion_element<T2>::type , typename detail::as_fusion_element<T3>::type , typename detail::as_fusion_element<T4>::type , typename detail::as_fusion_element<T5>::type , typename detail::as_fusion_element<T6>::type , typename detail::as_fusion_element<T7>::type , typename detail::as_fusion_element<T8>::type , typename detail::as_fusion_element<T9>::type>(
            _0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9);
    }
}}
