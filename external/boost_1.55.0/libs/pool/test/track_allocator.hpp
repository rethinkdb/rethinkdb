/* Copyright (C) 2000, 2001 Stephen Cleary
* Copyright (C) 2011 Kwan Ting Chan
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_POOL_TRACK_ALLOCATOR_HPP
#define BOOST_POOL_TRACK_ALLOCATOR_HPP

#include <boost/detail/lightweight_test.hpp>

#include <new>
#include <set>
#include <stdexcept>

#include <cstddef>

// Each "tester" object below checks into and out of the "cdtor_checker",
//  which will check for any problems related to the construction/destruction of
//  "tester" objects.
class cdtor_checker
{
private:
    // Each constructed object registers its "this" pointer into "objs"
    std::set<void*> objs;

public:
    // True iff all objects that have checked in have checked out
    bool ok() const { return objs.empty(); }

    ~cdtor_checker()
    {
        BOOST_TEST(ok());
    }

    void check_in(void * const This)
    {
        BOOST_TEST(objs.find(This) == objs.end());
        objs.insert(This);
    }

    void check_out(void * const This)
    {
        BOOST_TEST(objs.find(This) != objs.end());
        objs.erase(This);
    }
};
static cdtor_checker mem;

struct tester
{
    tester(bool throw_except = false)
    {
        if(throw_except)
        {
            throw std::logic_error("Deliberate constructor exception");
        }

        mem.check_in(this);
    }

    tester(const tester &)
    {
        mem.check_in(this);
    }

    ~tester()
    {
        mem.check_out(this);
    }
};

// Allocator that registers alloc/dealloc to/from the system memory
struct track_allocator
{
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    static std::set<char*> allocated_blocks;

    static char* malloc(const size_type bytes)
    {
        char* const ret = new (std::nothrow) char[bytes];
        allocated_blocks.insert(ret);
        return ret;
    }

    static void free(char* const block)
    {
        BOOST_TEST(allocated_blocks.find(block) != allocated_blocks.end());
        allocated_blocks.erase(block);
        delete [] block;
    }

    static bool ok()
    {
        return allocated_blocks.empty();
    }
};
std::set<char*> track_allocator::allocated_blocks;

#endif // BOOST_POOL_TRACK_ALLOCATOR_HPP
