#ifndef BTREE_CONCURRENT_TRAVERSAL_HPP_
#define BTREE_CONCURRENT_TRAVERSAL_HPP_

#include "btree/depth_first_traversal.hpp"

class concurrent_traversal_callback_t {
public:
    concurrent_traversal_callback_t() { }

    virtual bool handle_pair(dft_value_t &&keyvalue,
                             signal_t *eval_exclusivity_signal,
                             signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

protected:
    virtual ~concurrent_traversal_callback_t() { }
private:
    DISABLE_COPYING(concurrent_traversal_callback_t);
};

bool btree_concurrent_traversal(btree_slice_t *slice, transaction_t *transaction,
                                superblock_t *superblock, const key_range_t &range,
                                concurrent_traversal_callback_t *cb,
                                direction_t direction);



#endif  // BTREE_CONCURRENT_TRAVERSAL_HPP_
