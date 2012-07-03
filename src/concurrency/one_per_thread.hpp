#ifndef CONCURRENCY_ONE_PER_THREAD_HPP_
#define CONCURRENCY_ONE_PER_THREAD_HPP_

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"

template<class inner_t>
class one_per_thread_t {
public:
    struct construct_0_t {
        one_per_thread_t *parent_;
        construct_0_t(one_per_thread_t *p) : parent_(p) { }
        void operator()(int thread) const {
            on_thread_t th(thread);
            parent_->array[thread] = new inner_t;
        }
    };

    one_per_thread_t() : array(new inner_t *[get_num_threads()]) {
        pmap(get_num_threads(), construct_0_t(this));
    }

    template <class arg1_t>
    struct construct_1_t {
        one_per_thread_t *parent_;
        const arg1_t &arg1_;
        construct_1_t(one_per_thread_t *p, const arg1_t &arg1) : parent_(p), arg1_(arg1) { }

        void operator()(int thread) const {
            on_thread_t th(thread);
            parent_->array[thread] = new inner_t(arg1_);
        }
    };


    template<class arg1_t>
    explicit one_per_thread_t(const arg1_t &arg1) : array(new inner_t *[get_num_threads()]) {
        pmap(get_num_threads(), construct_1_t<arg1_t>(this, arg1));
    }

    template <class arg1_t, class arg2_t>
    struct construct_2_t {
        one_per_thread_t *parent_;
        const arg1_t &arg1_;
        const arg2_t &arg2_;
        construct_2_t(one_per_thread_t *p, const arg1_t &arg1, const arg2_t &arg2) : parent_(p), arg1_(arg1), arg2_(arg2) { }

        void operator()(int thread) const {
            on_thread_t th(thread);
            parent_->array[thread] = new inner_t(arg1_, arg2_);
        }
    };

    template<class arg1_t, class arg2_t>
    one_per_thread_t(const arg1_t &arg1, const arg2_t &arg2) : array(new inner_t *[get_num_threads()]) {
        pmap(get_num_threads(), construct_2_t<arg1_t, arg2_t>(this, arg1, arg2));
    }


    struct destruct_t {
        one_per_thread_t *parent_;
        destruct_t(one_per_thread_t *p) : parent_(p) { }
        void operator()(int thread) const {
            on_thread_t th(thread);
            delete parent_->array[thread];
            parent_->array[thread] = NULL;
        }
    };

    ~one_per_thread_t() {
        pmap(get_num_threads(), destruct_t(this));

        delete[] array;
    }

    inner_t *get() {
        return array[get_thread_id()];
    }

private:
    // An array of pointer-to-inner_t of size get_num_threads(), the
    // array is allocated and then the pointers are allocated (on
    // their respective threads) and both the pointees and arrays
    // owned by this object.
    inner_t **array;

    DISABLE_COPYING(one_per_thread_t);
};

#endif /* CONCURRENCY_ONE_PER_THREAD_HPP_ */
