// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_ALT_PAGE_HPP_
#define BUFFER_CACHE_ALT_PAGE_HPP_

#include <map>
#include <utility>
#include <vector>
#include <set>

#include "buffer_cache/alt/config.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/backindex_bag.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/segmented_vector.hpp"
#include "repli_timestamp.hpp"
#include "serializer/types.hpp"

class alt_memory_tracker_t;
class auto_drainer_t;
class cache_t;
class file_account_t;

namespace alt {
class current_page_acq_t;
class page_acq_t;
class page_cache_t;
class page_txn_t;

enum class page_create_t { no, yes };

}  // namespace alt

enum class alt_create_t { create };

class memory_tracker_t {
public:
    virtual ~memory_tracker_t() { }
    virtual void inform_memory_change(uint64_t in_memory_size,
                                      uint64_t memory_limit) = 0;
};

class alt_cache_account_t {
public:
    ~alt_cache_account_t();
private:
    friend class alt::page_cache_t;
    alt_cache_account_t(threadnum_t thread, file_account_t *io_account);
    // KSI: I hate having this thread_ variable, and it looks like the file_account_t
    // already worries about going to the right thread anyway.
    threadnum_t thread_;
    file_account_t *io_account_;
    DISABLE_COPYING(alt_cache_account_t);
};

class block_version_t {
public:
    block_version_t() : value_(0) { }

    block_version_t subsequent() const {
        block_version_t ret;
        ret.value_ = value_ + 1;
        return ret;
    }

    bool operator<(block_version_t other) const {
        return value_ < other.value_;
    }

    bool operator<=(block_version_t other) const {
        return value_ <= other.value_;
    }

    bool operator>=(block_version_t other) const {
        return value_ >= other.value_;
    }

    bool operator==(block_version_t other) const {
        return value_ == other.value_;
    }

    bool operator!=(block_version_t other) const {
        return !operator==(other);
    }

    uint64_t debug_value() const { return value_; }

private:
    uint64_t value_;
};

class cache_conn_t {
public:
    cache_conn_t(cache_t *cache)
        : cache_(cache),
          newest_txn_(NULL) { }
    ~cache_conn_t();

    cache_t *cache() const { return cache_; }
private:
    friend class alt::page_cache_t;
    friend class alt::page_txn_t;
    // Here for convenience, because otherwise you'd be passing around a cache_t with
    // every cache_conn_t parameter.
    cache_t *cache_;

    // The most recent unflushed txn, or NULL.  This gets set back to NULL when
    // newest_txn_ pulses its flush_complete_cond_.  It's a bidirectional pointer
    // pair with the newest txn's cache_conn_ pointer -- either both point at each
    // other or neither do.
    alt::page_txn_t *newest_txn_;

    DISABLE_COPYING(cache_conn_t);
};


namespace alt {

// A page_t represents a page (a byte buffer of a specific size), having a definite
// value known at the construction of the page_t (and possibly later modified
// in-place, but still a definite known value).
class page_t {
public:
    page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf,
           page_cache_t *page_cache);
    page_t(scoped_malloc_t<ser_buffer_t> buf,
           const counted_t<standard_block_token_t> &token,
           page_cache_t *page_cache);
    page_t(block_id_t block_id, page_cache_t *page_cache);
    page_t(page_t *copyee, page_cache_t *page_cache);
    ~page_t();

    page_t *make_copy(page_cache_t *page_cache);

    void add_waiter(page_acq_t *acq);
    void remove_waiter(page_acq_t *acq);

private:
    friend class page_acq_t;
    // These may not be called until the page_acq_t's buf_ready_signal is pulsed.
    void *get_page_buf(page_cache_t *page_cache);
    void reset_block_token();
    uint32_t get_page_buf_size();

    bool is_deleted();

    friend class page_ptr_t;
    void add_snapshotter();
    void remove_snapshotter(page_cache_t *page_cache);
    size_t num_snapshot_references();


    void pulse_waiters_or_make_evictable(page_cache_t *page_cache);

    static void load_with_block_id(page_t *page,
                                   block_id_t block_id,
                                   page_cache_t *page_cache);

    static void load_from_copyee(page_t *page, page_t *copyee,
                                 page_cache_t *page_cache);

    static void load_using_block_token(page_t *page, page_cache_t *page_cache);

    friend class page_cache_t;
    friend class evicter_t;
    friend class eviction_bag_t;
    friend backindex_bag_index_t *access_backindex(page_t *page);

    void evict_self();

    // KSI: Explain this more.
    // One of destroy_ptr_, buf_, or block_token_ is non-null.
    bool *destroy_ptr_;
    uint32_t ser_buf_size_;
    scoped_malloc_t<ser_buffer_t> buf_;
    counted_t<standard_block_token_t> block_token_;

    uint64_t access_time_;

    // How many page_ptr_t's point at this page, expecting nothing to modify it,
    // other than themselves.
    size_t snapshot_refcount_;

    // A list of waiters that expect the value to be loaded, and (as long as there
    // are waiters) expect the value to never be evicted.
    // RSP: This could be a single pointer instead of two.
    intrusive_list_t<page_acq_t> waiters_;

    // This page_t's index into its eviction bag (managed by the page_cache_t -- one
    // of unevictable_pages_, etc).  Which bag we should be in:
    //
    // if destroy_ptr_ is non-null:  unevictable_pages_
    // else if waiters_ is non-empty: unevictable_pages_
    // else if buf_ is null: evicted_pages_ (and block_token_ is non-null)
    // else if block_token_ is non-null: evictable_disk_backed_pages_
    // else: evictable_unbacked_pages_ (buf_ is non-null, block_token_ is null)
    //
    // So, when destroy_ptr_, waiters_, buf_, or block_token_ is touched, we might
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
    explicit page_ptr_t(page_t *page, page_cache_t *page_cache)
        : page_(NULL), page_cache_(NULL) { init(page, page_cache); }
    page_ptr_t();

    // The page_ptr_t _should_ be reset ()) before the destructor is called, but
    // it'll work right now without that.  Eventually, reset() will take a
    // page_cache_t parameter, and the page_cache_ field will be removed.
    ~page_ptr_t();

    page_ptr_t(page_ptr_t &&movee);
    page_ptr_t &operator=(page_ptr_t &&movee);

    void init(page_t *page, page_cache_t *page_cache);

    page_t *get_page_for_read() const;
    page_t *get_page_for_write(page_cache_t *page_cache);

    void reset();

    bool has() const {
        return page_ != NULL;
    }

private:
    page_t *page_;
    // KSI: Get rid of this variable.
    page_cache_t *page_cache_;
    DISABLE_COPYING(page_ptr_t);
};

// This type's purpose is to wait for the page to be loaded, and to prevent it from
// being unloaded.
class page_acq_t : public intrusive_list_node_t<page_acq_t> {
public:
    page_acq_t();
    ~page_acq_t();

    void init(page_t *page, page_cache_t *page_cache);

    page_cache_t *page_cache() const {
        rassert(page_cache_ != NULL);
        return page_cache_;
    }

    signal_t *buf_ready_signal();
    bool has() const;

    // These block, uninterruptibly waiting for buf_ready_signal() to be pulsed.
    uint32_t get_buf_size();
    void *get_buf_write();
    const void *get_buf_read();

private:
    friend class page_t;

    page_t *page_;
    page_cache_t *page_cache_;
    cond_t buf_ready_signal_;
    DISABLE_COPYING(page_acq_t);
};

// Has information necessary for the current_page_t to do certain things -- it's
// known by the current_page_acq_t.
struct current_page_help_t;

class current_page_t {
public:
    // Constructs a fresh, empty page.
    current_page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf,
                   page_cache_t *page_cache);
    current_page_t(scoped_malloc_t<ser_buffer_t> buf,
                   const counted_t<standard_block_token_t> &token,
                   page_cache_t *page_cache);
    // Constructs a page to be loaded from the serializer.
    current_page_t();
    ~current_page_t();


private:
    // current_page_acq_t should not access our fields directly.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);
    void pulse_pulsables(current_page_acq_t *acq);

    page_t *the_page_for_write(current_page_help_t help);
    page_t *the_page_for_read(current_page_help_t help);

    void convert_from_serializer_if_necessary(current_page_help_t help);

    void mark_deleted(current_page_help_t help);

    // page_txn_t should not access our fields directly.
    friend class page_txn_t;
    // Returns the previous last modifier (or NULL, if there's no active
    // last-modifying previous txn).
    page_txn_t *change_last_modifier(page_txn_t *new_last_modifier);

    // Returns NULL if the page was deleted.
    page_t *the_page_for_read_or_deleted(current_page_help_t help);

    // Has access to our fields.
    friend class page_cache_t;

    bool is_deleted() const { return is_deleted_; }

    void make_non_deleted(block_size_t block_size,
                          scoped_malloc_t<ser_buffer_t> buf,
                          page_cache_t *page_cache);

    // page_ can be null if we haven't tried loading the page yet.  We don't want to
    // prematurely bother loading the page if it's going to be deleted.
    // the_page_for_write, the_page_for_read, or the_page_for_read_or_deleted should
    // be used to access this variable.
    // KSI: Could we encapsulate that rule?
    page_ptr_t page_;
    // True if the block is in a deleted state.  page_ will be null.
    bool is_deleted_;

    // The last txn that modified the page, or marked it deleted.
    page_txn_t *last_modifier_;

    // An in-cache value that increments whenever the value is changed, so that
    // different page_txn_t's can know which was the last to modify the block.
    block_version_t block_version_;

    // Instead of storing the recency here, we store it page_cache_t::recencies_.

    // All list elements have current_page_ != NULL, snapshotted_page_ == NULL.
    intrusive_list_t<current_page_acq_t> acquirers_;

    DISABLE_COPYING(current_page_t);
};

class current_page_acq_t : public intrusive_list_node_t<current_page_acq_t>,
                           public home_thread_mixin_debug_only_t {
public:
    // KSI: Right now we support a default constructor but buf_lock_t actually
    // uses a scoped pointer now, because getting this type to be swappable was too
    // hard.  Make this type be swappable or remove the default constructor.  (Remove
    // the page_cache_ != NULL check in the destructor we remove the default
    // constructor.)
    current_page_acq_t();
    current_page_acq_t(page_txn_t *txn,
                       block_id_t block_id,
                       access_t access,
                       page_create_t create = page_create_t::no);
    current_page_acq_t(page_txn_t *txn,
                       alt_create_t create);
    current_page_acq_t(page_cache_t *cache,
                       block_id_t block_id,
                       read_access_t read);
    ~current_page_acq_t();

    // Declares ourself snapshotted.  (You must be readonly to do this.)
    void declare_snapshotted();

    signal_t *read_acq_signal();
    signal_t *write_acq_signal();

    page_t *current_page_for_read();
    page_t *current_page_for_write();

    // Returns current_page_for_read, except it guarantees that the page acq has
    // already snapshotted the page and is not waiting for the page_t *.
    page_t *snapshotted_page_ptr();

    block_id_t block_id() const { return block_id_; }
    access_t access() const { return access_; }
    repli_timestamp_t recency() const;

    void mark_deleted();

    block_version_t block_version() const;

private:
    void init(page_txn_t *txn,
              block_id_t block_id,
              access_t access,
              page_create_t create);
    void init(page_txn_t *txn,
              alt_create_t create);
    void init(page_cache_t *page_cache,
              block_id_t block_id,
              read_access_t read);
    friend class page_txn_t;
    friend class current_page_t;

    // Returns true if the page has been created, edited, or deleted.
    bool dirtied_page() const;
    // Declares ourself readonly.  Only page_txn_t::remove_acquirer can do this!
    void declare_readonly();

    current_page_help_t help() const;
    page_cache_t *page_cache() const;

    void pulse_read_available();
    void pulse_write_available();

    page_cache_t *page_cache_;
    page_txn_t *the_txn_;
    access_t access_;
    bool declared_snapshotted_;
    // The block id of the page we acquired.
    block_id_t block_id_;
    // At most one of current_page_ is null or snapshotted_page_ is null, unless the
    // acquired page has been deleted, in which case both are null.
    current_page_t *current_page_;
    page_ptr_t snapshotted_page_;
    cond_t read_cond_;
    cond_t write_cond_;

    // The recency for our acquisition of the page.
    repli_timestamp_t recency_;

    // The block version for our acquisition of the page -- every write acquirer sees
    // a greater block version than the previous acquirer.  The current page's block
    // version will be less than or equal to this value if we have not yet acquired
    // the page.  It could be greater than this value if we're snapshotted (since
    // we're holding an old version of the page).  These values are for internal
    // cache bookkeeping only.
    block_version_t block_version_;

    bool dirtied_page_;

    DISABLE_COPYING(current_page_acq_t);
};

class free_list_t {
public:
    explicit free_list_t(serializer_t *serializer);
    ~free_list_t();

    // Returns a block id.  The current_page_t for this block id, if present, has no
    // acquirers.
    MUST_USE block_id_t acquire_block_id();
    void release_block_id(block_id_t block_id);

    // Like acquire_block_id, only instead of being told what block id you acquired,
    // you tell it which block id you're acquiring.  It must be available!
    void acquire_chosen_block_id(block_id_t block_id);

private:
    block_id_t next_new_block_id_;
    // RSP: std::vector performance.
    std::vector<block_id_t> free_ids_;
    DISABLE_COPYING(free_list_t);
};

class eviction_bag_t {
public:
    eviction_bag_t();
    ~eviction_bag_t();

    // For adding a page that doesn't have a size yet -- its size is not yet
    // accounted for, because its buf isn't loaded yet.
    void add_without_size(page_t *page);

    // Adds the size for a page that was added with add_without_size, now that it has
    // been loaded and we know the size.
    void add_size(uint32_t ser_buf_size);

    // Adds the page with its known size.
    void add(page_t *page, uint32_t ser_buf_size);

    // Removes the page with its size.
    void remove(page_t *page, uint32_t ser_buf_size);

    // Returns true if this bag contains the given page.
    bool has_page(page_t *page) const;

    uint64_t size() const { return size_; }

    bool remove_oldish(page_t **page_out, uint64_t access_time_offset);

private:
    backindex_bag_t<page_t *> bag_;
    // The size in memory.
    uint64_t size_;

    DISABLE_COPYING(eviction_bag_t);
};

class evicter_t : public home_thread_mixin_debug_only_t {
public:
    void add_not_yet_loaded(page_t *page);
    void add_now_loaded_size(uint32_t ser_buf_size);
    void add_to_evictable_unbacked(page_t *page);
    void add_to_evictable_disk_backed(page_t *page);
    bool page_is_in_unevictable_bag(page_t *page) const;
    void move_unevictable_to_evictable(page_t *page);
    void change_eviction_bag(eviction_bag_t *current_bag, page_t *page);
    eviction_bag_t *correct_eviction_category(page_t *page);
    void remove_page(page_t *page);

    explicit evicter_t(memory_tracker_t *tracker,
                       uint64_t memory_limit);
    ~evicter_t();

    bool interested_in_read_ahead_block(uint32_t ser_block_size) const;

    uint64_t next_access_time() {
        return ++access_time_counter_;
    }

private:
    void evict_if_necessary();
    uint64_t in_memory_size() const;

    void inform_tracker() const;

    // LSI: Implement issue 97.
    memory_tracker_t *const tracker_;
    uint64_t memory_limit_;

    // This gets incremented every time a page is accessed.
    uint64_t access_time_counter_;

    // These track whether every page's eviction status.
    eviction_bag_t unevictable_;
    eviction_bag_t evictable_disk_backed_;
    eviction_bag_t evictable_unbacked_;
    eviction_bag_t evicted_;

    DISABLE_COPYING(evicter_t);
};

// This object lives on the serializer thread.
class page_read_ahead_cb_t : public home_thread_mixin_t,
                             public serializer_read_ahead_callback_t {
public:
    page_read_ahead_cb_t(serializer_t *serializer,
                         page_cache_t *cache,
                         uint64_t bytes_to_send);

    void offer_read_ahead_buf(block_id_t block_id,
                              scoped_malloc_t<ser_buffer_t> *buf,
                              const counted_t<standard_block_token_t> &token);

    void destroy_self();

private:
    ~page_read_ahead_cb_t();

    serializer_t *serializer_;
    page_cache_t *page_cache_;

    // How many more bytes of data can we send?
    uint64_t bytes_remaining_;

    DISABLE_COPYING(page_read_ahead_cb_t);
};

class tracker_acq_t {
public:
    tracker_acq_t() { }
    ~tracker_acq_t() { }
    tracker_acq_t(tracker_acq_t &&movee)
        : semaphore_acq_(std::move(movee.semaphore_acq_)) {
        movee.semaphore_acq_.reset();
    }

    // See below:  this can update how much semaphore_acq_ holds.
    void update_dirty_page_count(int64_t new_count);

private:
    friend class ::alt_memory_tracker_t;
    // At first, the number of dirty pages is 0 and semaphore_acq_.count() >=
    // dirtied_count_.  Once the number of dirty pages gets bigger than the original
    // value of semaphore_acq_.count(), we use semaphore_acq_.change_count() to keep
    // the numbers equal.
    new_semaphore_acq_t semaphore_acq_;

    DISABLE_COPYING(tracker_acq_t);
};

class page_cache_t : public home_thread_mixin_t {
public:
    page_cache_t(serializer_t *serializer,
                 const page_cache_config_t &config,
                 memory_tracker_t *tracker);
    ~page_cache_t();

    // Takes a txn to be flushed.  Calls on_flush_complete() when done.
    void flush_and_destroy_txn(
            scoped_ptr_t<page_txn_t> txn,
            std::function<void(tracker_acq_t)> on_flush_complete);

    current_page_t *page_for_block_id(block_id_t block_id);
    current_page_t *page_for_new_block_id(block_id_t *block_id_out);
    current_page_t *page_for_new_chosen_block_id(block_id_t block_id);

    // Returns how much memory is being used by all the pages in the cache at this
    // moment in time.
    size_t total_page_memory() const;
    size_t evictable_page_memory() const;

    block_size_t max_block_size() const;

    void create_cache_account(int priority, scoped_ptr_t<alt_cache_account_t> *out);

private:
    friend class page_read_ahead_cb_t;
    void add_read_ahead_buf(block_id_t block_id,
                            ser_buffer_t *buf,
                            const counted_t<standard_block_token_t> &token);

    void have_read_ahead_cb_destroyed();

    void read_ahead_cb_is_destroyed();


    current_page_t *internal_page_for_new_chosen(block_id_t block_id);

    friend class page_t;
    evicter_t &evicter() { return evicter_; }

    friend class page_txn_t;
    static void do_flush_txn_set(page_cache_t *page_cache,
                                 const std::set<page_txn_t *> &txns,
                                 fifo_enforcer_write_token_t index_write_token);
    static void remove_txn_set_from_graph(page_cache_t *page_cache,
                                          const std::set<page_txn_t *> &txns);

    // KSI: Maybe just have txn_t hold a single list of block_change_t objects.
    struct block_change_t {
        block_change_t(block_version_t _version, bool _modified,
                       page_t *_page, repli_timestamp_t _tstamp)
            : version(_version), modified(_modified), page(_page), tstamp(_tstamp) { }
        block_version_t version;

        // True if the value of the block was modified (or the block was deleted), false
        // if the block was only touched.
        bool modified;
        // If modified == true, the new value for the block, or NULL if the block was
        // deleted.  (The page_t's lifetime is kept by some page_txn_t's
        // snapshotted_dirtied_pages_ field.)
        page_t *page;
        repli_timestamp_t tstamp;
    };

    static std::map<block_id_t, block_change_t>
    compute_changes(const std::set<page_txn_t *> &txns);

    bool exists_flushable_txn_set(page_txn_t *txn,
                                  std::set<page_txn_t *> *flush_set_out);

    void im_waiting_for_flush(page_txn_t *txn);

    repli_timestamp_t recency_for_block_id(block_id_t id) {
        return recencies_.size() <= id
            ? repli_timestamp_t::invalid
            : recencies_[id];
    }

    void set_recency_for_block_id(block_id_t id, repli_timestamp_t recency) {
        while (recencies_.size() <= id) {
            recencies_.push_back(repli_timestamp_t::invalid);
        }
        recencies_[id] = recency;
    }

    friend class current_page_t;
    serializer_t *serializer() { return serializer_; }
    free_list_t *free_list() { return &free_list_; }

    void resize_current_pages_to_id(block_id_t block_id);

    const page_cache_config_t dynamic_config_;

    // We use a separate IO account for reads and writes, so reads can pass ahead of
    // active writebacks. Otherwise writebacks could badly block out readers, thereby
    // blocking user queries.
    scoped_ptr_t<file_account_t> reads_io_account_;
    scoped_ptr_t<file_account_t> writes_io_account_;

    // This fifo enforcement pair ensures ordering of index_write operations after we
    // move to the serializer thread and get a bunch of blocks written.
    // index_write_sink's pointee's home thread is on the serializer.
    fifo_enforcer_source_t index_write_source_;
    scoped_ptr_t<fifo_enforcer_sink_t> index_write_sink_;

    serializer_t *serializer_;
    segmented_vector_t<repli_timestamp_t> recencies_;

    // RSP: Array growth slow.
    std::vector<current_page_t *> current_pages_;

    free_list_t free_list_;

    evicter_t evicter_;

    // KSI: I bet this read_ahead_cb_ and read_ahead_cb_existence_ type could be
    // packaged in some new cross_thread_ptr type.
    page_read_ahead_cb_t *read_ahead_cb_;

    // Holds a lock on *drainer_ is until shortly after the page_read_ahead_cb_t is
    // destroyed and all possible read-ahead operations have completed.
    auto_drainer_t::lock_t read_ahead_cb_existence_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(page_cache_t);
};

class dirtied_page_t {
public:
    dirtied_page_t()
        : block_id(NULL_BLOCK_ID),
          tstamp(repli_timestamp_t::invalid) { }
    dirtied_page_t(block_version_t _block_version,
                   block_id_t _block_id, page_ptr_t &&_ptr,
                   repli_timestamp_t _tstamp)
        : block_version(_block_version),
          block_id(_block_id),
          ptr(std::move(_ptr)),
          tstamp(_tstamp) { }
    dirtied_page_t(dirtied_page_t &&movee)
        : block_version(movee.block_version),
          block_id(movee.block_id),
          ptr(std::move(movee.ptr)),
          tstamp(movee.tstamp) { }
    dirtied_page_t &operator=(dirtied_page_t &&movee) {
        block_version = movee.block_version;
        block_id = movee.block_id;
        ptr = std::move(movee.ptr);
        tstamp = movee.tstamp;
        return *this;
    }
    // Our block version of the dirty page.
    block_version_t block_version;
    // The block id of the dirty page.
    block_id_t block_id;
    // The pointer to the snapshotted dirty page value.  (If empty, the page was
    // deleted.)
    page_ptr_t ptr;
    // The timestamp of the modification.
    repli_timestamp_t tstamp;
};

class touched_page_t {
public:
    touched_page_t()
        : block_id(NULL_BLOCK_ID),
          tstamp(repli_timestamp_t::invalid) { }
    touched_page_t(block_version_t _block_version,
                   block_id_t _block_id,
                   repli_timestamp_t _tstamp)
        : block_version(_block_version),
          block_id(_block_id),
          tstamp(_tstamp) { }

    block_version_t block_version;
    block_id_t block_id;
    repli_timestamp_t tstamp;
};

// page_txn_t's exist for the purpose of writing to disk.  The rules are as follows:
//
//  - When a page_txn_t gets "committed" (written to disk), all blocks modified with
//    a given page_txn_t must be committed to disk at the same time.  (That is, they
//    all go in the same index_write operation.)
//
//  - For all blocks N and page_txn_t S and T, if S modifies N before T modifies N,
//    then S must be committed to disk before or at the same time as T.
//
//  - For all page_txn_t S and T, if S is the preceding_txn of T then S must be
//    committed to disk before or at the same time as T.
//
// As a result, we form a graph of txns, which gets modified on the fly, and we
// commit them in topological order.  Cycles can happen (for example, if (a) two
// transactions modify the same physical memory, or (b) they modify blocks in
// opposite orders), and transactions that depend on one another (forming a cycle)
// must get flushed simultaneously (in the same serializer->index_write operation).
// Situation '(a)' can happen as a matter of course, assuming transactions don't
// greedily save their modified copy of a page.  Situation '(b)' can happen if
// transactions apply a commutative operation on a block, like with the stats block.
// Right now, situation '(a)' doesn't happen because transactions do greedily keep
// their copies of the block.
//
// LSI: Make situation '(a)' happenable.
class page_txn_t {
public:
    // Our transaction has to get committed to disk _after_ or at the same time as
    // preceding transactions on cache_conn, if that parameter is not NULL.  (The
    // parameter's NULL for read txns, for now.)
    page_txn_t(page_cache_t *page_cache,
               // Unused for read transactions, pass repli_timestamp_t::invalid.
               repli_timestamp_t txn_recency,
               tracker_acq_t tracker_acq,
               cache_conn_t *cache_conn);

    // KSI: This is only to be called by the page cache -- should txn_t really use a
    // scoped_ptr_t?
    ~page_txn_t();

    page_cache_t *page_cache() const { return page_cache_; }

    void set_account(alt_cache_account_t *cache_account);

private:
    // To set cache_conn_ to NULL.
    friend class ::cache_conn_t;

    // To access tracker_acq_.
    friend class flush_and_destroy_t;

    // page cache has access to all of this type's innards, including fields.
    friend class page_cache_t;

    // For access to this_txn_recency_.
    friend class current_page_t;

    // Adds and connects a preceder.
    void connect_preceder(page_txn_t *preceder);

    // Removes a preceder, which is already half-way disconnected.
    void remove_preceder(page_txn_t *preceder);

    // Removes a subseqer, which is already half-way disconnected.
    void remove_subseqer(page_txn_t *subseqer);

    // current_page_acq should only call add_acquirer and remove_acquirer.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);

    void announce_waiting_for_flush_if_we_should();

    page_cache_t *page_cache_;
    // This can be NULL, if the txn is not part of some cache conn.
    cache_conn_t *cache_conn_;

    // An acquisition object for the memory tracker.
    tracker_acq_t tracker_acq_;

    repli_timestamp_t this_txn_recency_;

    // KSI: This is ugh-ish and the design is borrowed from the mirrored cache.
    alt_cache_account_t *cache_account_;

    // page_txn_t's form a directed graph.  preceders_ and subseqers_ represent the
    // inward-pointing and outward-pointing arrows.  (I'll let you decide which
    // direction should be inward and which should be outward.)  Each page_txn_t
    // pointed at by subseqers_ has an entry in its preceders_ that is back-pointing
    // at this page_txn_t.  (And vice versa for each page_txn_t pointed at by
    // preceders_.)

    // The transactions that must be committed before or at the same time as this
    // transaction.
    std::vector<page_txn_t *> preceders_;

    // txn's that we precede.
    // RSP: Performance?
    std::vector<page_txn_t *> subseqers_;

    // Pages for which this page_txn_t is the last_modifier_ of that page.
    std::vector<current_page_t *> pages_modified_last_;

    // acqs that are currently alive.
    // RSP: Performance?  remove_acquirer takes linear time.
    std::vector<current_page_acq_t *> live_acqs_;

    // Saved pages (by block id).
    segmented_vector_t<dirtied_page_t, 8> snapshotted_dirtied_pages_;

    // Touched pages (by block id).
    segmented_vector_t<touched_page_t, 8> touched_pages_;

    // KSI: We could probably turn began_waiting_for_flush_ and spawned_flush_ into a
    // generalized state enum.
    //
    // KSI: Should we have the spawned_flush_ variable or should we remove the txn
    // from the graph?

    // Tells whether this page_txn_t has announced itself (to the cache) to be
    // waiting for a flush.
    bool began_waiting_for_flush_;
    bool spawned_flush_;

    // This gets pulsed when the flush is complete or when the txn has no reason to
    // exist any more.
    cond_t flush_complete_cond_;

    DISABLE_COPYING(page_txn_t);
};

}  // namespace alt


#endif  // BUFFER_CACHE_ALT_PAGE_HPP_
