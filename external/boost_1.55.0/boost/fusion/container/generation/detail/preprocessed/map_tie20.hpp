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
            typename K0 = void_ , typename K1 = void_ , typename K2 = void_ , typename K3 = void_ , typename K4 = void_ , typename K5 = void_ , typename K6 = void_ , typename K7 = void_ , typename K8 = void_ , typename K9 = void_ , typename K10 = void_ , typename K11 = void_ , typename K12 = void_ , typename K13 = void_ , typename K14 = void_ , typename K15 = void_ , typename K16 = void_ , typename K17 = void_ , typename K18 = void_ , typename K19 = void_
          , typename D0 = void_ , typename D1 = void_ , typename D2 = void_ , typename D3 = void_ , typename D4 = void_ , typename D5 = void_ , typename D6 = void_ , typename D7 = void_ , typename D8 = void_ , typename D9 = void_ , typename D10 = void_ , typename D11 = void_ , typename D12 = void_ , typename D13 = void_ , typename D14 = void_ , typename D15 = void_ , typename D16 = void_ , typename D17 = void_ , typename D18 = void_ , typename D19 = void_
          , typename Extra = void_
        >
        struct map_tie;
        template <>
        struct map_tie<>
        {
            typedef map<> type;
        };
    }
    inline map<>
    map_tie()
    {
        return map<>();
    }
    namespace result_of
    {
        template <
            typename K0
          , typename D0
        >
        struct map_tie<K0, D0>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> > type;
        };
    }
    template <
        typename K0
      , typename D0
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> >
    map_tie(D0 & _0)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> >(
            fusion::pair_tie<K0>(_0));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1
          , typename D0 , typename D1
        >
        struct map_tie<K0 , K1, D0 , D1>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> > type;
        };
    }
    template <
        typename K0 , typename K1
      , typename D0 , typename D1
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> >
    map_tie(D0 & _0 , D1 & _1)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2
          , typename D0 , typename D1 , typename D2
        >
        struct map_tie<K0 , K1 , K2, D0 , D1 , D2>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2
      , typename D0 , typename D1 , typename D2
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3
          , typename D0 , typename D1 , typename D2 , typename D3
        >
        struct map_tie<K0 , K1 , K2 , K3, D0 , D1 , D2 , D3>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3
      , typename D0 , typename D1 , typename D2 , typename D3
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4, D0 , D1 , D2 , D3 , D4>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5, D0 , D1 , D2 , D3 , D4 , D5>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6, D0 , D1 , D2 , D3 , D4 , D5 , D6>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14 , D15 & _15)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14) , fusion::pair_tie<K15>(_15));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14 , D15 & _15 , D16 & _16)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14) , fusion::pair_tie<K15>(_15) , fusion::pair_tie<K16>(_16));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14 , D15 & _15 , D16 & _16 , D17 & _17)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14) , fusion::pair_tie<K15>(_15) , fusion::pair_tie<K16>(_16) , fusion::pair_tie<K17>(_17));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17 , K18, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17 , D18>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14 , D15 & _15 , D16 & _16 , D17 & _17 , D18 & _18)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14) , fusion::pair_tie<K15>(_15) , fusion::pair_tie<K16>(_16) , fusion::pair_tie<K17>(_17) , fusion::pair_tie<K18>(_18));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18 , typename K19
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18 , typename D19
        >
        struct map_tie<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17 , K18 , K19, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17 , D18 , D19>
        {
            typedef map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> , fusion::pair< K19 , typename add_reference<D19>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18 , typename K19
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18 , typename D19
    >
    inline map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> , fusion::pair< K19 , typename add_reference<D19>::type> >
    map_tie(D0 & _0 , D1 & _1 , D2 & _2 , D3 & _3 , D4 & _4 , D5 & _5 , D6 & _6 , D7 & _7 , D8 & _8 , D9 & _9 , D10 & _10 , D11 & _11 , D12 & _12 , D13 & _13 , D14 & _14 , D15 & _15 , D16 & _16 , D17 & _17 , D18 & _18 , D19 & _19)
    {
        return map<fusion::pair< K0 , typename add_reference<D0>::type> , fusion::pair< K1 , typename add_reference<D1>::type> , fusion::pair< K2 , typename add_reference<D2>::type> , fusion::pair< K3 , typename add_reference<D3>::type> , fusion::pair< K4 , typename add_reference<D4>::type> , fusion::pair< K5 , typename add_reference<D5>::type> , fusion::pair< K6 , typename add_reference<D6>::type> , fusion::pair< K7 , typename add_reference<D7>::type> , fusion::pair< K8 , typename add_reference<D8>::type> , fusion::pair< K9 , typename add_reference<D9>::type> , fusion::pair< K10 , typename add_reference<D10>::type> , fusion::pair< K11 , typename add_reference<D11>::type> , fusion::pair< K12 , typename add_reference<D12>::type> , fusion::pair< K13 , typename add_reference<D13>::type> , fusion::pair< K14 , typename add_reference<D14>::type> , fusion::pair< K15 , typename add_reference<D15>::type> , fusion::pair< K16 , typename add_reference<D16>::type> , fusion::pair< K17 , typename add_reference<D17>::type> , fusion::pair< K18 , typename add_reference<D18>::type> , fusion::pair< K19 , typename add_reference<D19>::type> >(
            fusion::pair_tie<K0>(_0) , fusion::pair_tie<K1>(_1) , fusion::pair_tie<K2>(_2) , fusion::pair_tie<K3>(_3) , fusion::pair_tie<K4>(_4) , fusion::pair_tie<K5>(_5) , fusion::pair_tie<K6>(_6) , fusion::pair_tie<K7>(_7) , fusion::pair_tie<K8>(_8) , fusion::pair_tie<K9>(_9) , fusion::pair_tie<K10>(_10) , fusion::pair_tie<K11>(_11) , fusion::pair_tie<K12>(_12) , fusion::pair_tie<K13>(_13) , fusion::pair_tie<K14>(_14) , fusion::pair_tie<K15>(_15) , fusion::pair_tie<K16>(_16) , fusion::pair_tie<K17>(_17) , fusion::pair_tie<K18>(_18) , fusion::pair_tie<K19>(_19));
    }
}}
