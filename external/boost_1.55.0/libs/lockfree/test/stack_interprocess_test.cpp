//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib> //std::system
#include <sstream>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/lockfree/stack.hpp>
#include <boost/thread/thread.hpp>

using namespace boost::interprocess;
typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;
typedef boost::lockfree::stack<int,
                               boost::lockfree::allocator<ShmemAllocator>,
                               boost::lockfree::capacity<2048>
                              > stack;

int main (int argc, char *argv[])
{
   if(argc == 1){
        struct shm_remove
        {
            shm_remove() {  shared_memory_object::remove("MySharedMemory"); }
            ~shm_remove(){  shared_memory_object::remove("MySharedMemory"); }
        } remover;

        managed_shared_memory segment(create_only, "MySharedMemory", 65536);
        ShmemAllocator alloc_inst (segment.get_segment_manager());

        stack * queue = segment.construct<stack>("stack")(alloc_inst);
        for (int i = 0; i != 1024; ++i)
            queue->push(i);

        std::string s(argv[0]); s += " child ";
        if(0 != std::system(s.c_str()))
            return 1;

        while (!queue->empty())
            boost::thread::yield();
        return 0;
    } else {
        managed_shared_memory segment(open_only, "MySharedMemory");
        stack * queue = segment.find<stack>("stack").first;

        int from_queue;
        for (int i = 0; i != 1024; ++i) {
            bool success = queue->pop(from_queue);
            assert (success);
            assert (from_queue == 1023 - i);
        }
        segment.destroy<stack>("stack");
    }
    return 0;
}
