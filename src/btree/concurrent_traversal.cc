#include "btree/concurrent_traversal.hpp"

class concurrent_traversal_adapter_t : public depth_first_traversal_callback_t {
public:
    concurrent_traversal_adapter_t(concurrent_traversal_callback_t *cb) : cb_(cb) { }

    virtual bool handle_pair(dft_value_t &&keyvalue) {
        cond_t dummy_exclusive_signal;
        dummy_exclusive_signal.pulse();
        cond_t dummy_interruptor;
        try {
            return cb_->handle_pair(std::move(keyvalue), &dummy_exclusive_signal,
                                    &dummy_interruptor);
        } catch (const interrupted_exc_t &) {
            return false;
        }
    }

private:
    concurrent_traversal_callback_t *cb_;
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
