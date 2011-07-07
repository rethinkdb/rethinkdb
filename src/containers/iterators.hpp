#ifndef __CONTAINERS_ITERATOR_HPP__
#define __CONTAINERS_ITERATOR_HPP__

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "utils2.hpp"

template <typename T>
struct one_way_iterator_t {
    virtual ~one_way_iterator_t() { }
    virtual typename boost::optional<T> next() = 0; // next() can block until it can read the next value
    virtual void prefetch() = 0;    // Fetch all the necessary data to be able to give the next value without blocking.
                                    // prefetch() is assumed to be asynchronous. Thus if next() is called before the data
                                    // is available, next() can still block.
};

template <typename F, typename S, typename Cmp = std::less<F> >
struct first_greater {
    typedef F first_argument_type;
    typedef S second_argument_type;
    bool operator()(std::pair<F,S> l, std::pair<F,S> r) {
        return Cmp()(r.first, l.first);
    }
};

template <typename T, typename Cmp = std::less<T> >
class merge_ordered_data_iterator_t : public one_way_iterator_t<T> {
public:
    typedef boost::shared_ptr<one_way_iterator_t<T> > mergee_t;
    typedef std::set<mergee_t> mergees_t;

    typedef std::pair<T, mergee_t> heap_elem_t;
    typedef std::vector<heap_elem_t> heap_container_t;


    merge_ordered_data_iterator_t() : mergees(), next_to_pop_from(),
        merge_heap(first_greater<T, mergee_t, Cmp>(), heap_container_t()) { }

    virtual ~merge_ordered_data_iterator_t() {
        done();
    }

    void add_mergee(mergee_t mergee) {
        rassert(!next_to_pop_from.get());
        mergees.insert(mergee);
    }

    void add_mergee(one_way_iterator_t<T> *mergee) {
        boost::shared_ptr<one_way_iterator_t<T> > _mergee(mergee);
        add_mergee(_mergee);
    }

    typename boost::optional<T> next() {
        // if we are getting the first element, we have to request the data from all of the mergees
        if (!next_to_pop_from.get()) {
            prefetch();
            for (typename mergees_t::iterator it = mergees.begin(); it != mergees.end();) {
                mergee_t mergee = *it;
                typename boost::optional<T> mergee_next = mergee->next();
                if (mergee_next) {
                    merge_heap.push(std::make_pair(mergee_next.get(), mergee));
                    it++;
                } else {
                    mergee.reset();          // nothing more in this iterator, delete it
                    mergees.erase(it++);    // Note: this increments the iterator, erases the element
                                            // at the old iterator position, and then updates the 'it' value
                }
            }
        } else {
            typename boost::optional<T> next_val = next_to_pop_from->next();
            if (next_val) {
                merge_heap.push(std::make_pair(next_val.get(), next_to_pop_from));
            } else {
                next_to_pop_from.reset();    // relinquish our hold
                mergees.erase(next_to_pop_from);
            }
        }
        if (merge_heap.size() == 0) {
            done();
            return boost::none;
        }

        heap_elem_t top = merge_heap.top();
        merge_heap.pop();
        next_to_pop_from = top.second;

        // issue the async prefetch, so that we don't need to block on next has_next/next call
        next_to_pop_from->prefetch();
        return boost::make_optional(top.first);
    }
    void prefetch() {
        typename mergees_t::iterator it;
        for (it = mergees.begin(); it != mergees.end(); it++) {
            (*it)->prefetch();
        }
    }
private:
    void done() {
        mergees.clear();
    }
    mergees_t mergees;
    mergee_t next_to_pop_from;
    typename std::priority_queue<heap_elem_t, heap_container_t, first_greater<T, mergee_t, Cmp> > merge_heap;

};

#endif /* __CONTAINERS_ITERATOR_HPP__ */
