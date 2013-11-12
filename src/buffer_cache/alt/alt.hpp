#ifndef BUFFER_CACHE_ALT_ALT_HPP_
#define BUFFER_CACHE_ALT_ALT_HPP_

#include <vector>

#include "buffer_cache/alt/page.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

class auto_drainer_t;
class serializer_t;

namespace alt {

class alt_buf_lock_t;

class alt_cache_t {
public:
    explicit alt_cache_t(serializer_t *serializer);
    ~alt_cache_t();

    block_size_t max_block_size() const;

    page_cache_t page_cache_;

private:
    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(alt_cache_t);
};

class alt_txn_t {
public:
    alt_txn_t(alt_cache_t *cache, alt_txn_t *preceding_txn = NULL);
    ~alt_txn_t();

    alt_cache_t *cache() { return cache_; }

    page_txn_t *page_txn() { return &page_txn_; }

private:
    alt_cache_t *cache_;
    page_txn_t page_txn_;
    // RSI: Is this_txn_timestamp_ used?  How?
    repli_timestamp_t this_txn_timestamp_;

    DISABLE_COPYING(alt_txn_t);
};

class alt_snapshot_node_t : public single_threaded_countable_t<alt_snapshot_node_t> {
public:
    alt_snapshot_node_t();
    ~alt_snapshot_node_t();

private:
    page_t *page_;

    std::vector<alt_snapshot_node_t *> children_;
    DISABLE_COPYING(alt_snapshot_node_t);
};

class alt_buf_lock_t {
public:
    alt_buf_lock_t();

    // Nonblocking constructor.
    alt_buf_lock_t(alt_txn_t *txn,
                   block_id_t block_id,
                   alt_access_t access);
    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  In either case, returns before the block is acquired, but after we're
    // _in line_ for the block.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   block_id_t block_id,
                   alt_access_t access);

    // Nonblocking constructor that acquires a block with a new block id.  (RSI: Is
    // this useful for _anything_?  We refer to the superblock by name.)  `access`
    // must be `write`.
    alt_buf_lock_t(alt_txn_t *txn,
                   alt_access_t access);

    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  Allocates a block with a new block id.  `access` must be `write`.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   alt_access_t access);



    ~alt_buf_lock_t();

    void swap(alt_buf_lock_t &other);

    void snapshot_subtree();

    block_id_t block_id() const {
        guarantee(txn_ != NULL);
        return current_page_acq_->block_id();
    }
    alt_access_t access() const {
        guarantee(txn_ != NULL);
        return current_page_acq_->access();
    }

    signal_t *read_acq_signal() {
        guarantee(txn_ != NULL);
        return current_page_acq_->read_acq_signal();
    }
    signal_t *write_acq_signal() {
        guarantee(txn_ != NULL);
        return current_page_acq_->write_acq_signal();
    }

    void mark_deleted() {
        guarantee(txn_ != NULL);
        current_page_acq_->mark_deleted();
    }

private:
    friend class alt_buf_read_t;
    friend class alt_buf_write_t;

    alt_txn_t *txn_;
    alt_cache_t *cache_;

    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    alt_snapshot_node_t *snapshot_node_;

    DISABLE_COPYING(alt_buf_lock_t);
};


class alt_buf_read_t {
public:
    explicit alt_buf_read_t(alt_buf_lock_t *lock);
    ~alt_buf_read_t();

    const void *get_data_read(uint32_t *block_size_out);

private:
    alt_buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(alt_buf_read_t);
};

class alt_buf_write_t {
public:
    explicit alt_buf_write_t(alt_buf_lock_t *lock);
    ~alt_buf_write_t();

    void *get_data_write(uint32_t block_size);

private:
    alt_buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(alt_buf_write_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_ALT_ALT_HPP_
