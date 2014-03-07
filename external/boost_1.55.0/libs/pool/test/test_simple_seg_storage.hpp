/* Copyright (C) 2000, 2001 Stephen Cleary
* Copyright (C) 2011 Kwan Ting Chan
* 
* Use, modification and distribution is subject to the 
* Boost Software License, Version 1.0. (See accompanying
* file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_POOL_TEST_SIMP_SEG_STORE_HPP
#define BOOST_POOL_TEST_SIMP_SEG_STORE_HPP

#include <boost/pool/simple_segregated_storage.hpp>
#include <boost/assert.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <functional>
#include <set>
#include <utility>
#include <vector>

#include <cstddef>

class test_simp_seg_store : public boost::simple_segregated_storage<std::size_t>
{
private:
    // ::first is the address of the start of the added block,
    //  ::second is the size in bytes of the added block
    std::vector<std::pair<void*, std::size_t> > allocated_blocks;
    size_type np_sz;
    std::set<void*> allocated_chunks;

    void set_partition_size(const size_type sz)
    {
        if(allocated_blocks.empty())
        {
            np_sz = sz;
        }
        else
        {
            BOOST_ASSERT(np_sz == sz);
        }
    }

    // Return: true if chunk is from added blocks, false otherwise
    bool is_inside_allocated_blocks(void* const chunk) const
    {
        typedef std::vector<std::pair<void*, std::size_t> >::const_iterator
            VPIter;
        for(VPIter iter = allocated_blocks.begin();
            iter != allocated_blocks.end();
            ++iter)
        {
            if( std::less_equal<void*>()(iter->first, chunk)
                && std::less_equal<void*>()(static_cast<char*>(chunk) + np_sz,
                    static_cast<char*>(iter->first) + iter->second) )
            {
                return true;
            }
        }

        return false;
    }

    void check_in(void* const chunk)
    {
        // Check that the newly allocated chunk has not already previously
        //  been allocated, and that the memory does not overlap with
        //  previously allocated chunks
        for(std::set<void*>::const_iterator iter = allocated_chunks.begin();
            iter != allocated_chunks.end();
            ++iter)
        {
            BOOST_TEST( std::less_equal<void*>()(static_cast<char*>(chunk)
                + np_sz, *iter)
                || std::less_equal<void*>()(static_cast<char*>(*iter)
                + np_sz, chunk) );
        }

        allocated_chunks.insert(chunk);
    }

    void check_out(void* const chunk)
    {
        BOOST_TEST(allocated_chunks.erase(chunk) == 1);
    }

public:
    test_simp_seg_store()
        : np_sz(0) {}

    void* get_first() { return first; }
    static void*& get_nextof(void* const ptr) { return nextof(ptr); }

    // (Test) Pre: npartition_sz of all added block is the same
    //             different blocks of memory does not overlap
    void add_block(void* const block,
        const size_type nsz, const size_type npartition_sz)
    {
        set_partition_size(npartition_sz);
        allocated_blocks.push_back(
            std::pair<void*, std::size_t>(block, nsz) );
        boost::simple_segregated_storage<std::size_t>::add_block(
            block, nsz, npartition_sz );
        // Post: !empty()
        BOOST_TEST(!empty());
    }

    // (Test) Pre: npartition_sz of all added block is the same
    //             different blocks of memory does not overlap
    void add_ordered_block(void* const block,
        const size_type nsz, const size_type npartition_sz)
    {
        set_partition_size(npartition_sz);
        allocated_blocks.push_back(
            std::pair<void*, std::size_t>(block, nsz) );
        boost::simple_segregated_storage<std::size_t>::add_ordered_block(
            block, nsz, npartition_sz );
        // Post: !empty()
        BOOST_TEST(!empty());
    }

    void* malloc()
    {
        void* const ret
            = boost::simple_segregated_storage<std::size_t>::malloc();
        // Chunk returned should actually be from added blocks
        BOOST_TEST(is_inside_allocated_blocks(ret));
        check_in(ret);
        return ret;
    }

    void free(void* const chunk)
    {
        BOOST_ASSERT(chunk);
        check_out(chunk);
        boost::simple_segregated_storage<std::size_t>::free(chunk);
        // Post: !empty()
        BOOST_TEST(!empty());
    }

    void ordered_free(void* const chunk)
    {
        BOOST_ASSERT(chunk);
        check_out(chunk);
        boost::simple_segregated_storage<std::size_t>::ordered_free(chunk);
        // Post: !empty()
        BOOST_TEST(!empty());
    }

    void* malloc_n(size_type n, size_type partition_size)
    {
        void* const ret
            = boost::simple_segregated_storage<std::size_t>::malloc_n(
                n, partition_size );

        if(ret)
        {
            for(std::size_t i=0; i < n; ++i)
            {
                void* const chunk = static_cast<char*>(ret)
                    + (i * partition_size);
                // Memory returned should actually be from added blocks
                BOOST_TEST(is_inside_allocated_blocks(chunk));
                check_in(chunk);
            }
        }

        return ret;
    }
};

#endif // BOOST_POOL_TEST_SIMP_SEG_STORE_HPP
