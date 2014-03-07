// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <rtree/test_rtree.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

template <typename Point, typename Parameters>
void test_rtree_interprocess(Parameters const& parameters = Parameters())
{
    namespace bi = boost::interprocess;
    struct shm_remove
    {
        shm_remove() { bi::shared_memory_object::remove("shmem"); }
        ~shm_remove(){ bi::shared_memory_object::remove("shmem"); }
    } remover;

    bi::managed_shared_memory segment(bi::create_only, "shmem", 65535);
    typedef bi::allocator<Point, bi::managed_shared_memory::segment_manager> shmem_alloc;

    test_rtree_for_box<Point>(parameters, shmem_alloc(segment.get_segment_manager()));
}

namespace testset { namespace interprocess {

template <typename Indexable, typename Parameters>
void modifiers(Parameters const& parameters = Parameters())
{
    namespace bi = boost::interprocess;
    struct shm_remove
    {
        shm_remove() { bi::shared_memory_object::remove("shmem"); }
        ~shm_remove(){ bi::shared_memory_object::remove("shmem"); }
    } remover;

    bi::managed_shared_memory segment(bi::create_only, "shmem", 65535);
    typedef bi::allocator<Indexable, bi::managed_shared_memory::segment_manager> shmem_alloc;

    testset::modifiers<Indexable>(parameters, shmem_alloc(segment.get_segment_manager()));
}

template <typename Indexable, typename Parameters>
void queries(Parameters const& parameters = Parameters())
{
    namespace bi = boost::interprocess;
    struct shm_remove
    {
        shm_remove() { bi::shared_memory_object::remove("shmem"); }
        ~shm_remove(){ bi::shared_memory_object::remove("shmem"); }
    } remover;

    bi::managed_shared_memory segment(bi::create_only, "shmem", 65535);
    typedef bi::allocator<Indexable, bi::managed_shared_memory::segment_manager> shmem_alloc;

    testset::queries<Indexable>(parameters, shmem_alloc(segment.get_segment_manager()));
}

template <typename Indexable, typename Parameters>
void additional(Parameters const& parameters = Parameters())
{
    namespace bi = boost::interprocess;
    struct shm_remove
    {
        shm_remove() { bi::shared_memory_object::remove("shmem"); }
        ~shm_remove(){ bi::shared_memory_object::remove("shmem"); }
    } remover;

    bi::managed_shared_memory segment(bi::create_only, "shmem", 65535);
    typedef bi::allocator<Indexable, bi::managed_shared_memory::segment_manager> shmem_alloc;

    testset::additional<Indexable>(parameters, shmem_alloc(segment.get_segment_manager()));
}

template <typename Indexable, typename Parameters>
void modifiers_and_additional(Parameters const& parameters = Parameters())
{
    namespace bi = boost::interprocess;
    struct shm_remove
    {
        shm_remove() { bi::shared_memory_object::remove("shmem"); }
        ~shm_remove(){ bi::shared_memory_object::remove("shmem"); }
    } remover;

    bi::managed_shared_memory segment(bi::create_only, "shmem", 65535);
    typedef bi::allocator<Indexable, bi::managed_shared_memory::segment_manager> shmem_alloc;

    testset::modifiers<Indexable>(parameters, shmem_alloc(segment.get_segment_manager()));
    testset::additional<Indexable>(parameters, shmem_alloc(segment.get_segment_manager()));
}

}}
