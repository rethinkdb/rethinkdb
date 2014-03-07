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
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct map : sequence_base<map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9> >
    {
        struct category : random_access_traversal_tag, associative_tag {};
        typedef map_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef vector<
            T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
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
