#include "btree/concurrent_traversal.hpp"

#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer.hpp"

class concurrent_traversal_adapter_t : public depth_first_traversal_callback_t {
public:
    static const int semaphore_capacity = 10;

    concurrent_traversal_adapter_t(concurrent_traversal_callback_t *cb)
        : semaphore_(semaphore_capacity),
          cb_(cb) { }

    void handle_pair_coro(dft_value_t *fragile_keyvalue, semaphore_acq_t *fragile_acq,
                          fifo_enforcer_write_token_t token,
                          auto_drainer_t::lock_t) {
        // This is called by coro_t::spawn_now_dangerously. We need to get these
        // values before the caller's stack frame is destroyed.
        dft_value_t keyvalue = std::move(*fragile_keyvalue);

        semaphore_acq_t semaphore_acq(std::move(*fragile_acq));

        fifo_enforcer_sink_t::exit_write_t exit_write(&sink_, token);

        bool success;
        try {
            success = cb_->handle_pair(std::move(keyvalue), &exit_write,
                                       &failure_cond_);
        } catch (const interrupted_exc_t &) {
            success = false;
        }

        if (!success) {
            failure_cond_.pulse_if_not_already_pulsed();
        }
    }

    virtual bool handle_pair(dft_value_t &&keyvalue) {
        // First thing first: Get in line with the token enforcer.

        fifo_enforcer_write_token_t token = source_.enter_write();
        // ... and wait for the semaphore, we don't want too many things loading
        // values at once.
        semaphore_acq_t acq(&semaphore_);

        coro_t::spawn_now_dangerously(
            std::bind(&concurrent_traversal_adapter_t::handle_pair_coro,
                      this, &keyvalue, &acq, token, auto_drainer_t::lock_t(&drainer_)));

        // Report if we've failed by the time this handle_pair call is called.
        return !failure_cond_.is_pulsed();
    }

private:
    semaphore_t semaphore_;

    fifo_enforcer_source_t source_;
    fifo_enforcer_sink_t sink_;

    concurrent_traversal_callback_t *cb_;

    // Signals when the query has failed, when we should give up all hope in executing
    // the query.
    cond_t failure_cond_;

    // We don't use the drainer's drain signal, we use failure_cond_
    auto_drainer_t drainer_;
    DISABLE_COPYING(concurrent_traversal_adapter_t);
};

bool btree_concurrent_traversal(btree_slice_t *slice, transaction_t *transaction,
                                superblock_t *superblock, const key_range_t &range,
                                concurrent_traversal_callback_t *cb,
                                direction_t direction) {
    concurrent_traversal_adapter_t adapter(cb);
    return btree_depth_first_traversal(slice, transaction, superblock, range, &adapter,
                                       direction);
}
