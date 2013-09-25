#ifndef BUFFER_CACHE_ALT_PAGE_HPP_
#define BUFFER_CACHE_ALT_PAGE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"
#include "serializer/types.hpp"

class auto_drainer_t;

namespace alt {

class current_page_acq_t;
class page_acq_t;


enum class alt_access_t { read, write };

class page_t {
public:
    page_t(block_id_t block_id, serializer_t *serializer);
    ~page_t();

    void *get_buf();
    uint32_t get_buf_size();

    void remove_snapshotter(current_page_acq_t *acq);

private:
    bool *destroy_ptr_;
    block_size_t buf_size_;
    scoped_malloc_t<ser_buffer_t> buf_;
    counted_t<standard_block_token_t> block_token_;

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
    cond_t buf_ready_signal_;
    DISABLE_COPYING(page_acq_t);
};

class current_page_t {
public:
    current_page_t(block_id_t block_id, serializer_t *serializer);
    ~current_page_t();


private:
    // current_page_acq_t should not access our fields directly.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);
    void pulse_pulsables(current_page_acq_t *acq);

    page_t *the_page_for_write();
    page_t *the_page_for_read();

    // Our block id.
    block_id_t block_id_;
    // Either page_ is null or serializer_ is null.  block_id_ and serializer_ can be
    // used to construct (and load) the page when it's null.
    // RSP:  This is a waste of memory.
    serializer_t *serializer_;
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
    serializer_t *serializer_;

    // RSP: std::vector bad growth performance
    std::vector<current_page_t *> current_pages_;

    free_list_t free_list_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(page_cache_t);
};




}  // namespace alt

#endif  // BUFFER_CACHE_ALT_PAGE_HPP_
