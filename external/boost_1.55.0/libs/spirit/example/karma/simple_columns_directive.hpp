//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(COLUMNS_DEC_05_2009_0716PM)
#define COLUMNS_DEC_05_2009_0716PM

#include <boost/spirit/include/karma_generate.hpp>

///////////////////////////////////////////////////////////////////////////////
// definition the place holder 
namespace custom_generator 
{ 
    BOOST_SPIRIT_TERMINAL(columns);
} 

///////////////////////////////////////////////////////////////////////////////
// implementation the enabler
namespace boost { namespace spirit 
{ 
    // We want custom_generator::columns to be usable as a directive only, 
    // and only for generator expressions (karma::domain).
    template <>
    struct use_directive<karma::domain, custom_generator::tag::columns> 
      : mpl::true_ {}; 
}}

///////////////////////////////////////////////////////////////////////////////
// implementation of the generator
namespace custom_generator
{ 
    // special delimiter wrapping the original one while additionally emitting
    // the column delimiter after each 5th invocation
    template <typename Delimiter>
    struct columns_delimiter 
    {
        columns_delimiter(Delimiter const& delim)
          : delimiter(delim), count(0) {}

        // This function is called during the actual delimiter output 
        template <typename OutputIterator, typename Context
          , typename Delimiter_, typename Attribute>
        bool generate(OutputIterator& sink, Context&, Delimiter_ const&
          , Attribute const&) const
        {
            // first invoke the wrapped delimiter
            if (!karma::delimit_out(sink, delimiter))
                return false;

            // now we count the number of invocations and emit the column 
            // delimiter after each 5th column
            if ((++count % 5) == 0) 
                *sink++ = '\n';
            return true;
        }

        // Generate a final column delimiter if the last invocation didn't 
        // emit one
        template <typename OutputIterator>
        bool final_delimit_out(OutputIterator& sink) const
        {
            if (count % 5)
                *sink++ = '\n';
            return true;
        }

        Delimiter const& delimiter;   // wrapped delimiter
        mutable unsigned int count;   // invocation counter
    };

    // That's the actual columns generator
    template <typename Subject>
    struct simple_columns_generator
      : boost::spirit::karma::unary_generator<
            simple_columns_generator<Subject> >
    {
        // Define required output iterator properties
        typedef typename Subject::properties properties;

        // Define the attribute type exposed by this parser component
        template <typename Context, typename Iterator>
        struct attribute 
          : boost::spirit::traits::attribute_of<Subject, Context, Iterator> 
        {};

        simple_columns_generator(Subject const& s)
          : subject(s)
        {}

        // This function is called during the actual output generation process.
        // It dispatches to the embedded generator while supplying a new 
        // delimiter to use, wrapping the outer delimiter.
        template <typename OutputIterator, typename Context
          , typename Delimiter, typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& delimiter, Attribute const& attr) const
        {
            columns_delimiter<Delimiter> d(delimiter);
            return subject.generate(sink, ctx, d, attr) && d.final_delimit_out(sink);
        }

        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context& ctx) const
        {
            return boost::spirit::info("columns", subject.what(ctx));
        }

        Subject subject;
    };
}

///////////////////////////////////////////////////////////////////////////////
// instantiation of the generator
namespace boost { namespace spirit { namespace karma
{
    // This is the factory function object invoked in order to create 
    // an instance of our simple_columns_generator.
    template <typename Subject, typename Modifiers>
    struct make_directive<custom_generator::tag::columns, Subject, Modifiers>
    {
        typedef custom_generator::simple_columns_generator<Subject> result_type;

        result_type operator()(unused_type, Subject const& s, unused_type) const
        {
            return result_type(s);
        }
    };
}}}

#endif
