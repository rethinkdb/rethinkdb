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

#include <fstream>
#include <string>
#include <cassert>

#include <boost/bimap/bimap.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace boost::bimaps;

int main()
{
    //[ code_bimap_and_boost_serialization

    typedef bimap< std::string, int > bm_type;

    // Create a bimap and serialize it to a file
    {
        bm_type bm;
        bm.insert( bm_type::value_type("one",1) );
        bm.insert( bm_type::value_type("two",2) );

        std::ofstream ofs("data");
        boost::archive::text_oarchive oa(ofs);

        oa << const_cast<const bm_type&>(bm); /*<
            We must do a const cast because Boost.Serialization archives
            only save const objects. Read Boost.Serializartion docs for the
            rationale behind this decision >*/

        /*<< We can only serialize iterators if the bimap was serialized first.
             Note that the const cast is not requiered here because we create
             our iterators as const. >>*/
        const bm_type::left_iterator left_iter = bm.left.find("two");
        oa << left_iter;

        const bm_type::right_iterator right_iter = bm.right.find(1);
        oa << right_iter;
    }

    // Load the bimap back
    {
        bm_type bm;

        std::ifstream ifs("data", std::ios::binary);
        boost::archive::text_iarchive ia(ifs);

        ia >> bm;

        assert( bm.size() == 2 );

        bm_type::left_iterator left_iter;
        ia >> left_iter;

        assert( left_iter->first == "two" );

        bm_type::right_iterator right_iter;
        ia >> right_iter;

        assert( right_iter->first == 1 );
    }
    //]

    return 0;
}

