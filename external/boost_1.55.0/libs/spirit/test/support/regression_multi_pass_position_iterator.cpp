//  Copyright (c) 2010 Chris Hoeppler
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This code below failed to compile with MSVC starting with Boost V1.42

#include <vector>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/qi.hpp>

namespace char_enc = boost::spirit::ascii;
namespace qi = boost::spirit::qi;

int main()
{
    typedef std::vector<char> file_storage;
    typedef boost::spirit::classic::position_iterator<
        file_storage::const_iterator> iterator_type;

    qi::rule<iterator_type, std::string(), qi::blank_type> top =
        qi::lexeme[+char_enc::alpha];

    return 0;
}

