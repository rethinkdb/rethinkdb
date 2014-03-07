//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ITER_POS_NOV_20_2009_1245PM)
#define ITER_POS_NOV_20_2009_1245PM

#include <boost/spirit/include/qi_parse.hpp>

///////////////////////////////////////////////////////////////////////////////
// definition the place holder 
namespace custom_parser 
{ 
    BOOST_SPIRIT_TERMINAL(iter_pos);
} 

///////////////////////////////////////////////////////////////////////////////
// implementation the enabler
namespace boost { namespace spirit 
{ 
    // We want custom_parser::iter_pos to be usable as a terminal only, 
    // and only for parser expressions (qi::domain).
    template <>
    struct use_terminal<qi::domain, custom_parser::tag::iter_pos> 
      : mpl::true_ 
    {}; 
}}

///////////////////////////////////////////////////////////////////////////////
// implementation of the parser
namespace custom_parser 
{ 
    struct iter_pos_parser 
      : boost::spirit::qi::primitive_parser<iter_pos_parser>
    {
        // Define the attribute type exposed by this parser component
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef Iterator type;
        };

        // This function is called during the actual parsing process
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context&, Skipper const& skipper, Attribute& attr) const
        {
            boost::spirit::qi::skip_over(first, last, skipper);
            boost::spirit::traits::assign_to(first, attr);
            return true;
        }

        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context&) const
        {
            return boost::spirit::info("iter_pos");
        }
    };
}

///////////////////////////////////////////////////////////////////////////////
// instantiation of the parser
namespace boost { namespace spirit { namespace qi
{
    // This is the factory function object invoked in order to create 
    // an instance of our iter_pos_parser.
    template <typename Modifiers>
    struct make_primitive<custom_parser::tag::iter_pos, Modifiers>
    {
        typedef custom_parser::iter_pos_parser result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };
}}}

#endif
