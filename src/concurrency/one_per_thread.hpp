#ifndef __CONCURRENCY_ONE_PER_THREAD_HPP__
#define __CONCURRENCY_ONE_PER_THREAD_HPP__

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include "arch/runtime/runtime.hpp"

template<class inner_t>
class one_per_thread_t {
public:
    one_per_thread_t() : array(new boost::scoped_ptr<inner_t>[get_num_threads()]) {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::construct_on_thread, this, _1));
    }
    ~one_per_thread_t() {
        pmap(get_num_threads(), boost::bind(&one_per_thread_t::destruct_on_thread, this, _1));
    }
    inner_t *get() {
        return array[get_thread_id()].get();
    }
private:
    void construct_on_thread(int thread) {
        on_thread_t thread_switcher(thread);
        array[thread].reset(new inner_t);
    }
    void destruct_on_thread(int thread) {
        on_thread_t thread_switcher(thread);
        array[thread].reset();
    }
    boost::scoped_array<boost::scoped_ptr<inner_t> > array;
};

#endif /* __CONCURRENCY_ONE_PER_THREAD_HPP__ */
