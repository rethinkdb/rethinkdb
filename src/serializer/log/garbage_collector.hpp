
#ifndef __SERIALIZER_GARBAGE_COLLECTOR_HPP__
#define __SERIALIZER_GARBAGE_COLLECTOR_HPP__

#include <bitset>
#include <deque>
#include <functional>
#include "alloc/alloc_mixin.hpp"

/* \brief priority_queue_t
 * Priority queues are by defined to be max priority queues, that is
 * pop() gives you an element a such that for all b in the heap Less(b, a) == True
 */
template<class T, class Less>
class priority_queue_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, priority_queue_t<T, Less> > {
public:
    struct entry_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, entry_t> {
        public:
            T data;

        /* these 2 fields are accessed the our friend the priority_queue_t */
        friend class priority_queue_t<T, Less>;
        private:
            priority_queue_t<T, Less> *pq; /* !< the priority queue the data is registered in */
            int index;                         /* the index withing the priority queue */

        public:
            /* \brief update() should be called after an change is made to the data
             * to preserve the order in the queue
             */
            void update();
            bool operator< (const entry_t &b) {return Less(data, b.data);}
    };
    //typedef entry_t entry;
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
    T peak();
    entry_t *push(T);
    T pop();
    void update(int);
};

/* extend standard less to work with bitsets the way we want
 */
namespace std {
    template <int N> struct less<const std::bitset<N> > {
        bool operator()(const std::bitset<N> x, const std::bitset<N> y) const {
            return x.count() < y.count();
        }
    };
    template <int N> struct less<std::bitset<N> > {
        bool operator()(const std::bitset<N> x, const std::bitset<N> y) const {
            return x.count() < y.count();
        }
    };
};

#include "serializer/log/garbage_collector.tcc"
#endif
