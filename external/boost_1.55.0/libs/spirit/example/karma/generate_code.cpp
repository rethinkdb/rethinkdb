//   Copyright (c) 2001-2010 Hartmut Kaiser
// 
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
//
//  Several small snippets generating different C++ code constructs
//
//  [ HK October 08, 2009 ]  Spirit V2.2
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <iostream>
#include <string>
#include <complex>

namespace client
{
    namespace karma = boost::spirit::karma;
    namespace phoenix = boost::phoenix;

    // create for instance: int name[5] = { 1, 2, 3, 4, 5 };
    template <typename Iterator>
    struct int_array : karma::grammar<Iterator, std::vector<int>()>
    {
        int_array(char const* name) : int_array::base_type(start)
        {
            using karma::int_;
            using karma::uint_;
            using karma::eol;
            using karma::lit;
            using karma::_val;
            using karma::_r1;

            start = array_def(phoenix::size(_val)) << " = " << initializer 
                                                   << ';' << eol;
            array_def = "int " << lit(name) << "[" << uint_(_r1) << "]";
            initializer = "{ " << -(int_ % ", ") << " }";
        }

        karma::rule<Iterator, void(unsigned)> array_def;
        karma::rule<Iterator, std::vector<int>()> initializer;
        karma::rule<Iterator, std::vector<int>()> start;
    };

    typedef std::back_insert_iterator<std::string> iterator_type;
    bool generate_array(char const* name, std::vector<int> const& v)
    {
        std::string generated;
        iterator_type sink(generated);
        int_array<iterator_type> g(name);
        if (karma::generate(sink, g, v))
        {
            std::cout << generated;
            return true;
        }
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main()
{
    // generate an array of integers with initializers
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    client::generate_array("array1", v);

    return 0;
}
