//  Copyright (c) 2010 Jeroen Habraken
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/spirit/include/qi.hpp>

#include <iostream>
#include <ostream>
#include <string>

namespace client
{
    namespace qi = boost::spirit::qi;

    template <typename InputIterator>
    struct unescaped_string
        : qi::grammar<InputIterator, std::string(char const*)>
    {
        unescaped_string()
            : unescaped_string::base_type(unesc_str)
        {
            unesc_char.add("\\a", '\a')("\\b", '\b')("\\f", '\f')("\\n", '\n')
                          ("\\r", '\r')("\\t", '\t')("\\v", '\v')("\\\\", '\\')
                          ("\\\'", '\'')("\\\"", '\"')
                ;

            unesc_str = qi::lit(qi::_r1)
                    >> *(unesc_char | qi::alnum | "\\x" >> qi::hex)
                    >>  qi::lit(qi::_r1)
                ;
        }

        qi::rule<InputIterator, std::string(char const*)> unesc_str;
        qi::symbols<char const, char const> unesc_char;
    };

}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace qi = boost::spirit::qi;
    
    typedef std::string::const_iterator iterator_type;

    std::string parsed;

    std::string str("'''string\\x20to\\x20unescape\\x3a\\x20\\n\\r\\t\\\"\\'\\x41'''");
    char const* quote = "'''";

    iterator_type iter = str.begin();
    iterator_type end = str.end();

    client::unescaped_string<iterator_type> p;
    if (!qi::parse(iter, end, p(quote), parsed))
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsed: " << parsed << "\n";
        std::cout << "-------------------------\n";
    }
 
    return 0;
}
