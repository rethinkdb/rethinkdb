/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_SKIPPER_HPP)
#define BOOST_SPIRIT_CONJURE_SKIPPER_HPP

#include <boost/spirit/include/qi.hpp>

namespace client { namespace parser
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////////
    //  The skipper grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct skipper : qi::grammar<Iterator>
    {
        skipper() : skipper::base_type(start)
        {
            qi::char_type char_;
            ascii::space_type space;

            start =
                    space                               // tab/space/cr/lf
                |   "/*" >> *(char_ - "*/") >> "*/"     // C-style comments
                ;
        }

        qi::rule<Iterator> start;
    };
}}

#endif


