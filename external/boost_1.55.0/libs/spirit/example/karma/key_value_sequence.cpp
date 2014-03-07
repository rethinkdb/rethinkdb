//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show how we can generate output from 
//  a container holding key/value pairs.
//
//  For more information see here: http://spirit.sourceforge.net/home/?p=400

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_stream.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <iostream>
#include <map>
#include <algorithm>
#include <cstdlib> 

namespace client
{
    namespace karma = boost::spirit::karma;

    typedef std::pair<std::string, boost::optional<std::string> > pair_type;

    template <typename OutputIterator>
    struct keys_and_values
      : karma::grammar<OutputIterator, std::vector<pair_type>()>
    {
        keys_and_values()
          : keys_and_values::base_type(query)
        {
            query =  pair << *('&' << pair);
            pair  =  karma::string << -('=' << karma::string);
        }

        karma::rule<OutputIterator, std::vector<pair_type>()> query;
        karma::rule<OutputIterator, pair_type()> pair;
    };
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    namespace karma = boost::spirit::karma;

    typedef std::vector<client::pair_type>::value_type value_type;
    typedef std::back_insert_iterator<std::string> sink_type;

    std::vector<client::pair_type> v;
    v.push_back(value_type("key1", boost::optional<std::string>("value1")));
    v.push_back(value_type("key2", boost::optional<std::string>()));
    v.push_back(value_type("key3", boost::optional<std::string>("")));

    std::string generated;
    sink_type sink(generated);
    client::keys_and_values<sink_type> g;
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

