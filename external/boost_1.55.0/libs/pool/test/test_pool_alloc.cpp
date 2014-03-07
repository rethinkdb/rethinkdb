/* Copyright (C) 2000, 2001 Stephen Cleary
 * Copyright (C) 2011 Kwan Ting Chan
 * 
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <algorithm>
#include <deque>
#include <list>
#include <set>
#include <stdexcept>
#include <vector>

#include <cstdlib>
#include <ctime>

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

// This is a wrapper around a UserAllocator. It just registers alloc/dealloc
//  to/from the system memory. It's used to make sure pool's are allocating
//  and deallocating system memory properly.
// Do NOT use this class with static or singleton pools.
template <typename UserAllocator>
struct TrackAlloc
{
    typedef typename UserAllocator::size_type size_type;
    typedef typename UserAllocator::difference_type difference_type;

    static std::set<char *> allocated_blocks;

    static char * malloc(const size_type bytes)
    {
        char * const ret = UserAllocator::malloc(bytes);
        allocated_blocks.insert(ret);
        return ret;
    }

    static void free(char * const block)
    {
        BOOST_TEST(allocated_blocks.find(block) != allocated_blocks.end());
        allocated_blocks.erase(block);
        UserAllocator::free(block);
    }

    static bool ok()
    {
        return allocated_blocks.empty();
    }
};
template <typename UserAllocator>
std::set<char *> TrackAlloc<UserAllocator>::allocated_blocks;

typedef TrackAlloc<boost::default_user_allocator_new_delete> track_alloc;

void test()
{
    {
        // Do nothing pool
        boost::object_pool<tester> pool;
    }

    {
        // Construct several tester objects. Don't delete them (i.e.,
        //  test pool's garbage collection).
        boost::object_pool<tester> pool;
        for(int i=0; i < 10; ++i)
        {
            pool.construct();
        }
    }

    {
        // Construct several tester objects. Delete some of them.
        boost::object_pool<tester> pool;
        std::vector<tester *> v;
        for(int i=0; i < 10; ++i)
        {
            v.push_back(pool.construct());
        }
        std::random_shuffle(v.begin(), v.end());
        for(int j=0; j < 5; ++j)
        {
            pool.destroy(v[j]);
        }
    }

    {
        // Test how pool reacts with constructors that throw exceptions.
        //  Shouldn't have any memory leaks.
        boost::object_pool<tester> pool;
        for(int i=0; i < 5; ++i)
        {
            pool.construct();
        }
        for(int j=0; j < 5; ++j)
        {
            try
            {
                // The following constructions will raise an exception.
                pool.construct(true);
            }
            catch(const std::logic_error &) {}
        }
    }
}

void test_alloc()
{
    {
        // Allocate several tester objects. Delete one.
        std::vector<tester, boost::pool_allocator<tester> > l;
        for(int i=0; i < 10; ++i)
        {
            l.push_back(tester());
        }
        l.pop_back();
    }

    {
        // Allocate several tester objects. Delete two.
        std::deque<tester, boost::pool_allocator<tester> > l;
        for(int i=0; i < 10; ++i)
        {
            l.push_back(tester());
        }
        l.pop_back();
        l.pop_front();
    }

    {
        // Allocate several tester objects. Delete two.
        std::list<tester, boost::fast_pool_allocator<tester> > l;
        // lists rebind their allocators, so dumping is useless
        for(int i=0; i < 10; ++i)
        {
            l.push_back(tester());
        }
        l.pop_back();
        l.pop_front();
    }

    tester * tmp;
    {
        // Create a memory leak on purpose. (Allocator doesn't have
        //  garbage collection)
        // (Note: memory leak)
        boost::pool_allocator<tester> a;
        tmp = a.allocate(1, 0);
        new (tmp) tester();
    }
    if(mem.ok())
    {
        BOOST_ERROR("Pool allocator cleaned up itself");
    }
    // Remove memory checker entry (to avoid error later) and
    //  clean up memory leak
    tmp->~tester();
    boost::pool_allocator<tester>::deallocate(tmp, 1);

    // test allocating zero elements
    {
        boost::pool_allocator<tester> alloc;
        tester* ip = alloc.allocate(0);
        alloc.deallocate(ip, 0);
    }
}

void test_mem_usage()
{
    typedef boost::pool<track_alloc> pool_type;

    {
        // Constructor should do nothing; no memory allocation
        pool_type pool(sizeof(int));
        BOOST_TEST(track_alloc::ok());
        BOOST_TEST(!pool.release_memory());
        BOOST_TEST(!pool.purge_memory());

        // Should allocate from system
        pool.free(pool.malloc());
        BOOST_TEST(!track_alloc::ok());

        // Ask pool to give up memory it's not using; this should succeed
        BOOST_TEST(pool.release_memory());
        BOOST_TEST(track_alloc::ok());

        // Should allocate from system again
        pool.malloc(); // loses the pointer to the returned chunk (*A*)

        // Ask pool to give up memory it's not using; this should fail
        BOOST_TEST(!pool.release_memory());

        // Force pool to give up memory it's not using; this should succeed
        // This will clean up the memory leak from (*A*)
        BOOST_TEST(pool.purge_memory());
        BOOST_TEST(track_alloc::ok());

        // Should allocate from system again
        pool.malloc(); // loses the pointer to the returned chunk (*B*)

        // pool's destructor should purge the memory
        //  This will clean up the memory leak from (*B*)
    }

    BOOST_TEST(track_alloc::ok());
}

void test_void()
{
    typedef boost::pool_allocator<void> void_allocator;
    typedef boost::fast_pool_allocator<void> fast_void_allocator;

    typedef void_allocator::rebind<int>::other int_allocator;
    typedef fast_void_allocator::rebind<int>::other fast_int_allocator;

    std::vector<int, int_allocator> v1;
    std::vector<int, fast_int_allocator> v2;
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(0)));

    test();
    test_alloc();
    test_mem_usage();
    test_void();

    return boost::report_errors();
}
