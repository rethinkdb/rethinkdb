//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_TEST_MAR_23_2007_0721PM)
#define BOOST_SPIRIT_LEX_TEST_MAR_23_2007_0721PM

#include <boost/variant.hpp>
#include <boost/range/iterator_range.hpp>

namespace spirit_test
{
    ///////////////////////////////////////////////////////////////////////////
    struct display_type
    {
        template<typename T>
        void operator()(T const &) const
        {
            std::cout << typeid(T).name() << std::endl;
        }

        template<typename T>
        static void print() 
        {
            std::cout << typeid(T).name() << std::endl;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    display_type const display = {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    inline boost::iterator_range<Iterator> const& 
    get_iterpair(boost::iterator_range<Iterator> const& itp)
    {
        return itp;
    }

    template <typename Iterator, BOOST_VARIANT_ENUM_PARAMS(typename T)>
    inline boost::iterator_range<Iterator> const& 
    get_iterpair(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& v)
    {
        return boost::get<boost::iterator_range<Iterator> >(v);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Lexer, typename Char>
    inline bool 
    test(Lexer& lex, Char const* input, std::size_t token_id = 0, 
        Char const* state = NULL)
    {
        typedef typename Lexer::iterator_type iterator_type;
        typedef std::basic_string<Char> string_type;

        string_type str(input);
        typename string_type::iterator it = str.begin();

        iterator_type first = lex.begin(it, str.end());
        iterator_type last = lex.end();

        bool r = true;

        if (NULL != state) {
            std::size_t stateid = lex.map_state(state);
            r = r && (static_cast<unsigned>(~0) != stateid);
            first.set_state(stateid);
        }

        r = r && lex;
        r = r && first != last;

        if (token_id != 0)
            r = r && (*first).id() == token_id;
        else
            r = r && (*first).id() != 0;

        using namespace boost;

        typedef typename Lexer::iterator_type::base_iterator_type iterator;
        typedef iterator_range<iterator> iterpair_type;
        iterpair_type const& ip = get_iterpair<iterator>((*first).value());

        r = r && string_type(ip.begin(), ip.end()) == str;
        return r && first != last && ++first == last;
    }
}

#endif


