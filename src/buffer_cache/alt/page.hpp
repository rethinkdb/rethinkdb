#ifndef BUFFER_CACHE_ALT_PAGE_HPP_
#define BUFFER_CACHE_ALT_PAGE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"
#include "serializer/types.hpp"

namespace alt {

class current_page_acq_t;
class page_acq_t;


enum class alt_access_t { read, write };

class page_t {
public:
    page_t();
    ~page_t();

    void *get_buf();
    uint32_t get_buf_size();

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
    current_page_t();
    ~current_page_t();

private:
    intrusive_list_t<current_page_acq_t> acquirers_;
    DISABLE_COPYING(current_page_t);
};

class current_page_acq_t : public intrusive_list_node_t<current_page_acq_t> {
public:
    current_page_acq_t(current_page_t *current_page, alt_access_t access);
    ~current_page_acq_t();

    signal_t *read_acq_signal();
    signal_t *write_acq_signal();

    page_t *page_for_read();
    page_t *page_for_write();

private:
    alt_access_t access_;
    current_page_t *current_page_;
    page_t *snapshotted_page_;
    cond_t read_cond_;
    cond_t write_cond_;

    DISABLE_COPYING(current_page_acq_t);
};

class page_cache_t {
public:
    page_cache_t(serializer_t *serializer);
    current_page_t *page_for_block_id(block_id_t block_id);
    current_page_t *page_for_new_block_id(block_id_t *block_id_out);



private:
    DISABLE_COPYING(page_cache_t);
};




}  // namespace alt

#endif  // BUFFER_CACHE_ALT_PAGE_HPP_
