/* Copyright (C) 2011 Kwan Ting Chan
 * 
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

// Test of bug #4346 (https://svn.boost.org/trac/boost/ticket/4346)

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <boost/pool/poolfwd.hpp>
#include <boost/pool/simple_segregated_storage.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/pool/object_pool.hpp>

#include <vector>

struct Foo {};

int main()
{
    {
        boost::pool<> p(sizeof(int));
        (p.malloc)();
    }

    {
        boost::object_pool<Foo> p;
        (p.malloc)();
    }

    {
        (boost::singleton_pool<Foo, sizeof(int)>::malloc)();
    }
    boost::singleton_pool<Foo, sizeof(int)>::purge_memory();

    {
        std::vector<int, boost::pool_allocator<int> > v;
        v.push_back(8);
    }
    boost::singleton_pool<boost::pool_allocator_tag,
        sizeof(int)>::release_memory();
}
