/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct void_;
    struct fusion_sequence_tag;
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29>
    struct map : sequence_base<map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29> >
    {
        struct category : random_access_traversal_tag, associative_tag {};
        typedef map_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef vector<
            T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29>
        storage_type;
        typedef typename storage_type::size size;
        map()
            : data() {}
        template <typename Sequence>
        map(Sequence const& rhs)
            : data(rhs) {}
    explicit
    map(T0 const& _0)
        : data(_0) {}
    map(T0 const& _0 , T1 const& _1)
        : data(_0 , _1) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2)
        : data(_0 , _1 , _2) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3)
        : data(_0 , _1 , _2 , _3) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4)
        : data(_0 , _1 , _2 , _3 , _4) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5)
        : data(_0 , _1 , _2 , _3 , _4 , _5) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24 , T25 const& _25)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24 , _25) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24 , T25 const& _25 , T26 const& _26)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24 , _25 , _26) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24 , T25 const& _25 , T26 const& _26 , T27 const& _27)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24 , _25 , _26 , _27) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24 , T25 const& _25 , T26 const& _26 , T27 const& _27 , T28 const& _28)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24 , _25 , _26 , _27 , _28) {}
    map(T0 const& _0 , T1 const& _1 , T2 const& _2 , T3 const& _3 , T4 const& _4 , T5 const& _5 , T6 const& _6 , T7 const& _7 , T8 const& _8 , T9 const& _9 , T10 const& _10 , T11 const& _11 , T12 const& _12 , T13 const& _13 , T14 const& _14 , T15 const& _15 , T16 const& _16 , T17 const& _17 , T18 const& _18 , T19 const& _19 , T20 const& _20 , T21 const& _21 , T22 const& _22 , T23 const& _23 , T24 const& _24 , T25 const& _25 , T26 const& _26 , T27 const& _27 , T28 const& _28 , T29 const& _29)
        : data(_0 , _1 , _2 , _3 , _4 , _5 , _6 , _7 , _8 , _9 , _10 , _11 , _12 , _13 , _14 , _15 , _16 , _17 , _18 , _19 , _20 , _21 , _22 , _23 , _24 , _25 , _26 , _27 , _28 , _29) {}
        template <typename T>
        map&
        operator=(T const& rhs)
        {
            data = rhs;
            return *this;
        }
        storage_type& get_data() { return data; }
        storage_type const& get_data() const { return data; }
    private:
        storage_type data;
    };
}}
