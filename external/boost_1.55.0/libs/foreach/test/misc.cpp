//  misc.cpp
//
//  (C) Copyright Eric Niebler 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   4 March 2008 : Initial version.
*/

#include <vector>
#include <boost/test/minimal.hpp>
#include <boost/foreach.hpp>

struct xxx : std::vector<int>
{
    virtual ~xxx() = 0;
};

void test_abstract(xxx& rng)
{
    BOOST_FOREACH (int x, rng)
    {
        (void)x;
    }
}

struct yyy : std::vector<int>
{
    void test()
    {
        BOOST_FOREACH(int x, *this)
        {
            (void)x;
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    return 0;
}
