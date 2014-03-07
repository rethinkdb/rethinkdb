/*==============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2010-2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_GENERATOR_HPP)
#define BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_GENERATOR_HPP

#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/karma.hpp>

namespace boost {
namespace spirit {
namespace traits { 

template<>
struct transform_attribute<utree::nil_type, unused_type, karma::domain> {
  typedef unused_type type;

  static unused_type pre (utree::nil_type&) { return unused_type(); }
};

} // traits
} // spirit
} // boost

namespace sexpr
{

namespace karma = boost::spirit::karma;
namespace standard = boost::spirit::standard;

using boost::spirit::utree;
using boost::spirit::utf8_symbol_range_type;
using boost::spirit::utf8_string_range_type;
using boost::spirit::binary_range_type;

struct bool_output_policies : karma::bool_policies<>
{
    template <typename CharEncoding, typename Tag, typename Iterator>
    static bool generate_true(Iterator& sink, bool)
    {
        return string_inserter<CharEncoding, Tag>::call(sink, "#t");
    }

    template <typename CharEncoding, typename Tag, typename Iterator>
    static bool generate_false(Iterator& sink, bool)
    {
        return string_inserter<CharEncoding, Tag>::call(sink, "#f");
    }
};

template <typename Iterator>
struct generator : karma::grammar<Iterator, utree()>
{
    typedef boost::iterator_range<utree::const_iterator> utree_list;

    karma::rule<Iterator, utree()>
        start, ref_;

    karma::rule<Iterator, utree_list()>
        list;

    karma::rule<Iterator, utf8_symbol_range_type()>
        symbol;

    karma::rule<Iterator, utree::nil_type()>
        nil_;

    karma::rule<Iterator, utf8_string_range_type()>
        utf8;

    karma::rule<Iterator, binary_range_type()>
        binary;

    generator() : generator::base_type(start)
    {
        using standard::char_;
        using standard::string;
        using karma::bool_generator;
        using karma::uint_generator;
        using karma::double_;
        using karma::int_;
        using karma::lit;
        using karma::right_align;

        uint_generator<unsigned char, 16> hex2;
        bool_generator<bool, bool_output_policies> boolean;

        start = nil_
              | double_
              | int_
              | boolean
              | utf8
              | symbol
              | binary
              | list
              | ref_;
  
        ref_ = start;

        list = '(' << -(start % ' ') << ')';

        utf8 = '"' << *(&char_('"') << "\\\"" | char_) << '"';

        symbol = string;

        binary = '#' << *right_align(2, '0')[hex2] << '#';

        nil_ = karma::attr_cast(lit("nil"));

        start.name("sexpr");
        ref_.name("ref");
        list.name("list");
        utf8.name("string");
        symbol.name("symbol");
        binary.name("binary");
        nil_.name("nil");
    }
};

} // sexpr

#endif // BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_GENERATOR_HPP

