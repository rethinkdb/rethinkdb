//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show the uniform and easy way of
//  output formatting for different container types. 
//
//  The 'auto_' primitive used below is very similar to the 'stream' primitive
//  demonstrated in the example 'basic_facilities.cpp' as it allows to generate
//  output from a multitude of data types. The main difference is that it is
//  mapped to the correct Karma generator instead of using any available 
//  operator<<() for the contained data type. Additionally this means, that
//  the format descriptions used below will be usable for any contained type as
//  long as this type has a defined mapping to a Karma generator.

//  use a larger value for the alignment field width (default is 10)
#define BOOST_KARMA_DEFAULT_FIELD_LENGTH 25

#include <boost/config/warning_disable.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <cstdlib> 

#include <boost/range.hpp>
#include <boost/array.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/array.hpp>

#include <boost/spirit/include/karma.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    // We add a specialization for the create_generator customization point
    // defining a custom output format for the value type of the std::map used
    // below (std::pair<int const, std::string>). Generally, any specialization 
    // for create_generator is expected to return the proto expression to be 
    // used to generate output for the type the customization point has been 
    // specialized for.
    //
    // We need to utilize proto::deep_copy as the expression contains a literal 
    // (the ':') which normally gets embedded in the proto expression by 
    // reference only. The deep copy converts the proto tree to hold this by 
    // value. The deep copy operation can be left out for simpler proto 
    // expressions (not containing references to temporaries). Alternatively
    // you could use the proto::make_expr() facility to build the required
    // proto expression.
    template <>
    struct create_generator<std::pair<int const, std::string> >
    {
        typedef proto::result_of::deep_copy<
            BOOST_TYPEOF(int_ << ':' << string)
        >::type type;

        static type call()
        {
            return proto::deep_copy(int_ << ':' << string);
        }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
// Output the given containers in list format
// Note: the format description does not depend on the type of the sequence
//       nor does it depend on the type of the elements contained in the 
//       sequence
///////////////////////////////////////////////////////////////////////////////
template <typename Container>
void output_container(std::ostream& os, Container const& c)
{
    // output the container as a sequence without separators
    os << 
        karma::format(
            auto_,                                // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            *auto_,                               // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a space separated sequence
    os << 
        karma::format_delimited(
            auto_,                                // format description
            space,                                // delimiter
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format_delimited(
            *auto_,                               // format description
            space,                                // delimiter
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format_delimited(
            '[' << *auto_ << ']',                 // format description
            space,                                // delimiter
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list
    os << 
        karma::format(
            auto_ % ", ",                         // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (auto_ % ", ") << ']',         // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << -(auto_ % ", ") << ']',        // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (+auto_ | "empty") << ']',     // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list of items enclosed in '()'
    os << 
        karma::format(
            ('(' << auto_ << ')') % ", ",         // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (  
                ('(' << auto_ << ')') % ", "
             )  << ']',                           // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a HTML list
    os << 
        karma::format_delimited(
            "<ol>" << 
                *verbatim["<li>" << auto_ << "</li>"]
            << "</ol>",                           // format description
            '\n',                                 // delimiter
            c                                     // data
        ) << std::endl;

    // output the container as right aligned column
    os << 
        karma::format_delimited(
           *verbatim[
                "|" << right_align[auto_] << "|"
            ],                                    // format description
            '\n',                                 // delimiter
            c                                     // data
        ) << std::endl;

    os << std::endl;
}

int main()
{
    ///////////////////////////////////////////////////////////////////////////
    // C-style array
    int i[4] = { 3, 6, 9, 12 };

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "int i[]" << std::endl;
    output_container(std::cout, boost::make_iterator_range(i, i+4));

    ///////////////////////////////////////////////////////////////////////////
    // vector
    std::vector<int> v (5);
    std::generate(v.begin(), v.end(), std::rand); // randomly fill the vector

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "std::vector<int>" << std::endl;
    output_container(std::cout, v);

    ///////////////////////////////////////////////////////////////////////////
    // list
    std::list<char> l;
    l.push_back('A');
    l.push_back('B');
    l.push_back('C');

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "std::list<char>" << std::endl;
    output_container(std::cout, l);

    ///////////////////////////////////////////////////////////////////////////
    // strings
    std::string str("Hello world!");

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "std::string" << std::endl;
    output_container(std::cout, str);

    ///////////////////////////////////////////////////////////////////////////
    // boost::array
    boost::array<long, 5> arr;
    std::generate(arr.begin(), arr.end(), std::rand); // randomly fill the array

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "boost::array<long, 5>" << std::endl;
    output_container(std::cout, arr);

    ///////////////////////////////////////////////////////////////////////////
    //  map of int --> string mappings
    std::map<int, std::string> mappings;
    mappings.insert(std::make_pair(0, "zero"));
    mappings.insert(std::make_pair(1, "one"));
    mappings.insert(std::make_pair(2, "two"));

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "std::map<int, std::string>" << std::endl;
    output_container(std::cout, mappings);

    return 0;
}

