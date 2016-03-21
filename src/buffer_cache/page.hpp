#ifndef BUFFER_CACHE_PAGE_HPP_
#define BUFFER_CACHE_PAGE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/backindex_bag.hpp"
#include "containers/half_intrusive_list.hpp"
#include "repli_timestamp.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/types.hpp"

class cache_account_t;

namespace alt {

class page_cache_t;
class page_acq_t;

class page_loader_t;
class deferred_page_loader_t;
class deferred_block_token_t;

// A page_t represents a page (a byte buffer of a specific size), having a definite
// value known at the construction of the page_t (and possibly later modified
// in-place, but still a definite known value).
class page_t {
public:
    // Defers loading the block for the given block id (but does go and get its block
    // token ASAP, so that we can't lose access to the current version of the block).
    page_t(block_id_t block_id, page_cache_t *page_cache);
    // Loads the block for the given block id.
    page_t(block_id_t block_id, page_cache_t *page_cache, cache_account_t *account);

    page_t(block_id_t block_id, buf_ptr_t buf, page_cache_t *page_cache);
    page_t(block_id_t block_id, buf_ptr_t buf,
           const counted_t<standard_block_token_t> &token,
           page_cache_t *page_cache);
    page_t(page_t *copyee, page_cache_t *page_cache, cache_account_t *account);
    ~page_t();

    page_t *make_copy(page_cache_t *page_cache, cache_account_t *account);

    void add_waiter(page_acq_t *acq, cache_account_t *account);
    void remove_waiter(page_acq_t *acq);

    // These may not be called until the page_acq_t's buf_ready_signal is pulsed.
    void *get_page_buf(page_cache_t *page_cache);
    void reset_block_token(page_cache_t *page_cache);
    void set_page_buf_size(block_size_t block_size, page_cache_t *page_cache);

    block_size_t get_page_buf_size();

    // How much memory the block would use, if it were in memory.  (If the block is
    // already in memory, this is how much memory the block is currently
    // using, of course.)

    uint32_t hypothetical_memory_usage(page_cache_t *page_cache) const;
    uint64_t access_time() const { return access_time_; }

    bool is_loading() const {
        return loader_ != nullptr && page_t::loader_is_loading(loader_);
    }
    bool is_deferred_loading() const {
        return loader_ != nullptr && !page_t::loader_is_loading(loader_);
    }
    bool has_waiters() const { return !waiters_.empty(); }
    bool is_loaded() const { return buf_.has(); }
    bool is_disk_backed() const { return block_token_.has(); }

    void evict_self(page_cache_t *page_cache);

    block_id_t block_id() const { return block_id_; }

    bool page_ptr_count() const { return snapshot_refcount_; }

    const counted_t<standard_block_token_t> &block_token() const {
        return block_token_;
    }

    ser_buffer_t *get_loaded_ser_buffer();
    void init_block_token(counted_t<standard_block_token_t> token,
                          page_cache_t *page_cache);

private:
    friend class page_ptr_t;
    friend class deferred_page_loader_t;
    static bool loader_is_loading(page_loader_t *loader);
    void add_snapshotter();
    void remove_snapshotter(page_cache_t *page_cache);
    size_t num_snapshot_references();


    void pulse_waiters_or_make_evictable(page_cache_t *page_cache);


    static void finish_load_with_block_id(page_t *page, page_cache_t *page_cache,
                                          counted_t<standard_block_token_t> block_token,
                                          buf_ptr_t buf);

    static void catch_up_with_deferred_load(
            deferred_page_loader_t *deferred_loader,
            page_cache_t *page_cache,
            cache_account_t *account);

    static void deferred_load_with_block_id(page_t *page, block_id_t block_id,
                                            page_cache_t *page_cache);


    static void load_with_block_id(page_t *page,
                                   block_id_t block_id,
                                   page_cache_t *page_cache,
                                   cache_account_t *account);

    static void load_from_copyee(page_t *page, page_t *copyee,
                                 page_cache_t *page_cache,
                                 cache_account_t *account);

    static void load_using_block_token(page_t *page, page_cache_t *page_cache,
                                       cache_account_t *account);

    friend backindex_bag_index_t *access_backindex(page_t *page);

    // The block id.  Used to (potentially) delete the page_t and current_page_t when
    // it gets evicted.
    const block_id_t block_id_;

    // One of loader_, buf_, or block_token_ is non-null.  (Either the page is in
    // memory, or there is always a way to get the page into memory.)
    page_loader_t *loader_;

    buf_ptr_t buf_;
    counted_t<standard_block_token_t> block_token_;

    uint64_t access_time_;

    // How many page_ptr_t's point at this page, expecting nothing to modify it,
    // other than themselves.
    size_t snapshot_refcount_;

    // A list of waiters that expect the value to be loaded, and (as long as there
    // are waiters) expect the value to never be evicted.
    half_intrusive_list_t<page_acq_t> waiters_;

    // This page_t's index into its eviction bag (managed by the page_cache_t -- one
    // of unevictable_pages_, etc).  Which bag we should be in:
    //
    // if loader_ is non-null:  unevictable_pages_
    // else if waiters_ is non-empty: unevictable_pages_
    // else if buf_ is null: evicted_pages_ (and block_token_ is non-null)
    // else if block_token_ is non-null: evictable_disk_backed_pages_
    // else: evictable_unbacked_pages_ (buf_ is non-null, block_token_ is null)
    //
    // So, when loader_, waiters_, buf_, or block_token_ is touched, we might
    // need to change this page's eviction bag.
    //
    // The logic above is implemented in page_cache_t::correct_eviction_category.
    backindex_bag_index_t eviction_index_;

    DISABLE_COPYING(page_t);
};

inline backindex_bag_index_t *access_backindex(page_t *page) {
    return &page->eviction_index_;
}

// A page_ptr_t holds a pointer to a page_t.
class page_ptr_t {
public:
    explicit page_ptr_t(page_t *page)
        : page_(nullptr) { init(page); }
    page_ptr_t();

    // The page_ptr_t MUST be reset before the destructor is called.
    ~page_ptr_t();

    // You MUST manually call reset_page_ptr() to reset the page_ptr_t.  Then, please
    // call consider_evicting_current_page if applicable.
    void reset_page_ptr(page_cache_t *page_cache);

    page_ptr_t(page_ptr_t &&movee);
    page_ptr_t &operator=(page_ptr_t &&movee);

    void init(page_t *page);

    page_t *get_page_for_read() const;
    page_t *get_page_for_write(page_cache_t *page_cache,
                               cache_account_t *account);

    bool has() const {
        return page_ != nullptr;
    }

private:
    void swap_with(page_ptr_t *other);

    page_t *page_;
    DISABLE_COPYING(page_ptr_t);
};

class timestamped_page_ptr_t {
public:
    timestamped_page_ptr_t();
    ~timestamped_page_ptr_t();

    timestamped_page_ptr_t(timestamped_page_ptr_t &&movee);
    timestamped_page_ptr_t &operator=(timestamped_page_ptr_t &&movee);

    bool has() const;

    void init(repli_timestamp_t timestamp, page_t *page);

    page_t *get_page_for_read() const;

    repli_timestamp_t timestamp() const { return timestamp_; }

    void reset_page_ptr(page_cache_t *page_cache);

private:
    repli_timestamp_t timestamp_;
    page_ptr_t page_ptr_;
    DISABLE_COPYING(timestamped_page_ptr_t);
};

// This type's purpose is to wait for the page to be loaded, and to prevent it from
// being unloaded.
class page_acq_t : public half_intrusive_list_node_t<page_acq_t> {
public:
    page_acq_t();
    ~page_acq_t();

    void init(page_t *page, page_cache_t *page_cache, cache_account_t *account);

    page_cache_t *page_cache() const {
        rassert(page_cache_ != NULL);
        return page_cache_;
    }

    signal_t *buf_ready_signal();
    bool has() const;

    // These block, uninterruptibly waiting for buf_ready_signal() to be pulsed.
    block_size_t get_buf_size();
    void *get_buf_write(block_size_t block_size);
    const void *get_buf_read();

private:
    friend class page_t;

    page_t *page_;
    page_cache_t *page_cache_;
    cond_t buf_ready_signal_;
    DISABLE_COPYING(page_acq_t);
};

}  // namespace alt

#endif  // BUFFER_CACHE_PAGE_HPP_
