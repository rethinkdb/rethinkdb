//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to show how any character sequence can be
//  printed while being properly quoted.

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>

namespace client
{
    namespace karma = boost::spirit::karma;

    template <typename OutputIterator>
    struct escaped_string
      : karma::grammar<OutputIterator, std::string(char const*)>
    {
        escaped_string()
          : escaped_string::base_type(esc_str)
        {
            esc_char.add('\a', "\\a")('\b', "\\b")('\f', "\\f")('\n', "\\n")
                        ('\r', "\\r")('\t', "\\t")('\v', "\\v")('\\', "\\\\")
                        ('\'', "\\\'")('\"', "\\\"")
                ;

            esc_str =   karma::lit(karma::_r1) 
                    << *(esc_char | karma::print | "\\x" << karma::hex) 
                    <<  karma::lit(karma::_r1)
                ;
        }

        karma::rule<OutputIterator, std::string(char const*)> esc_str;
        karma::symbols<char, char const*> esc_char;
    };
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace karma = boost::spirit::karma;

    typedef std::back_insert_iterator<std::string> sink_type;

    std::string generated;
    sink_type sink(generated);

    std::string str("string to escape: \n\r\t\"'\x19");
    char const* quote = "'''";

    client::escaped_string<sink_type> g;
    if (!karma::generate(sink, g(quote), str))
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
