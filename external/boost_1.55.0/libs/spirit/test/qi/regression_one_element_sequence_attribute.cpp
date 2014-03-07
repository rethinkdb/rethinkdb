//  Copyright (c) 2010 Josh Wilson
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant.hpp>
#include <string>

namespace qi = boost::spirit::qi;

///////////////////////////////////////////////////////////////////////////////
struct Number { float base; };

BOOST_FUSION_ADAPT_STRUCT( Number, (float, base) )

void instantiate1() 
{
      qi::symbols<char, Number> sym;
      qi::rule<std::string::const_iterator, Number()> rule;
      rule %= sym;  // Caused compiler error after getting r61322
}

///////////////////////////////////////////////////////////////////////////////
typedef boost::variant<int, float> internal_type;

struct Number2 { internal_type base; };

BOOST_FUSION_ADAPT_STRUCT( Number2, (internal_type, base) )

void instantiate2() 
{
      qi::symbols<char, Number2> sym;
      qi::rule<std::string::const_iterator, Number2()> rule;
      rule %= sym;  // Caused compiler error after getting r61322
}

