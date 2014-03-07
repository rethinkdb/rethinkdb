// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  VC++ 8.0 warns on usage of certain Standard Library and API functions that
//  can be cause buffer overruns or other possible security issues if misused.
//  See http://msdn.microsoft.com/msdnmag/issues/05/05/SafeCandC/default.aspx
//  But the wording of the warning is misleading and unsettling, there are no
//  portable alternative functions, and VC++ 8.0's own libraries use the
//  functions in question. So turn off the warnings.
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_DEPRECATE

// Boost.Bimap Example
//-----------------------------------------------------------------------------

#include <boost/config.hpp>

//[ code_simple_bimap

#include <string>
#include <iostream>

#include <boost/bimap.hpp>

template< class MapType >
void print_map(const MapType & map,
               const std::string & separator,
               std::ostream & os )
{
    typedef typename MapType::const_iterator const_iterator;

    for( const_iterator i = map.begin(), iend = map.end(); i != iend; ++i )
    {
        os << i->first << separator << i->second << std::endl;
    }
}

int main()
{
    // Soccer World cup

    typedef boost::bimap< std::string, int > results_bimap;
    typedef results_bimap::value_type position;

    results_bimap results;
    results.insert( position("Argentina"    ,1) );
    results.insert( position("Spain"        ,2) );
    results.insert( position("Germany"      ,3) );
    results.insert( position("France"       ,4) );

    std::cout << "The number of countries is " << results.size()
              << std::endl;

    std::cout << "The winner is " << results.right.at(1)
              << std::endl
              << std::endl;

    std::cout << "Countries names ordered by their final position:"
              << std::endl;

    // results.right works like a std::map< int, std::string >

    print_map( results.right, ") ", std::cout );

    std::cout << std::endl
              << "Countries names ordered alphabetically along with"
                    "their final position:"
              << std::endl;

    // results.left works like a std::map< std::string, int >

    print_map( results.left, " ends in position ", std::cout );

    return 0;
}
//]

