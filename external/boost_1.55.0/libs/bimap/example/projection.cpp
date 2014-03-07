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
#include <boost/bimap/multiset_of.hpp>
using namespace boost::bimaps;

void years_example()
{
    //[ code_projection_years

    typedef bimap<std::string,multiset_of<int,std::greater<int> > > bm_type;

    bm_type bm;
    bm.insert( bm_type::value_type("John" ,34) );
    bm.insert( bm_type::value_type("Peter",24) );
    bm.insert( bm_type::value_type("Mary" ,12) );

    // Find the name of the next younger person after Peter

    bm_type::left_const_iterator name_iter = bm.left.find("Peter");

    bm_type::right_const_iterator years_iter = bm.project_right(name_iter);

    ++years_iter;

    std::cout << "The next younger person after Peter is " << years_iter->second;
    //]
}

int main()
{
    years_example();

    return 0;
}


