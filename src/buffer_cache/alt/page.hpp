#ifndef BUFFER_CACHE_ALT_PAGE_HPP_
#define BUFFER_CACHE_ALT_PAGE_HPP_

namespace alt {

class page_t {
public:

    void *get_buf();

private:


    DISABLE_COPYING(page_t);
};

class page_acq_t : public intrusive_list_node_t<page_acq_t> {
public:
    page_acq_t();
    ~page_acq_t();

    void init(page_t *page);

    signal_t *buf_ready_signal();

private:
    cond_t buf_ready_signal_;
    DISABLE_COPYING(page_acq_t);
};

class current_page_t {
public:

private:
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
