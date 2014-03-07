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

#include <boost/foreach.hpp>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;


int main()
{
    //[ code_bimap_and_boost_foreach

    typedef bimap< std::string, list_of<int> > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("1", 1) );
    bm.insert( bm_type::value_type("2", 2) );
    bm.insert( bm_type::value_type("3", 4) );
    bm.insert( bm_type::value_type("4", 2) );

    BOOST_FOREACH( bm_type::left_reference p, bm.left )
    {
        ++p.second; /*< We can modify the right element because we have
                        use a mutable collection type in the right side. >*/
    }

    BOOST_FOREACH( bm_type::right_const_reference p, bm.right )
    {
        std::cout << p.first << "-->" << p.second << std::endl;
    }

    //]

    // More examples

    BOOST_FOREACH( bm_type::right_reference p, bm.right )
    {
        ++p.first;
    }

    BOOST_FOREACH( bm_type::left_const_reference p, bm.left )
    {
        std::cout << p.first << "-->" << p.second << std::endl;
    }

    BOOST_FOREACH( bm_type::reference p, bm )
    {
        ++p.right;
    }

    const bm_type & cbm = bm;
    BOOST_FOREACH( bm_type::const_reference p, cbm )
    {
        std::cout << p.left << "-->" << p.right << std::endl;
    }

    BOOST_FOREACH( bm_type::const_reference p, bm )
    {
        std::cout << p.left << "-->" << p.right << std::endl;
    }

    //[ code_bimap_and_boost_foreach_using_range

    BOOST_FOREACH( bm_type::left_reference p,
                 ( bm.left.range( std::string("1") <= _key, _key < std::string("3") ) ))
    {
        ++p.second;
    }

    BOOST_FOREACH( bm_type::left_const_reference p,
                 ( bm.left.range( std::string("1") <= _key, _key < std::string("3") ) ))
    {
        std::cout << p.first << "-->" << p.second << std::endl;
    }
    //]

    return 0;
}


