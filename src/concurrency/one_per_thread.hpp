// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_ONE_PER_THREAD_HPP_
#define CONCURRENCY_ONE_PER_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"
#include "containers/scoped.hpp"
#include "containers/object_buffer.hpp"

template <class T>
class one_per_thread_t {
public:
    struct construct_0_t {
        one_per_thread_t *parent_;
        explicit construct_0_t(one_per_thread_t *p) : parent_(p) { }
        void operator()(int thread) const {
            on_thread_t th((threadnum_t(thread)));
            parent_->array[thread].init(new T());
        }
    };

    one_per_thread_t() : array(get_num_threads()) {
        pmap(get_num_threads(), construct_0_t(this));
    }

    template <class arg1_t>
    struct construct_1_t {
        one_per_thread_t *parent_;
        const arg1_t &arg1_;
        construct_1_t(one_per_thread_t *p, const arg1_t &arg1) : parent_(p), arg1_(arg1) { }

        void operator()(int thread) const {
            on_thread_t th((threadnum_t(thread)));
            parent_->array[thread].init(new T(arg1_));
        }
    };


    template<class arg1_t>
    explicit one_per_thread_t(const arg1_t &arg1) : array(get_num_threads()) {
        pmap(get_num_threads(), construct_1_t<arg1_t>(this, arg1));
    }

    template <class arg1_t, class arg2_t>
    struct construct_2_t {
        one_per_thread_t *parent_;
        const arg1_t &arg1_;
        const arg2_t &arg2_;
        construct_2_t(one_per_thread_t *p, const arg1_t &arg1, const arg2_t &arg2) : parent_(p), arg1_(arg1), arg2_(arg2) { }

        void operator()(int thread) const {
            on_thread_t th((threadnum_t(thread)));
            parent_->array[thread].init(new T(arg1_, arg2_));
        }
    };

    template<class arg1_t, class arg2_t>
    one_per_thread_t(const arg1_t &arg1, const arg2_t &arg2) : array(get_num_threads()) {
        pmap(get_num_threads(), construct_2_t<arg1_t, arg2_t>(this, arg1, arg2));
    }


    struct destruct_t {
        one_per_thread_t *parent_;
        explicit destruct_t(one_per_thread_t *p) : parent_(p) { }
        void operator()(int thread) const {
            on_thread_t th((threadnum_t(thread)));
            parent_->array[thread].reset();
        }
    };

    ~one_per_thread_t() {
        pmap(get_num_threads(), destruct_t(this));
    }

    T *get() {
        return array[get_thread_id().threadnum].get();
    }

private:
    // An array of size get_num_threads().  Pointees are allocated and destroyed on
    // their respective threads.  Because this is an array of pointers and not
    // object_buffer_t<T>, objects live on separate cache lines.
    scoped_array_t<scoped_ptr_t<T> > array;

    DISABLE_COPYING(one_per_thread_t);
};

#endif /* CONCURRENCY_ONE_PER_THREAD_HPP_ */

