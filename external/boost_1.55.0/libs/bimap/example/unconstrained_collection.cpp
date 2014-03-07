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

#include <string>
#include <iostream>

#include <map>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unconstrained_set_of.hpp>
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;

int main()
{
    // Boost.Bimap
    {
    //[ code_unconstrained_collection_bimap

    typedef bimap< std::string, unconstrained_set_of<int> > bm_type;
    typedef bm_type::left_map map_type;

    bm_type bm;
    map_type & m = bm.left;
    //]

    //[ code_unconstrained_collection_common

    m["one"] = 1;

    assert( m.find("one") != m.end() );

    for( map_type::iterator i = m.begin(), iend = m.end(); i != iend; ++i )
    {
        /*<< The right collection of the bimap is mutable so its elements 
             can be modified using iterators. >>*/
        ++(i->second);
    }

    m.erase("one");
    //]

    m["one"] = 1;
    m["two"] = 2;

    //[ code_unconstrained_collection_only_for_bimap
    typedef map_type::const_iterator const_iterator;
    typedef std::pair<const_iterator,const_iterator> const_range;

    /*<< This range is a model of BidirectionalRange, read the docs of 
         Boost.Range for more information. >>*/
    const_range r = m.range( "one" <= _key, _key <= "two" );
    for( const_iterator i = r.first; i != r.second; ++i )
    {
        std::cout << i->first << "-->" << i->second << std::endl;
    }

    m.modify_key( m.begin(), _key = "1" );
    //]
    }

    // Standard map
    {
    //[ code_unconstrained_collection_map

    typedef std::map< std::string, int > map_type;

    map_type m;
    //]
    }

    return 0;
}


