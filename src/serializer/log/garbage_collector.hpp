
#ifndef __SERIALIZER_GARBAGE_COLLECTOR_HPP__
#define __SERIALIZER_GARBAGE_COLLECTOR_HPP__

#include <bitset>
#include <deque>
#include "alloc/alloc_mixin.hpp"

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
            Value &value;
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
    void update(int);
};

template<class Priority_Queue>
class base_value_t {
    public:
        /* these get set by our priority queue */
        Priority_Queue *priority_queue;
        int index;
    public:
        base_value_t();
        ~base_value_t();
};

/* \brief a bitarray_t records in a bitset which of N blocks are garbage
 *  by convention true or 1 = garbage,
 *  can false or 0 = live
 */
template<class Priority_Queue, int N>
class gcarray_t : public base_value_t<Priority_Queue>,
                  public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, gcarray_t<Priority_Queue, N> > {
private:
    std::bitset<N> data;
public:
    void set(size_t pos, bool value);
    bool get(size_t pos);
    bool operator< (const gcarray_t &b);
};

#include "serializer/log/garbage_collector.tcc"
#endif
