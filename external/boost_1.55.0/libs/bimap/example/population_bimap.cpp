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

#include <iostream>
#include <string>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/multiset_of.hpp>

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_inserter.hpp>

using namespace boost::bimaps;
using namespace boost;
using namespace std;

int main()
{
    {

    typedef bimap<

        string,
        multiset_of< optional<string> >

    > bm_type;

    bm_type bm;

    assign::insert( bm )

        ( "John" , string("lazarus" ) )
        ( "Peter", string("vinicius") )
        ( "Simon", string("vinicius") )
        ( "Brian", none               )
    ;

    cout << "John is working in "
         << bm.left.at( "John" ).get_value_or( "no project" )
         << endl;

    cout << "Project vinicius is being developed by " << endl;
    BOOST_FOREACH( bm_type::right_reference rp,
                   bm.right.equal_range( std::string("vinicius") ) )
    {
        cout << rp.second << endl;
    }

    cout << "This workers need a project " << endl;
    BOOST_FOREACH( bm_type::right_reference rp,
                   bm.right.equal_range(none) )
    {
        cout << rp.second << endl;
    }

}

    //[ code_population_bimap

    typedef bimap<

        unordered_set_of< std::string >,
        multiset_of< long, std::greater<long> >

    > population_bimap;

    typedef population_bimap::value_type population;

    population_bimap pop;
    pop.insert( population("China",          1321000000) );
    pop.insert( population("India",          1129000000) );
    pop.insert( population("United States",   301950000) );
    pop.insert( population("Indonesia",       234950000) );
    pop.insert( population("Brazil",          186500000) );
    pop.insert( population("Pakistan",        163630000) );

    std::cout << "Countries by their population:" << std::endl;

    // First requirement
    /*<< The right map view works like a
         `std::multimap< long, std::string, std::greater<long> >`,
         We can iterate over it to print the results in the required order. >>*/
    for( population_bimap::right_const_iterator
            i = pop.right.begin(), iend = pop.right.end();
            i != iend ; ++i )
    {
        std::cout << i->second << " with " << i->first << std::endl;
    }

    // Second requirement
    /*<< The left map view works like a `std::unordered_map< std::string, long >`,
         given the name of the country we can use it to search for the population 
         in constant time >>*/
    std::cout << "Population of China: " << pop.left.at("China") << std::endl;
    //]

    return 0;
}

