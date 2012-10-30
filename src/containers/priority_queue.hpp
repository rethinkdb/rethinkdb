// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_PRIORITY_QUEUE_HPP_
#define CONTAINERS_PRIORITY_QUEUE_HPP_

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
            /* \brief update() should be called after a change is made to the data
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
    inline void swap_entries(unsigned int, unsigned int);
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

private:
    DISABLE_COPYING(priority_queue_t);
};

#include "containers/priority_queue.tcc"
#endif /* CONTAINERS_PRIORITY_QUEUE_HPP_ */
