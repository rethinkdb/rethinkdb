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

#include <boost/bimap/bimap.hpp>

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>

using namespace boost::bimaps;
using namespace boost::xpressive;
namespace xp = boost::xpressive;

int main()
{
    //[ code_bimap_and_boost_xpressive

    typedef bimap< std::string, int > bm_type;
    bm_type bm;

    std::string rel_str("one <--> 1     two <--> 2      three <--> 3");

    sregex rel = ( (s1= +_w) >> " <--> " >> (s2= +_d) )
    [
        xp::ref(bm)->*insert( xp::construct<bm_type::value_type>(s1, as<int>(s2)) )
    ];

    sregex relations = rel >> *(+_s >> rel);

    regex_match(rel_str, relations);

    assert( bm.size() == 3 );
    //]

    return 0;
}

