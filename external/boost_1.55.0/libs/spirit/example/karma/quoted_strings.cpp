//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate how to utilize alternatives 
//  and the built in matching capabilities of Karma generators to emit output 
//  in different formats based on the content of an attribute (not its type).

#include <boost/config/warning_disable.hpp>

#include <string>
#include <vector>

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>

namespace client
{
    namespace karma = boost::spirit::karma;
    namespace phx = boost::phoenix;

    template <typename OutputIterator>
    struct quoted_strings
      : karma::grammar<OutputIterator, std::vector<std::string>()>
    {
        quoted_strings()
          : quoted_strings::base_type(strings)
        {
            strings = (bareword | qstring) % ' ';
            bareword = karma::repeat(phx::size(karma::_val))
                          [ karma::alnum | karma::char_("-.,_$") ];
            qstring = '"' << karma::string << '"';
        }

        karma::rule<OutputIterator, std::vector<std::string>()> strings;
        karma::rule<OutputIterator, std::string()> bareword, qstring;
    };
}

int main()
{
    namespace karma = boost::spirit::karma;

    typedef std::back_insert_iterator<std::string> sink_type;

    std::string generated;
    sink_type sink(generated);

    std::vector<std::string> v;
    v.push_back("foo");
    v.push_back("bar baz");
    v.push_back("hello");

    client::quoted_strings<sink_type> g;
    if (!karma::generate(sink, g, v))
    {
        std::cout << "-------------------------\n";
        std::cout << "Generating failed\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Generated: " << generated << "\n";
        std::cout << "-------------------------\n";
    }
    return 0;
}

