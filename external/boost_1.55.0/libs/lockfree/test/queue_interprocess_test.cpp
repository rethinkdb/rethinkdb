//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib> //std::system
#include <sstream>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/thread.hpp>

using namespace boost::interprocess;
typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;
typedef boost::lockfree::queue<int,
                               boost::lockfree::allocator<ShmemAllocator>,
                               boost::lockfree::capacity<2048>
                              > queue;

int main (int argc, char *argv[])
{
   if(argc == 1){
        struct shm_remove
        {
            shm_remove() {  shared_memory_object::remove("boost_queue_interprocess_test_shm"); }
            ~shm_remove(){  shared_memory_object::remove("boost_queue_interprocess_test_shm"); }
        } remover;

        managed_shared_memory segment(create_only, "boost_queue_interprocess_test_shm", 262144);
        ShmemAllocator alloc_inst (segment.get_segment_manager());

        queue * q = segment.construct<queue>("queue")(alloc_inst);
        for (int i = 0; i != 1024; ++i)
            q->push(i);

        std::string s(argv[0]); s += " child ";
        if(0 != std::system(s.c_str()))
            return 1;

        while (!q->empty())
            boost::thread::yield();
        return 0;
    } else {
        managed_shared_memory segment(open_only, "boost_queue_interprocess_test_shm");
        queue * q = segment.find<queue>("queue").first;

        int from_queue;
        for (int i = 0; i != 1024; ++i) {
            bool success = q->pop(from_queue);
            assert (success);
            assert (from_queue == i);
        }
        segment.destroy<queue>("queue");
    }
    return 0;
}
