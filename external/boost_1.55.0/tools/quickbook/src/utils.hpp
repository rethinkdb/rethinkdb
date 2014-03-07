/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_UTILS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_UTILS_HPP

#include <string>
#include <ostream>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/utility/string_ref.hpp>

namespace quickbook { namespace detail {
    std::string encode_string(boost::string_ref);
    void print_char(char ch, std::ostream& out);
    void print_string(boost::string_ref str, std::ostream& out);
    char filter_identifier_char(char ch);

    template <typename Range>
    inline std::string
    make_identifier(Range const& range)
    {
        std::string out_name;

        boost::push_back(out_name,
            range | boost::adaptors::transformed(filter_identifier_char));

        return out_name;
    }

    std::string escape_uri(std::string uri);
    inline std::string escape_uri(boost::string_ref uri) {
        return escape_uri(std::string(uri.begin(), uri.end()));
    }

    inline std::string to_s(boost::string_ref x) {
        return std::string(x.begin(), x.end());
    }
}}

#endif // BOOST_SPIRIT_QUICKBOOK_UTILS_HPP

