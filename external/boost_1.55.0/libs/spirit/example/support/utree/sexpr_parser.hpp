/*==============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2010-2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_PARSER_HPP)
#define BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_PARSER_HPP

#include "utf8_parser.hpp"
#include "error_handler.hpp"

namespace boost {
namespace spirit {
namespace traits { 

template<>
struct transform_attribute<utree::nil_type, unused_type, qi::domain> {
  typedef unused_type type;

  static unused_type pre (utree::nil_type&) { return unused_type(); }
  static void post (utree::nil_type&, unused_type) { }
  static void fail (utree::nil_type&) { }
};

} // traits
} // spirit
} // boost

namespace sexpr
{

namespace qi = boost::spirit::qi;
namespace px = boost::phoenix;
namespace standard = boost::spirit::standard;

using boost::spirit::utree;
using boost::spirit::utf8_symbol_type;
using boost::spirit::utf8_string_type;
using boost::spirit::binary_string_type;

struct bool_input_policies
{
    template <typename Iterator, typename Attribute>
    static bool
    parse_true(Iterator& first, Iterator const& last, Attribute& attr)
    {
        using boost::spirit::qi::detail::string_parse;
        using boost::spirit::qi::bool_policies;
        using boost::spirit::qi::unused;
        using boost::spirit::traits::assign_to;
        if (string_parse("#t", first, last, unused))
        {
            assign_to(true, attr);    // result is true
            return true;
        }
        return bool_policies<bool>::parse_true(first, last, attr);
    }

    template <typename Iterator, typename Attribute>
    static bool
    parse_false(Iterator& first, Iterator const& last, Attribute& attr)
    {
        using boost::spirit::qi::detail::string_parse;
        using boost::spirit::qi::bool_policies;
        using boost::spirit::qi::unused;
        using boost::spirit::traits::assign_to;
        if (string_parse("#f", first, last, unused))
        {
            assign_to(false, attr);   // result is false
            return true;
        }
        return bool_policies<bool>::parse_false(first, last, attr);
    }
};

struct save_line_pos
{
    template <typename, typename>
    struct result
    {
        typedef void type;
    };

    template <typename Range>
    void operator()(utree& ast, Range const& rng) const
    {
        using boost::spirit::get_line;
        std::size_t n = get_line(rng.begin());
        if (n != -1)
        {
            BOOST_ASSERT(n <= (std::numeric_limits<short>::max)());
            ast.tag(n);
        }
        else
            ast.tag(-1);
    }
};

template <typename Iterator, typename F>
struct tagger : qi::grammar<Iterator, void(utree&, char)>
{
    qi::rule<Iterator, void(utree&, char)>
        start;
  
    qi::rule<Iterator, void(utree&)>
        epsilon;

    px::function<F>
        f;

    tagger(F f_ = F()) : tagger::base_type(start), f(f_)
    {
        using qi::omit;
        using qi::raw;
        using qi::eps;
        using qi::lit;
        using qi::_1;
        using qi::_r1;
        using qi::_r2;

        start   = omit[raw[lit(_r2)] [f(_r1, _1)]];

        epsilon = omit[raw[eps]      [f(_r1, _1)]];
    }
};

template <typename Iterator>
struct whitespace : qi::grammar<Iterator> {
    qi::rule<Iterator>
        start;

    whitespace() : whitespace::base_type(start)
    {
        using standard::space;
        using standard::char_;
        using qi::eol;

        start = space | (';' >> *(char_ - eol) >> eol);
    }
};

} // sexpr

//[utree_sexpr_parser
namespace sexpr
{

template <typename Iterator, typename ErrorHandler = error_handler<Iterator> >
struct parser : qi::grammar<Iterator, utree(), whitespace<Iterator> >
{
    qi::rule<Iterator, utree(), whitespace<Iterator> >
        start, element, list;

    qi::rule<Iterator, utree()>
        atom;

    qi::rule<Iterator, int()>
        integer;

    qi::rule<Iterator, utf8_symbol_type()>
        symbol;
 
    qi::rule<Iterator, utree::nil_type()>
        nil_;

    qi::rule<Iterator, binary_string_type()>
        binary;

    utf8::parser<Iterator>
        string;

    px::function<ErrorHandler> const
        error;
  
    tagger<Iterator, save_line_pos>
        pos;

    parser(std::string const& source_file = "<string>"):
        parser::base_type(start), error(ErrorHandler(source_file))
    {
        using standard::char_;
        using qi::unused_type;
        using qi::lexeme;
        using qi::hex;
        using qi::oct;
        using qi::no_case;
        using qi::real_parser;
        using qi::strict_real_policies;
        using qi::uint_parser;
        using qi::bool_parser;
        using qi::on_error;
        using qi::fail;
        using qi::int_;
        using qi::lit;
        using qi::_val;
        using qi::_1;
        using qi::_2;
        using qi::_3;
        using qi::_4;

        real_parser<double, strict_real_policies<double> > strict_double;
        uint_parser<unsigned char, 16, 2, 2> hex2;
        bool_parser<bool, sexpr::bool_input_policies> boolean;
 
        start = element.alias();

        element = atom | list;

        list = pos(_val, '(') > *element > ')';

        atom = nil_ 
             | strict_double
             | integer
             | boolean
             | string
             | symbol
             | binary;

        nil_ = qi::attr_cast(lit("nil")); 

        integer = lexeme[ no_case["#x"] >  hex]
                | lexeme[ no_case["#o"] >> oct]
                | lexeme[-no_case["#d"] >> int_];

        std::string exclude = std::string(" ();\"\x01-\x1f\x7f") + '\0';
        symbol = lexeme[+(~char_(exclude))];

        binary = lexeme['#' > *hex2 > '#'];

        start.name("sexpr");
        element.name("element");
        list.name("list");
        atom.name("atom");
        nil_.name("nil");
        integer.name("integer");
        symbol.name("symbol");
        binary.name("binary");
 
        on_error<fail>(start, error(_1, _2, _3, _4));
    }
};

} // sexpr
//]

#endif // BOOST_SPIRIT_UTREE_EXAMPLE_SEXPR_PARSER_HPP

