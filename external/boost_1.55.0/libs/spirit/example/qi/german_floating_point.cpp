//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Michael Caisse
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

template <typename T>
struct german_real_policies : qi::real_policies<T>
{
    template <typename Iterator>
    static bool parse_dot(Iterator& first, Iterator const& last)
    {
        if (first == last || *first != ',')
            return false;
        ++first;
        return true;
    }
};

qi::real_parser<double, german_real_policies<double> > const german_double;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::string input("123,456");
    std::string::iterator begin = input.begin();
    std::string::iterator end = input.end();

    double value = 0;
    if (!qi::parse(begin, end, german_double, value))
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------------- \n";
    }
    else
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing succeeded, got: " << value << "\n";
        std::cout << "---------------------------------\n";
    }
    return 0;
}

