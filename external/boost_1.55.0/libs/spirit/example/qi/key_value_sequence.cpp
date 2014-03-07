//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to show how to parse arbitrary key/value 
// pairs delimited by some separator into a std::map. Parsing the URL query 
// format is the example we use to demonstrate how this can be done 
// (i.e. things like: key1=value1;key2=value2;...;keyN=valueN).
//
// For a more elaborate explanation see here: http://spirit.sourceforge.net/home/?p=371

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include <map>

namespace client
{
    namespace qi = boost::spirit::qi;

    typedef std::map<std::string, std::string> pairs_type;

    template <typename Iterator>
    struct key_value_sequence 
      : qi::grammar<Iterator, pairs_type()>
    {
        key_value_sequence()
          : key_value_sequence::base_type(query)
        {
            query =  pair >> *((qi::lit(';') | '&') >> pair);
            pair  =  key >> -('=' >> value);
            key   =  qi::char_("a-zA-Z_") >> *qi::char_("a-zA-Z_0-9");
            value = +qi::char_("a-zA-Z_0-9");
        }

        qi::rule<Iterator, pairs_type()> query;
        qi::rule<Iterator, std::pair<std::string, std::string>()> pair;
        qi::rule<Iterator, std::string()> key, value;
    };
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace qi = boost::spirit::qi;

    std::string input("key1=value1;key2;key3=value3");
    std::string::iterator begin = input.begin();
    std::string::iterator end = input.end();

    client::key_value_sequence<std::string::iterator> p;
    client::pairs_type m;

    if (!qi::parse(begin, end, p, m))
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------------- \n";
    }
    else
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing succeeded, found entries:\n";
        client::pairs_type::iterator end = m.end();
        for (client::pairs_type::iterator it = m.begin(); it != end; ++it)
        {
            std::cout << (*it).first;
            if (!(*it).second.empty())
                std::cout << "=" << (*it).second;
            std::cout << std::endl;
        }
        std::cout << "---------------------------------\n";
    }
    return 0;
}

