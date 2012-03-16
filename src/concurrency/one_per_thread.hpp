#ifndef CONCURRENCY_ONE_PER_THREAD_HPP_
#define CONCURRENCY_ONE_PER_THREAD_HPP_

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"

template<class inner_t>
class one_per_thread_t {
public:
    one_per_thread_t() : array(new boost::scoped_ptr<inner_t>[get_num_threads()]) {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::construct_on_thread_0, this, _1));
    }
    template<class arg1_t>
    explicit one_per_thread_t(const arg1_t &arg1) : array(new boost::scoped_ptr<inner_t>[get_num_threads()]) {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::construct_on_thread_1<arg1_t>, this, _1, arg1));
    }
    template<class arg1_t, class arg2_t>
    one_per_thread_t(const arg1_t &arg1, const arg2_t &arg2) : array(new boost::scoped_ptr<inner_t>[get_num_threads()]) {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::construct_on_thread_2<arg1_t, arg2_t>, this, _1, arg1, arg2));
    }
    ~one_per_thread_t() {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::destruct_on_thread, this, _1));
    }
    inner_t *get() {
        return array[get_thread_id()].get();
    }
private:
    void construct_on_thread_0(int thread) {
        on_thread_t thread_switcher(thread);
        array[thread].reset(new inner_t);
    }
    template<class arg1_t>
    void construct_on_thread_1(int thread, const arg1_t &arg1) {
        on_thread_t thread_switcher(thread);
        array[thread].reset(new inner_t(arg1));
    }
    template<class arg1_t, class arg2_t>
    void construct_on_thread_2(int thread, const arg1_t &arg1, const arg2_t &arg2) {
        on_thread_t thread_switcher(thread);
        array[thread].reset(new inner_t(arg1, arg2));
    }
    void destruct_on_thread(int thread) {
        on_thread_t thread_switcher(thread);
        array[thread].reset();
    }
    boost::scoped_array<boost::scoped_ptr<inner_t> > array;
};

#endif /* CONCURRENCY_ONE_PER_THREAD_HPP_ */
