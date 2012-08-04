#ifndef CONTAINERS_ITERATORS_HPP_
#define CONTAINERS_ITERATORS_HPP_

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <vector>
#include <utility>

#include "utils.hpp"
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "containers/scoped.hpp"


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
    virtual ~transform_iterator_t() { }

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
    scoped_ptr_t<one_way_iterator_t<T> > ownee;
};

template <class T>
struct filter_iterator_t : public one_way_iterator_t<T> {
    filter_iterator_t(const boost::function<bool(T&)>& _predicate, one_way_iterator_t<T> *_ownee) : predicate(_predicate), ownee(_ownee) { }
    virtual ~filter_iterator_t() { }

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
    scoped_ptr_t<one_way_iterator_t<T> > ownee;
};

template <typename F, typename S, typename Cmp = std::less<F> >
struct first_greater {
    typedef F first_argument_type;
    typedef S second_argument_type;
    bool operator()(std::pair<F, S> l, std::pair<F, S> r) {
        return Cmp()(r.first, l.first);
    }
};

template <typename T, typename Cmp = std::less<T> >
class merge_ordered_data_iterator_t : public one_way_iterator_t<T> {
public:
    typedef boost::shared_ptr<one_way_iterator_t<T> > mergee_t;



    merge_ordered_data_iterator_t() : mergees(), next_to_pop_from(),
        merge_heap(first_greater<T, mergee_t, Cmp>(), std::vector<std::pair<T, mergee_t> >()) { }

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
            for (typename std::set<mergee_t>::iterator it = mergees.begin(); it != mergees.end();) {
                mergee_t mergee = *it;
                typename boost::optional<T> mergee_next = mergee->next();
                if (mergee_next) {
                    merge_heap.push(std::make_pair(mergee_next.get(), mergee));
                    ++it;
                } else {
                    mergee.reset();          // nothing more in this iterator, delete it
                    typename std::set<mergee_t>::iterator old_it = it;
                    ++it;
                    mergees.erase(old_it);    // Note: this increments the iterator, erases the
                                              // element at the old iterator position, and then
                                              // updates the 'it' value
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

        std::pair<T, mergee_t> top = merge_heap.top();
        merge_heap.pop();
        next_to_pop_from = top.second;

        // issue the async prefetch, so that we don't need to block on next has_next/next call
        next_to_pop_from->prefetch();
        return boost::make_optional(top.first);
    }
    void prefetch() {
        typename std::set<mergee_t>::iterator it;
        for (it = mergees.begin(); it != mergees.end(); ++it) {
            (*it)->prefetch();
        }
    }
private:
    void done() {
        mergees.clear();
    }
    std::set<mergee_t> mergees;
    mergee_t next_to_pop_from;
    typename std::priority_queue<std::pair<T, mergee_t>, std::vector<std::pair<T, mergee_t> >, first_greater<T, mergee_t, Cmp> > merge_heap;

};

/*
unique_iterator_t removes duplicates. It requires the input iterator to be sorted.
T has to implement operator==().

Note: Union of sets can be implemented as a unque iterator around a merge iterator.
*/
template <class T>
class unique_filter_iterator_t : public one_way_iterator_t<T> {
public:
    explicit unique_filter_iterator_t(one_way_iterator_t<T> *_ownee) : previous(boost::none), ownee(_ownee) { }
    virtual ~unique_filter_iterator_t() {
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
    scoped_ptr_t<one_way_iterator_t<T> > ownee;
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
    virtual ~repetition_filter_iterator_t() { }

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
    scoped_ptr_t<one_way_iterator_t<T> > ownee;
    int n_repetitions;
};


#endif /* CONTAINERS_ITERATORS_HPP_ */
