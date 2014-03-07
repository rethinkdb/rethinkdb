//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to demonstrate how custom, user defined types
// can be easily integrated with the lexer as token value types. Moreover, the
// custom token values are properly exposed to the parser as well, allowing to 
// retrieve the custom values using the built in parser attribute propagation
// rules.

#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/qi.hpp>

namespace lex = boost::spirit::lex;
namespace qi = boost::spirit::qi;
namespace mpl = boost::mpl;

///////////////////////////////////////////////////////////////////////////////
// This is just a simple custom rational data structure holding two ints to be
// interpreted as a rational number
struct rational
{
    rational(int n = 0, int d = 0)
      : nominator_(n), denominator_(d)
    {}

    int nominator_;
    int denominator_;
};

///////////////////////////////////////////////////////////////////////////////
// A rational is represented as "{n,d}", where 'n' and 'd' are the nominator
// and denominator of the number. We use Spirit.Qi to do the low level parsing
// of the input sequence as matched by the lexer. Certainly, any other 
// conversion could be used instead.
//
// The lexer uses the template assign_to_attribute_from_iterators<> to convert
// the matched input sequence (pair of iterators) to the token value type as
// specified while defining the lex::token_def<>. 
//
// Our specialization of assign_to_attribute_from_iterators<> for the rational
// data type defined above has to be placed into the 
// namespace boost::spirit::traits, otherwise it won't be found by the library.
namespace boost { namespace spirit { namespace traits
{
    template <typename Iterator>
    struct assign_to_attribute_from_iterators<rational, Iterator>
    {
        static void 
        call(Iterator const& first, Iterator const& last, rational& attr)
        {
            int x, y;
            Iterator b = first;
            qi::parse(b, last, 
                '{' >> qi::int_ >> ',' >> qi::int_ >> '}', x, y);
            attr = rational(x, y);
        }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
// a lexer recognizing a single token type: rational
template <typename Lexer>
struct lex_rational : lex::lexer<Lexer>
{
    lex_rational()
    {
        this->self.add_pattern("INT", "[1-9][0-9]*");

        rt = "\\{{INT},{INT}\\}";
        this->self.add(rt);
    }
    lex::token_def<rational> rt;
};


int main()
{
    // the token type needs to know the iterator type of the underlying
    // input and the set of used token value types
    typedef lex::lexertl::token<std::string::iterator,
        mpl::vector<rational> > token_type;

    // use actor_lexer<> here if your token definitions have semantic
    // actions
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // this is the iterator exposed by the lexer, we use this for parsing
    typedef lexer_type::iterator_type iterator_type;

    // create a lexer instance
    std::string input("{3,4}");
    std::string::iterator s = input.begin();

    lex_rational<lexer_type> lex;
    iterator_type b = lex.begin(s, input.end());

    // use the embedded token_def as a parser, it exposes its token value type
    // as its parser attribute type
    rational r;
    if (!qi::parse(b, lex.end(), lex.rt, r))
    {
        std::cerr << "Parsing failed!" << std::endl;
        return -1;
    }

    std::cout << "Parsing succeeded: {" 
              << r.nominator_ << ", " << r.denominator_ << "}" << std::endl;
    return 0;
}

