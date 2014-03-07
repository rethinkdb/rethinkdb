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

// Boost.Test
#include <boost/test/minimal.hpp>

// Boost.MPL
#include <boost/mpl/list.hpp>
#include <boost/type_traits/is_same.hpp>

// Boost.Bimap
#include <boost/bimap/relation/detail/mutant.hpp>

using namespace boost::bimaps::relation::detail;

// The mutant idiom is standard if only POD types are used.

typedef double  type_a;
typedef int     type_b;

const type_a value_a = 1.4;
const type_b value_b = 3;

struct Data
{
    type_a a;
    type_b b;
};

struct StdPairView
{
    typedef type_a first_type;
    typedef type_b second_type;
    type_a first;
    type_b second;
};

struct ReverseStdPairView
{
    typedef type_a second_type;
    typedef type_b first_type;
    type_a second;
    type_b first;
};


struct MutantData
{
    typedef boost::mpl::list< StdPairView, ReverseStdPairView > mutant_views;

    MutantData(type_a ap, type_b bp) : a(ap), b(bp) {}
    type_a a;
    type_b b;
};


void test_mutant_basic()
{

    // mutant test
    {
        MutantData m(value_a,value_b);

        BOOST_CHECK( sizeof( MutantData ) == sizeof( StdPairView ) );

        BOOST_CHECK( mutate<StdPairView>(m).first  == value_a );
        BOOST_CHECK( mutate<StdPairView>(m).second == value_b );
        BOOST_CHECK( mutate<ReverseStdPairView>(m).first  == value_b );
        BOOST_CHECK( mutate<ReverseStdPairView>(m).second == value_a );

        ReverseStdPairView & rpair = mutate<ReverseStdPairView>(m);
        rpair.first = value_b;
        rpair.second = value_a;

        BOOST_CHECK( mutate<StdPairView>(m).first  == value_a );
        BOOST_CHECK( mutate<StdPairView>(m).second == value_b );

        BOOST_CHECK( &mutate<StdPairView>(m).first  == &m.a );
        BOOST_CHECK( &mutate<StdPairView>(m).second == &m.b );
    }
}

int test_main( int, char* [] )
{
    test_mutant_basic();
    return 0;
}
