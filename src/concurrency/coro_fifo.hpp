#ifndef __CONCURRENCY_CORO_FIFO_HPP__
#define __CONCURRENCY_CORO_FIFO_HPP__

#include "utils.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

// A coro_fifo_t is an object that people on the same thread can
// acquire and release.  Multiple people can acquire it at the same
// time, but they have to release it in the same order in which they
// acquire it.  This could, for example, prevent sets from hopping
// over one another.

// This must run on the same thread, which limits its usefulness.

class coro_fifo_acq_t;

class coro_fifo_t : public home_thread_mixin_t {
public:
    coro_fifo_t() { }

    friend class coro_fifo_acq_t;

private:
    void inform_ready_to_exit(coro_fifo_acq_t *acq);

    // The acquisitor_queue_ is either empty, or it begins with a
    // coro_fifo_acq_t for which is_waiting_to_exit_ is false.  When a
    // coro_fifo_acq_t is ready to die, it tells us so.  Then we set
    // its is_waiting_to_exit_ to true and pulse the contiguous group
    // of coro_fifo_acq_t's at the front of the queue that have
    // is_waiting_to_exit_ set to true.  This lets us shove stuff out
    // of the acquisitor_queue_ without waiting for another turn of
    // the event queue for every item we could remove.
    intrusive_list_t<coro_fifo_acq_t> acquisitor_queue_;

    DISABLE_COPYING(coro_fifo_t);
};

class coro_fifo_acq_t : public intrusive_list_node_t<coro_fifo_acq_t> {
public:
    friend class coro_fifo_t;

    explicit coro_fifo_acq_t(coro_fifo_t *fifo) : fifo_(fifo), is_waiting_to_exit_(false) {
        fifo_->assert_thread();
        fifo_->acquisitor_queue_.push_back(this);
    }

    ~coro_fifo_acq_t() {
        fifo_->inform_ready_to_exit(this);
        ready_to_exit_.wait();
        rassert(!in_a_list, "should have been removed from the acquisitor_queue_");
    }

private:
    coro_fifo_t *fifo_;
    cond_t ready_to_exit_;
    bool is_waiting_to_exit_;


    DISABLE_COPYING(coro_fifo_acq_t);
};


#endif  // __CONCURRENCY_CORO_FIFO_HPP__
