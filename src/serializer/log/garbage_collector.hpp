
#ifndef __SERIALIZER_GARBAGE_COLLECTOR_HPP__
#define __SERIALIZER_GARBAGE_COLLECTOR_HPP__

#include <bitset>
#include <deque>
#include "alloc/alloc_mixin.hpp"

class datablock_bitarray_t : public std::bitset<EXTENT_SIZE / BTREE_BLOCK_SIZE>, 
                         public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, datablock_bitarray_t> {
public:
    bool operator[] (size_t pos) const;
    reference operator[] (size_t pos);
};

/* \brief priority_queue_t
 * Priority queues are by defined to be max priority queues, that is
 * pop() gives you an element a such that for all b in the heap Less(b, a) == True
 */
template<class Key, class Value, class Less>
struct priority_queue_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, priority_queue_t<Key, Value, Less> > {
private:
    struct entry_t {
        public:
            Key key;
            Value value;
    };
private:
    std::deque<entry_t> heap;
private:
    inline int parent(int);
    inline int left(int);
    inline int right(int);
    inline void swap(int, int);
    inline void bubble_up(int &);
    inline void bubble_down(int &);
public:
    priority_queue_t();
    ~priority_queue_t();
    bool empty();
    size_t size();
    Key peak();
    void push(Key, Value&);
    Key pop();
};

#include "serializer/log/garbage_collector.tcc"
#endif
