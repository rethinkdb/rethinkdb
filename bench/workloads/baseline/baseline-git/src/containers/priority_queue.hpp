
#ifndef __PRIORITY_QUEUE_HPP__
#define __PRIORITY_QUEUE_HPP__

#include <bitset>
#include <deque>
#include <functional>
#include "utils.hpp"

/* \brief priority_queue_t
 * Priority queues are by defined to be max priority queues, that is
 * pop() gives you an element a such that for all b in the heap Less(b, a) == True
 */
template<class T, class Less = std::less<T> >
class priority_queue_t {
public:
    struct entry_t {
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
            bool operator< (const entry_t &b) {return Less(b.data, data);}
            entry_t()
                : pq(0), index(0)
                {}
    };
private:
    std::deque<entry_t *> heap;
private:
    inline unsigned int parent(unsigned int);
    inline unsigned int left(unsigned int);
    inline unsigned int right(unsigned int);
    inline void swap(unsigned int, unsigned int);
    inline void bubble_up(int *);
    inline void bubble_up(int);
    inline void bubble_down(int *);
    inline void bubble_down(int);
public:
    priority_queue_t();
    ~priority_queue_t();
    bool empty();
    size_t size();
    T peak();
    entry_t *push(T);
    void remove(entry_t *);
    T pop();
    void update(int);
public:
    void validate();
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

#include "priority_queue.tcc"
#endif
