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

//[ code_tagged_simple_bimap

#include <iostream>

#include <boost/bimap.hpp>

struct country  {};
struct place    {};

int main()
{
    using namespace boost::bimaps;

    // Soccer World cup.

    typedef bimap
    <
        tagged< std::string, country >,
        tagged< int        , place   >

    > results_bimap;

    typedef results_bimap::value_type position;

    results_bimap results;
    results.insert( position("Argentina"    ,1) );
    results.insert( position("Spain"        ,2) );
    results.insert( position("Germany"      ,3) );
    results.insert( position("France"       ,4) );

    std::cout << "Countries names ordered by their final position:"
                << std::endl;

    /*<< `results.by<place>()` is equivalent to `results.right` >>*/
    for( results_bimap::map_by<place>::const_iterator
            i    = results.by<place>().begin(),
            iend = results.by<place>().end() ;
            i != iend; ++i )
    {
        /*<< `get<Tag>` works for each view of the bimap >>*/
        std::cout << i->get<place  >() << ") "
                  << i->get<country>() << std::endl;
    }

    std::cout << std::endl
              << "Countries names ordered alfabetically along with"
                 "their final position:"
              << std::endl;

    /*<< `results.by<country>()` is equivalent to `results.left` >>*/
    for( results_bimap::map_by<country>::const_iterator
            i    = results.by<country>().begin(),
            iend = results.by<country>().end() ;
            i != iend; ++i )
    {
        std::cout << i->get<country>() << " ends "
                  << i->get<place  >() << "º"
                  << std::endl;
    }

    return 0;
}
//]

