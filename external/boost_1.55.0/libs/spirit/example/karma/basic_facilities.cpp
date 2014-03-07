//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The main purpose of this example is to show the uniform and easy way of
//  output formatting for different container types. 
//
//  Since the 'stream' primitive used below uses the streaming operator defined 
//  for the container value_type, you must make sure to have a corresponding
//  operator<<() available for this contained data type. OTOH this means, that
//  the format descriptions used below will be usable for any contained type as
//  long as this type has an associated streaming operator defined.

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
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/fusion/include/std_pair.hpp>

///////////////////////////////////////////////////////////////////////////////
// This streaming operator is needed to generate the output from the map below
// Yes, it's heresy, but this operator has to live in namespace std to be 
// picked up by the compiler.
namespace std {
    inline std::ostream& 
    operator<<(std::ostream& os, std::pair<int const, std::string> v)
    {
        os << v.first << ": " << v.second;
        return os;
    }
}

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_format.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
// Output the given containers in list format
// Note: the format description does not depend on the type of the sequence
//       nor does it depend on the type of the elements contained in the 
//       sequence
///////////////////////////////////////////////////////////////////////////////
template <typename Container>
void output_container(std::ostream& os, Container const& c)
{
    // output the container as a space separated sequence
    os << 
        karma::format(
            *stream,                              // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a space separated sequence
    os << 
        karma::format_delimited(
            *stream,                              // format description
            space,                                // delimiter
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format_delimited(
            '[' << *stream << ']',                // format description
            space,                                // delimiter
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list
    os << 
        karma::format(
            stream % ", ",                        // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (stream % ", ") << ']',        // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << -(stream % ", ") << ']',       // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (+stream | "empty") << ']',    // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a comma separated list of items enclosed in '()'
    os << 
        karma::format(
            ('(' << stream << ')') % ", ",        // format description
            c                                     // data
        ) << std::endl << std::endl;

    os << 
        karma::format(
            '[' << (  
                ('(' << stream << ')') % ", "
             )  << ']',                           // format description
            c                                     // data
        ) << std::endl << std::endl;

    // output the container as a HTML list
    os << 
        karma::format_delimited(
            "<ol>" << 
                *verbatim["<li>" << stream << "</li>"]
            << "</ol>",                           // format description
            '\n',                                 // delimiter
            c                                     // data
        ) << std::endl;

    // output the container as right aligned column
    os << 
        karma::format_delimited(
           *verbatim[
                "|" << right_align[stream] << "|"
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
    //  vector of boost::date objects
    //  Note: any registered facets get used!
    using namespace boost::gregorian;
    std::vector<date> dates;
    dates.push_back(date(2005, Jun, 25));
    dates.push_back(date(2006, Jan, 13));
    dates.push_back(date(2007, May, 03));

    date_facet* facet(new date_facet("%A %B %d, %Y"));
    std::cout.imbue(std::locale(std::cout.getloc(), facet));

    std::cout << "-------------------------------------------------------------" 
              << std::endl;
    std::cout << "std::vector<boost::date>" << std::endl;
    output_container(std::cout, dates);

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

