#ifndef BUFFER_CACHE_ALT_PAGE_HPP_
#define BUFFER_CACHE_ALT_PAGE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"
#include "serializer/types.hpp"

class auto_drainer_t;
class file_account_t;

namespace alt {

class current_page_acq_t;
class page_acq_t;
class page_cache_t;


enum class alt_access_t { read, write };

class page_t {
public:
    page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf);
    page_t(block_id_t block_id, page_cache_t *page_cache);
    page_t(page_t *copyee, page_cache_t *page_cache);
    ~page_t();

    void *get_buf();
    uint32_t get_buf_size();

    void add_snapshotter();
    void remove_snapshotter();
    bool has_snapshot_references();
    page_t *make_copy(page_cache_t *page_cache);

    void add_waiter(page_acq_t *acq);
    void remove_waiter(page_acq_t *acq);

private:
    void pulse_waiters();

    static void load_with_block_id(page_t *page,
                                   block_id_t block_id,
                                   page_cache_t *page_cache);

    static void load_from_copyee(page_t *page, page_t *copyee,
                                 page_cache_t *page_cache);

    // One of destroy_ptr_, buf_, or block_token_ is non-null.
    bool *destroy_ptr_;
    block_size_t buf_size_;
    scoped_malloc_t<ser_buffer_t> buf_;
    counted_t<standard_block_token_t> block_token_;

    // How many pointers to this value are expecting it to be snapshotted?
    size_t snapshot_refcount_;

    // RSP: This could be a single pointer instead of two.
    intrusive_list_t<page_acq_t> waiters_;
    DISABLE_COPYING(page_t);
};

class page_acq_t : public intrusive_list_node_t<page_acq_t> {
public:
    page_acq_t();
    ~page_acq_t();

    void init(page_t *page);

    signal_t *buf_ready_signal();
    bool has() const;

private:
    friend class page_t;

    page_t *page_;
    cond_t buf_ready_signal_;
    DISABLE_COPYING(page_acq_t);
};

class current_page_t {
public:
    // Constructs a fresh, empty page.
    current_page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf,
                   page_cache_t *page_cache);
    // Constructs a page to be loaded from the serializer.
    current_page_t(block_id_t block_id, page_cache_t *page_cache);
    ~current_page_t();


private:
    // current_page_acq_t should not access our fields directly.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);
    void pulse_pulsables(current_page_acq_t *acq);

    page_t *the_page_for_write();
    page_t *the_page_for_read();

    void convert_from_serializer_if_necessary();

    // Our block id.
    block_id_t block_id_;
    // Either page_ is null or page_cache_ is null (except that page_cache_ is never
    // null).  block_id_ and page_cache_ can be used to construct (and load) the page
    // when it's null.  RSP: This is a waste of memory.
    page_cache_t *page_cache_;
    page_t *page_;

    // All list elements have current_page_ != NULL, snapshotted_page_ == NULL.
    intrusive_list_t<current_page_acq_t> acquirers_;
    DISABLE_COPYING(current_page_t);
};

class current_page_acq_t : public intrusive_list_node_t<current_page_acq_t> {
public:
    current_page_acq_t(current_page_t *current_page, alt_access_t access);
    ~current_page_acq_t();

    void declare_snapshotted();

    signal_t *read_acq_signal();
    signal_t *write_acq_signal();

    page_t *page_for_read();
    page_t *page_for_write();

private:
    friend class current_page_t;

    alt_access_t access_;
    bool declared_snapshotted_ = false;  // RSI this won't compile
    // At most one of current_page_ is NULL or snapshotted_page_ is NULL.
    current_page_t *current_page_;
    page_t *snapshotted_page_;
    cond_t read_cond_;
    cond_t write_cond_;

    DISABLE_COPYING(current_page_acq_t);
};

class free_list_t {
public:
    free_list_t(serializer_t *serializer);
    ~free_list_t();

    block_id_t acquire_block_id();
    void release_block_id(block_id_t block_id);

private:
    block_id_t next_new_block_id_;
    // RSP: std::vector performance.
    std::vector<block_id_t> free_ids_;
    DISABLE_COPYING(free_list_t);
};

class page_cache_t {
public:
    page_cache_t(serializer_t *serializer);
    ~page_cache_t();
    current_page_t *page_for_block_id(block_id_t block_id);
    current_page_t *page_for_new_block_id(block_id_t *block_id_out);



private:
    friend class page_t;

    // We use a separate IO account for reads and writes, so reads can pass ahead
    // of active writebacks. Otherwise writebacks could badly block out readers,
    // thereby blocking user queries.
    // RSI: ^ wat.
    scoped_ptr_t<file_account_t> reads_io_account;
    scoped_ptr_t<file_account_t> writes_io_account;

    serializer_t *serializer_;

    // RSP: std::vector bad growth performance
    std::vector<current_page_t *> current_pages_;

    free_list_t free_list_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(page_cache_t);
};




}  // namespace alt

#endif  // BUFFER_CACHE_ALT_PAGE_HPP_
