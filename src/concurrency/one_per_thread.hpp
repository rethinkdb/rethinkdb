// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ONE_PER_THREAD_HPP_
#define CONCURRENCY_ONE_PER_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "containers/scoped.hpp"

template <class T>
class one_per_thread_t {
public:
    template <class... Args>
    one_per_thread_t(const Args &... args) : array(get_num_threads()) {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::construct<Args...>,
                                          this, _1, std::ref(args)...));
    }

    ~one_per_thread_t() {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::destruct, this, _1));
    }

    T *get() {
        return array[get_thread_id().threadnum].get();
    }

private:
    template <class... Args>
    void construct(int thread, const Args &... args) {
        on_thread_t th((threadnum_t(thread)));
        array[thread].init(new T(args...));
    }

    void destruct(int thread) {
        on_thread_t th((threadnum_t(thread)));
        array[thread].reset();
    }

    // An array of size get_num_threads().  Pointees are allocated and destroyed on
    // their respective threads.  Because this is an array of pointers and not
    // object_buffer_t<T>, objects live on separate cache lines.
    scoped_array_t<scoped_ptr_t<T> > array;

    DISABLE_COPYING(one_per_thread_t);
};

#endif /* CONCURRENCY_ONE_PER_THREAD_HPP_ */

