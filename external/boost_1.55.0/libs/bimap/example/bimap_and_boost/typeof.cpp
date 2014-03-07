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
#include <boost/bimap/bimap.hpp>

#include <boost/typeof/typeof.hpp>

using namespace boost::bimaps;

struct name   {};
struct number {};

void using_auto()
{
    //[ code_bimap_and_boost_typeof_first

    typedef bimap< tagged<std::string,name>, tagged<int,number> > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type("one"  ,1) );
    bm.insert( bm_type::value_type("two"  ,2) );
    //]

    //[ code_bimap_and_boost_typeof_using_auto

    for( BOOST_AUTO(iter, bm.by<name>().begin()); iter!=bm.by<name>().end(); ++iter)
    {
        std::cout << iter->first << " --> " << iter->second << std::endl;
    }

    BOOST_AUTO( iter, bm.by<number>().find(2) );
    std::cout << "2: " << iter->get<name>();
    //]
}

void not_using_auto()
{
    typedef bimap< tagged<std::string,name>, tagged<int,number> > bm_type;
    bm_type bm;
    bm.insert( bm_type::value_type("one"  ,1) );
    bm.insert( bm_type::value_type("two"  ,2) );

    //[ code_bimap_and_boost_typeof_not_using_auto

    for( bm_type::map_by<name>::iterator iter = bm.by<name>().begin();
         iter!=bm.by<name>().end(); ++iter)
    {
        std::cout << iter->first << " --> " << iter->second << std::endl;
    }

    bm_type::map_by<number>::iterator iter = bm.by<number>().find(2);
    std::cout << "2: " << iter->get<name>();
    //]
}

int main()
{
    using_auto();
    not_using_auto();

    return 0;
}




