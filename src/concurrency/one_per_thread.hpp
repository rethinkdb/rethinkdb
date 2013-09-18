// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ONE_PER_THREAD_HPP_
#define CONCURRENCY_ONE_PER_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "containers/scoped.hpp"

template <class T>
class one_per_thread_t {
public:
    one_per_thread_t() : array(get_num_threads()) {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::construct_0, this, ph::_1));
    }

    template <class arg1_t>
    explicit one_per_thread_t(const arg1_t &arg1) : array(get_num_threads()) {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::construct_1<arg1_t>,
                                          this, ph::_1, std::ref(arg1)));
    }

    template<class arg1_t, class arg2_t>
    one_per_thread_t(const arg1_t &arg1, const arg2_t &arg2) : array(get_num_threads()) {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::construct_2<arg1_t, arg2_t>,
                                          this, ph::_1, std::ref(arg1), std::ref(arg2)));
    }

    ~one_per_thread_t() {
        pmap(get_num_threads(), std::bind(&one_per_thread_t::destruct, this, ph::_1));
    }

    T *get() {
        return array[get_thread_id().threadnum].get();
    }

private:
    void construct_0(int thread) {
        on_thread_t th((threadnum_t(thread)));
        array[thread].init(new T());
    }

    template <class arg1_t>
    void construct_1(int thread, const arg1_t &arg1) {
        on_thread_t th((threadnum_t(thread)));
        array[thread].init(new T(arg1));
    }

    template <class arg1_t, class arg2_t>
    void construct_2(int thread, const arg1_t &arg1, const arg2_t &arg2) {
        on_thread_t th((threadnum_t(thread)));
        array[thread].init(new T(arg1, arg2));
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

