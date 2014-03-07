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

#include <boost/config.hpp>

// std
#include <set>
#include <map>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <algorithm>

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

// Boost.Bimap
#include <boost/bimap/bimap.hpp>


template< class Bimap, class Archive >
void save_bimap(const Bimap & b, Archive & ar)
{
    using namespace boost::bimaps;

    ar << b;

    const typename Bimap::left_const_iterator left_iter = b.left.begin();
    ar << left_iter;

    const typename Bimap::const_iterator iter = ++b.begin();
    ar << iter;
}




void test_bimap_serialization()
{
    using namespace boost::bimaps;

    typedef bimap<int,double> bm;

    std::set< bm::value_type > data;
    data.insert( bm::value_type(1,0.1) );
    data.insert( bm::value_type(2,0.2) );
    data.insert( bm::value_type(3,0.3) );
    data.insert( bm::value_type(4,0.4) );

    std::ostringstream oss;

    // Save it
    {
        bm b;

        b.insert(data.begin(),data.end());

        boost::archive::text_oarchive oa(oss);

        save_bimap(b,oa);
    }

    // Reload it
    {
        bm b;

        std::istringstream iss(oss.str());
        boost::archive::text_iarchive ia(iss);

        ia >> b;

        BOOST_CHECK( std::equal( b.begin(), b.end(), data.begin() ) );

        bm::left_const_iterator left_iter;

        ia >> left_iter;

        BOOST_CHECK( left_iter == b.left.begin() );

        bm::const_iterator iter;

        ia >> iter;

        BOOST_CHECK( iter == ++b.begin() );
    }

}


int test_main( int, char* [] )
{
    test_bimap_serialization();
    return 0;
}

