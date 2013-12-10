#ifndef BUFFER_CACHE_ALT_ALT_HPP_
#define BUFFER_CACHE_ALT_ALT_HPP_

#include <vector>
#include <utility>

#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/general_types.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

class auto_drainer_t;
class serializer_t;

namespace alt {

class alt_buf_lock_t;

enum class alt_create_t {
    create,
};

class alt_cache_t : public home_thread_mixin_t {
public:
    explicit alt_cache_t(serializer_t *serializer);
    ~alt_cache_t();

    block_size_t max_block_size() const;
    // RSI: Remove this.
    block_size_t get_block_size() const { return max_block_size(); }

    page_cache_t page_cache_;

private:
    friend class alt_txn_t;  // for cache()->drainer_->lock().
    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(alt_cache_t);
};

class alt_inner_txn_t {
public:
    // This is public because scoped_ptr_t needs to call it.
    ~alt_inner_txn_t();

private:
    friend class alt_txn_t;
    alt_inner_txn_t(alt_cache_t *cache, alt_inner_txn_t *preceding_txn_or_null);

    alt_cache_t *cache() { return cache_; }

    page_txn_t *page_txn() { return &page_txn_; }

    alt_cache_t *cache_;
    page_txn_t page_txn_;
    // RSI: Is this_txn_timestamp_ used?  How?
    repli_timestamp_t this_txn_timestamp_;

    DISABLE_COPYING(alt_inner_txn_t);
};

class alt_txn_t {
public:
    alt_txn_t(alt_cache_t *cache, write_durability_t durability,
              alt_txn_t *preceding_txn = NULL);
    alt_txn_t(alt_cache_t *cache, alt_txn_t *preceding_txn = NULL);
    ~alt_txn_t();

    alt_cache_t *cache() { return inner_->cache(); }
    page_txn_t *page_txn() { return inner_->page_txn(); }
private:
    scoped_ptr_t<alt_inner_txn_t> inner_;
    write_durability_t durability_;
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

class alt_buf_parent_t;

class alt_buf_lock_t {
public:
    alt_buf_lock_t();

    // RSI: These constructors definitely duplicate one another.  Too bad one
    // constructor can't call another (in GCC 4.4).  Maybe we could still dedup
    // these, though, or get rid of some.  (Make alt_access_t include create, and
    // separate it from page_access_t?)

    // Nonblocking constructor.
    alt_buf_lock_t(alt_buf_parent_t parent,
                   block_id_t block_id,
                   alt_access_t access);

    // Nonblocking constructor, creates a new block with a specified block id.
    alt_buf_lock_t(alt_txn_t *txn,
                   block_id_t block_id,
                   alt_create_t create);

    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  In either case, returns before the block is acquired, but after we're
    // _in line_ for the block.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   block_id_t block_id,
                   alt_access_t access);

    // Nonblocking constructor that acquires a block with a new block id.  (RSI: Is
    // this useful for _anything_?  We refer to the superblock by name.)  `access`
    // must be `write`.
    alt_buf_lock_t(alt_buf_parent_t parent,
                   alt_create_t access);

    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  Allocates a block with a new block id.  `access` must be `write`.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   alt_create_t access);

    ~alt_buf_lock_t();

    alt_buf_lock_t(alt_buf_lock_t &&movee);
    alt_buf_lock_t &operator=(alt_buf_lock_t &&movee);

    void swap(alt_buf_lock_t &other);
    void reset_buf_lock();
    bool empty() const {
        return txn_ == NULL;
    }

    void snapshot_subtree();

    void detach_child(block_id_t child_id);

    // Reduces access to readonly.
    void reduce_to_readonly();

    // Reduces access to nothing, but we still hold the block for snapshotting
    // purposes.
    void reduce_to_nothing();



    block_id_t block_id() const {
        guarantee(txn_ != NULL);
        return current_page_acq_->block_id();
    }
    // RSI: Remove get_block_id().
    block_id_t get_block_id() const { return block_id(); }

    // RSI: Rename.
    repli_timestamp_t get_recency() const;

    alt_access_t access() const {
        guarantee(!empty());
        return current_page_acq_->access();
    }

    signal_t *read_acq_signal() {
        guarantee(!empty());
        return current_page_acq_->read_acq_signal();
    }
    signal_t *write_acq_signal() {
        guarantee(!empty());
        return current_page_acq_->write_acq_signal();
    }

    void mark_deleted() {
        guarantee(!empty());
        current_page_acq_->mark_deleted();
    }

    alt_txn_t *txn() const { return txn_; }
    alt_cache_t *cache() const { return txn_->cache(); }

private:
    static void wait_for_parent(alt_buf_parent_t parent, alt_access_t access);

    friend class alt_buf_read_t;
    friend class alt_buf_write_t;

    alt_txn_t *txn_;

    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    alt_snapshot_node_t *snapshot_node_;

    DISABLE_COPYING(alt_buf_lock_t);
};


class alt_buf_parent_t {
public:
    alt_buf_parent_t() : txn_(NULL), lock_or_null_(NULL) { }

    explicit alt_buf_parent_t(alt_buf_lock_t *lock)
        : txn_(lock->txn()), lock_or_null_(lock) {
        guarantee(lock != NULL);
        guarantee(!lock->empty());
    }
    // RSI: Replace this constructor with a create_dangerously static method.
    explicit alt_buf_parent_t(alt_txn_t *txn)
        : txn_(txn), lock_or_null_(NULL) { }

    alt_txn_t *txn() const {
        guarantee(txn_ != NULL);
        return txn_;
    }
    alt_cache_t *cache() const {
        guarantee(txn_ != NULL);
        return txn_->cache();
    }

private:
    friend class alt_buf_lock_t;
    alt_txn_t *txn_;
    alt_buf_lock_t *lock_or_null_;
};

class alt_buf_read_t {
public:
    explicit alt_buf_read_t(alt_buf_lock_t *lock);
    ~alt_buf_read_t();

    const void *get_data_read(uint32_t *block_size_out);
    // RSI: Remove.
    const void *get_data_read() {
        uint32_t unused_block_size;
        return get_data_read(&unused_block_size);
    }

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
    // Equivalent to passing the max_block_size.
    void *get_data_write();

private:
    alt_buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(alt_buf_write_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_ALT_ALT_HPP_
