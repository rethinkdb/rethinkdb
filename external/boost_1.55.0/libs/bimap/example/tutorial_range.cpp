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
#include <boost/bimap/support/lambda.hpp>

using namespace boost::bimaps;

void using_upper_and_lower_bound()
{
    //[ code_tutorial_range_standard_way

    typedef bimap<int,std::string> bm_type;
    bm_type bm;

    // ...

    bm_type::left_iterator iter_first  = bm.left.lower_bound(20);
    bm_type::left_iterator iter_second = bm.left.upper_bound(50);

    // range [iter_first,iter_second) contains the elements in [20,50]
    //]

    // Subtle changes
    {
        //[ code_tutorial_range_standard_way_subtle_changes

        bm_type::left_iterator iter_first  = bm.left.upper_bound(20);
        bm_type::left_iterator iter_second = bm.left.lower_bound(50);

        // range [iter_first,iter_second) contains the elements in (20,50)
        //]
    }
}

void using_range()
{
    //[ code_tutorial_range

    typedef bimap<int,std::string> bm_type;
    bm_type bm;

    // ...

    /*<< `range_type` is a handy typedef equal to `std::pair<iterator,iterator>`.
         `const_range_type` is provided too, and it is equal to
         `std::pair<const_iterator,const_iterator>`  >>*/
    bm_type::left_range_type r;

    /*<< _key is a Boost.Lambda placeholder. To use it you have to include
        `<boost/bimap/support/lambda.hpp>` >>*/
    r = bm.left.range( 20 <= _key, _key <= 50 ); // [20,50]

    r = bm.left.range( 20 <  _key, _key <  50 ); // (20,50)

    r = bm.left.range( 20 <= _key, _key <  50 ); // [20,50)
    //]

    //[ code_tutorial_range_unbounded

    r = bm.left.range( 20 <= _key, unbounded ); // [20,inf)

    r = bm.left.range( unbounded , _key < 50 ); // (-inf,50)

    /*<< This is equivalent to std::make_pair(s.begin(),s.end()) >>*/
    r = bm.left.range( unbounded , unbounded ); // (-inf,inf)
    //]
}

int main()
{
    using_upper_and_lower_bound();
    using_range();

    return 0;
}


