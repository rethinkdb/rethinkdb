#ifndef __CONTAINERS_ITERATOR_HPP__
#define __CONTAINERS_ITERATOR_HPP__

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <vector>

#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>


template <typename T>
struct one_way_iterator_t {
    virtual ~one_way_iterator_t() { }
    virtual typename boost::optional<T> next() = 0; // next() can block until it can read the next value
    virtual void prefetch() = 0;    // Fetch all the necessary data to be able to give the next value without blocking.
                                    // prefetch() is assumed to be asynchronous. Thus if next() is called before the data
                                    // is available, next() can still block.
};

template <class T, class U>
struct transform_iterator_t : public one_way_iterator_t<U> {
    transform_iterator_t(const boost::function<U(T&)>& _func, one_way_iterator_t<T> *_ownee) : func(_func), ownee(_ownee) { }
    ~transform_iterator_t() {
        delete ownee;
    }
    virtual typename boost::optional<U> next() {
        boost::optional<T> value = ownee->next();
        if (!value) {
            return boost::none;
        } else {
            return boost::make_optional(func(value.get()));
        }
    }
    void prefetch() {
        ownee->prefetch();
    }

    boost::function<U(T&)> func;
    one_way_iterator_t<T> *ownee;
};

template <class T>
struct filter_iterator_t : public one_way_iterator_t<T> {
    filter_iterator_t(const boost::function<bool(T&)>& _predicate, one_way_iterator_t<T> *_ownee) : predicate(_predicate), ownee(_ownee) { }
    ~filter_iterator_t() {
        delete ownee;
    }
    virtual typename boost::optional<T> next() {
        for (;;) {
            boost::optional<T> value = ownee->next();
            if (!value) {
                return boost::none;
            } else {
                if (predicate(value.get())) {
                    return value;
                }
                // otherwise, continue.
            }
        }
    }
    void prefetch() {
        // Unfortunately we don't feel like really implementing this
        // function right now.  It would require running the filter
        // operation on return values, consuming ahead into the
        // iterator.  Also some other one_way_iterator_t
        // implementations blow this off, so why not us.
        ownee->prefetch();
    }

    boost::function<bool(T&)> predicate;
    one_way_iterator_t<T> *ownee;
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
                mergees.erase(next_to_pop_from);
                next_to_pop_from.reset();    // relinquish our hold
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

/*
unique_iterator_t removes duplicates. It requires the input iterator to be sorted.
T has to implement operator==().

Note: Union of sets can be implemented as a unque iterator around a merge iterator.
*/
template <class T>
class unique_filter_iterator_t : public one_way_iterator_t<T> {
public:
    unique_filter_iterator_t(one_way_iterator_t<T> *_ownee) : previous(boost::none), ownee(_ownee) { }
    ~unique_filter_iterator_t() {
        delete ownee;
    }
    virtual typename boost::optional<T> next() {
        for (;;) {
            boost::optional<T> value = ownee->next();
            if (!value) {
                previous = value;
                return boost::none;
            } else {
                // Is this element different from what we had before?
                if (!previous || !(previous.get() == value.get())) {
                    previous = value;
                    return value;
                }
                // otherwise, continue.
            }
        }
    }
    void prefetch() {
        // TODO: Should we implement this?
        ownee->prefetch();
    }

private:
    boost::optional<T> previous;
    one_way_iterator_t<T> *ownee;
};

/* repetition_filter_iterator_t produces an element whenever it was repeated
n_repetitions times in the input iterator. T has to implement operator==().
If a element is repeated more than n_repetitions times, repetition_filter_iterator_t
will output the element once per n_repetitions repetitions.

Note: Intersection of sets can be implemented as a repetition iterator around a merge iterator,
 where n_repetitions must be set to the number of input sets. This works as
 long as the input iterators are sorted and don't produce any element more than once.
 */
template <class T>
class repetition_filter_iterator_t : public one_way_iterator_t<T> {
public:
    repetition_filter_iterator_t(one_way_iterator_t<T> *_ownee, int _n_repetitions) : previous(boost::none), previous_repetitions(0), ownee(_ownee), n_repetitions(_n_repetitions) {
        rassert(n_repetitions > 0);
    }
    ~repetition_filter_iterator_t() {
        delete ownee;
    }
    virtual typename boost::optional<T> next() {
        for (;;) {
            boost::optional<T> value = ownee->next();
            if (!value) {
                previous = value;
                previous_repetitions = 0;
                return boost::none;
            } else {
                // Is this element different from what we had before?
                if (!previous || !(previous.get() == value.get())) {
                    // Different, start over
                    previous_repetitions = 1;
                    previous = value;
                } else {
                    // The same, increment counter
                    ++previous_repetitions;
                }

                // Have we got enough repetitions?
                rassert(previous_repetitions <= n_repetitions);
                if (previous_repetitions == n_repetitions) {
                    previous_repetitions = 0;
                    return value;
                }
                // otherwise, continue.
            }
        }
    }
    void prefetch() {
        // TODO: Should we implement this?
        ownee->prefetch();
    }

private:
    boost::optional<T> previous;
    int previous_repetitions;
    one_way_iterator_t<T> *ownee;
    int n_repetitions;
};

/*
 diff_filter_iterator_t implements set difference semantics. It's output
 are all the keys from ownee_left that are not in ownee_right, assuming
 that they are both sorted. T has to implement operator==() and operator<().
*/
// TODO/WARNING: As of Jul 28th, this has not been thoroughly tested. (daniel)
//  If you use this and stuff fails, consider diff_filter_iterator_t to be potentially faulty.
template <class T>
class diff_filter_iterator_t : public one_way_iterator_t<T> {
public:
    diff_filter_iterator_t(one_way_iterator_t<T> *_ownee_left, one_way_iterator_t<T> *_ownee_right) : ownee_left(_ownee_left), ownee_right(_ownee_right), prefetched_right(boost::none) { }
    ~diff_filter_iterator_t() {
        delete ownee_left;
        delete ownee_right;
    }
    virtual typename boost::optional<T> next() {
        for (;;) {
            boost::optional<T> value = ownee_left->next();
            if (!value) {
                return boost::none;
            } else {
                // Is this element the same as prefetched_right?
                if (prefetched_right && prefetched_right.get() == value.get()) {
                    // It is, skip value.
                } else {
                    // Fetch new elements from ownee_right until they pass value
                    if (!prefetched_right || prefetched_right.get() < value.get()) {
                        do {
                            prefetched_right = ownee_right->next();
                        } while (prefetched_right && prefetched_right.get() < value.get());
                    }

                    // Now check again...
                    if (prefetched_right && prefetched_right.get() == value.get()) {
                        // Ok, it's the same as value. Skip value.
                    } else {
                        // no element like value in prefetched_right, return value
                        return value;
                    }
                }
            }
        }
    }
    void prefetch() {
        // TODO: Should we implement this?
        ownee_left->prefetch();
        ownee_right->prefetch();
    }

private:
    one_way_iterator_t<T> *ownee_left;
    one_way_iterator_t<T> *ownee_right;
    boost::optional<T> prefetched_right;
};

#endif /* __CONTAINERS_ITERATOR_HPP__ */
