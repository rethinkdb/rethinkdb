//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to show how to parse arbitrary key/value 
// pairs delimited by some separator into a std::vector. The difference to 
// the example 'key_value_sequence.cpp' is that we preserve the order of the
// elements in the parsed seqeunce as well as possibly existing duplicates.
//
// For a more elaborate explanation see here: http://spirit.sourceforge.net/home/?p=371

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include <map>

namespace client
{
    namespace qi = boost::spirit::qi;

    typedef std::vector<std::pair<std::string, std::string> > pairs_type;

    template <typename Iterator>
    struct key_value_sequence_ordered 
      : qi::grammar<Iterator, pairs_type()>
    {
        key_value_sequence_ordered()
          : key_value_sequence_ordered::base_type(query)
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

    std::string input("key2=value2;key1;key3=value3");
    std::string::iterator begin = input.begin();
    std::string::iterator end = input.end();

    client::key_value_sequence_ordered<std::string::iterator> p;
    client::pairs_type v;

    if (!qi::parse(begin, end, p, v))
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------------- \n";
    }
    else
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing succeeded, found entries:\n";
        client::pairs_type::iterator end = v.end();
        for (client::pairs_type::iterator it = v.begin(); it != end; ++it)
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

