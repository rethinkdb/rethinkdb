
#ifndef __BUFFER_CACHE_CALLBACKS_HPP__
#define __BUFFER_CACHE_CALLBACKS_HPP__

template <class config_t>
struct block_available_callback : public intrusive_list_node_t<block_available_callback<config_t> > {
public:
    typedef typename config_t::buf_t buf_t;

public:
    virtual ~block_available_callback() {}
    virtual void on_block_available(buf_t *buf) = 0;
};

template <class config_t>
struct transaction_begin_callback {
    virtual ~transaction_begin_callback() {}
    virtual void on_txn_begin(typename config_t::transaction_t *txn) = 0;
};

template <class config_t>
struct transaction_commit_callback {
    virtual ~transaction_commit_callback() {}
    virtual void on_txn_commit(typename config_t::transaction_t *txn) = 0;
};

template <class config_t>
struct sync_callback {
    virtual ~sync_callback() {}
    virtual void on_sync() = 0;
};

#endif // __BUFFER_CACHE_CALLBACKS_HPP__

