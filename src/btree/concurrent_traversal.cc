#include "btree/concurrent_traversal.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer.hpp"

class incr_decr_t {
public:
    explicit incr_decr_t(size_t *ptr) : ptr_(ptr) {
        guarantee(*ptr_ < INT_MAX);
        ++*ptr_;
    }
    ~incr_decr_t() {
        --*ptr_;
    }

private:
    size_t *ptr_;

    DISABLE_COPYING(incr_decr_t);
};


namespace concurrent_traversal {
static const int initial_semaphore_capacity = 3;
static const int min_semaphore_capacity = 2;
static const int max_semaphore_capacity = 30;
}  // namespace concurrent_traversal

class concurrent_traversal_adapter_t : public depth_first_traversal_callback_t {
public:

    explicit concurrent_traversal_adapter_t(concurrent_traversal_callback_t *cb,
                                            cond_t *failure_cond)
        : semaphore_(concurrent_traversal::initial_semaphore_capacity, 0.5),
          sink_waiters_(0),
          cb_(cb),
          failure_cond_(failure_cond) { }

    void handle_pair_coro(scoped_key_value_t *fragile_keyvalue,
                          adjustable_semaphore_acq_t *fragile_acq,
                          fifo_enforcer_write_token_t token,
                          auto_drainer_t::lock_t) {
        // This is called by coro_t::spawn_now_dangerously. We need to get these
        // values before the caller's stack frame is destroyed.
        scoped_key_value_t keyvalue = std::move(*fragile_keyvalue);

        adjustable_semaphore_acq_t semaphore_acq(std::move(*fragile_acq));

        fifo_enforcer_sink_t::exit_write_t exit_write(&sink_, token);

        bool success;
        try {
            success = cb_->handle_pair(std::move(keyvalue),
                                       concurrent_traversal_fifo_enforcer_signal_t(&exit_write,
                                                                     this));
        } catch (const interrupted_exc_t &) {
            success = false;
        }

        if (!success) {
            failure_cond_->pulse_if_not_already_pulsed();
        }
    }

    virtual bool handle_pair(scoped_key_value_t &&keyvalue) {
        // First thing first: Get in line with the token enforcer.

        fifo_enforcer_write_token_t token = source_.enter_write();
        // ... and wait for the semaphore, we don't want too many things loading
        // values at once.
        adjustable_semaphore_acq_t acq(&semaphore_);

        coro_t::spawn_now_dangerously(
            std::bind(&concurrent_traversal_adapter_t::handle_pair_coro,
                      this, &keyvalue, &acq, token, auto_drainer_t::lock_t(&drainer_)));

        // Report if we've failed by the time this handle_pair call is called.
        return !failure_cond_->is_pulsed();
    }

private:
    friend class concurrent_traversal_fifo_enforcer_signal_t;

    adjustable_semaphore_t semaphore_;

    fifo_enforcer_source_t source_;
    fifo_enforcer_sink_t sink_;

    // The number of coroutines waiting for their turn with sink_.
    size_t sink_waiters_;

    concurrent_traversal_callback_t *cb_;

    // Signals when the query has failed, when we should give up all hope in executing
    // the query.
    cond_t *failure_cond_;

    // We don't use the drainer's drain signal, we use failure_cond_
    auto_drainer_t drainer_;
    DISABLE_COPYING(concurrent_traversal_adapter_t);
};

concurrent_traversal_fifo_enforcer_signal_t::concurrent_traversal_fifo_enforcer_signal_t(
        signal_t *eval_exclusivity_signal,
        concurrent_traversal_adapter_t *parent)
    : eval_exclusivity_signal_(eval_exclusivity_signal),
      parent_(parent) { }

void concurrent_traversal_fifo_enforcer_signal_t::wait_interruptible() THROWS_ONLY(interrupted_exc_t) {
    incr_decr_t incr_decr(&parent_->sink_waiters_);

    if (parent_->sink_waiters_ >= 2) {
        // If we have two or more things waiting for the signal (including ourselves)
        // we're probably looking too far ahead.  Lower the capacity by 1.  This might
        // seem abrupt (especially considering that our semapoher_.get_capacity()
        // concurrent reads can finish in any order), but we have the trickle fraction
        // set to 0.5, so there's smoothing.
        parent_->semaphore_.set_capacity(std::max(concurrent_traversal::min_semaphore_capacity,
                                                  parent_->semaphore_.get_capacity() - 1));
    } else if (parent_->sink_waiters_ == 1) {
        // We're the only thing waiting for the signal?  We might not be looking far
        // ahead enough.
        parent_->semaphore_.set_capacity(std::min(concurrent_traversal::max_semaphore_capacity,
                                                  parent_->semaphore_.get_capacity() + 1));
    }

    ::wait_interruptible(eval_exclusivity_signal_, parent_->failure_cond_);
}

bool btree_concurrent_traversal(btree_slice_t *slice, transaction_t *transaction,
                                superblock_t *superblock, const key_range_t &range,
                                concurrent_traversal_callback_t *cb,
                                direction_t direction) {
    cond_t failure_cond;
    bool failure_seen;
    {
        concurrent_traversal_adapter_t adapter(cb, &failure_cond);
        failure_seen = !btree_depth_first_traversal(slice, transaction, superblock,
                                                    range, &adapter, direction);
    }
    // Now that adapter is destroyed, the operations that might have failed have all
    // drained.  (If we fail, we try to report it to btree_depth_first_traversal (to
    // kill the traversal), but it's possible for us to fail after
    // btree_depth_first_traversal returns.)
    guarantee(!(failure_seen && !failure_cond.is_pulsed()));
    return !failure_cond.is_pulsed();
}
