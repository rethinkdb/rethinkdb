#ifndef __BTREE_RGET_INTERNAL_HPP__
#define __BTREE_RGET_INTERNAL_HPP__

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <queue>
#include "errors.hpp"

/* This file is separate from rget.{cc,hpp} because we need to test the
 * iterator merging, but we don't really want to expose the types outside of
 * rget.cc for now.
 */

template <typename T, typename Cmp = std::less<T> >
struct ordered_data_iterator {
    virtual bool has_next() = 0;
    virtual T next() = 0;           // next can block until it can read the next value
    virtual void prefetch() = 0;    // fetch all the necessary data to be able to give the next value without blocking.
                                    // prefetch() is assumed to be asynchronous. Thus if next/has_next is called before
                                    // the data is available, they can block.
};

template <typename F, typename S, typename Cmp = std::less<F> >
struct first_not_less {
    typedef F first_argument_type;
    typedef S second_argument_type;
    bool operator()(std::pair<F,S> l, std::pair<F,S> r) {
        return !Cmp()(l.first, r.first);
    }
};

template <typename T, typename Cmp = std::less<T> >
struct merge_ordered_data_iterator : public ordered_data_iterator<T, Cmp> {
    typedef ordered_data_iterator<T, Cmp> mergee_t;
    typedef std::vector<mergee_t*> mergees_t;

    typedef std::pair<T, mergee_t*> heap_elem_t;
    typedef std::vector<heap_elem_t> heap_container_t;

    merge_ordered_data_iterator(const mergees_t& mergees)
        : mergees(mergees), next_to_pop_from(NULL), merge_heap(first_not_less<T, mergee_t*>(), heap_container_t()) { }

    T next() {
        // if we are getting the first element, we have to request the data from all of the mergees
        if (next_to_pop_from == NULL) {
            prefetch();
            for (typename mergees_t::iterator it = mergees.begin(); it != mergees.end(); ++it) {
                mergee_t *mergee = *it;
                if (mergee->has_next()) {
                    merge_heap.push(std::make_pair(mergee->next(), mergee));
                }
            }
        } else {
            if (next_to_pop_from->has_next()) {
                merge_heap.push(std::make_pair(next_to_pop_from->next(), next_to_pop_from));
            }
        }
        guarantee(merge_heap.size() > 0);

        heap_elem_t top = merge_heap.top();
        merge_heap.pop();
        next_to_pop_from = top.second;
        next_to_pop_from->prefetch();   // issue the async prefetch, so that we don't need to block on next has_next/next call
        return top.first;
    }
    bool has_next() {
        if (next_to_pop_from == NULL) {
            prefetch();
            for (typename mergees_t::iterator it = mergees.begin(); it != mergees.end(); ++it) {
                mergee_t *mergee = *it;
                if (mergee->has_next())
                    return true;
            }
            return false;
        } else {
            return merge_heap.size() > 0 || next_to_pop_from->has_next();
        }
    }
    void prefetch() {
        std::for_each(mergees.begin(), mergees.end(), std::mem_fun(&mergee_t::prefetch));
    }
private:
    mergees_t mergees;
    mergee_t *next_to_pop_from;
    typename std::priority_queue<heap_elem_t, heap_container_t, first_not_less<T, mergee_t*> > merge_heap;

};

#endif // __BTREE_RGET_INTERNAL_HPP__
