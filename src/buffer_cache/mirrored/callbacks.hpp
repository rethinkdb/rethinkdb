#ifndef __BUFFER_CACHE_CALLBACKS_HPP__
#define __BUFFER_CACHE_CALLBACKS_HPP__

class mc_buf_t;
class mc_transaction_t;
class mc_cache_t;

struct mc_block_available_callback_t
{
    virtual ~mc_block_available_callback_t() {}
    virtual void on_block_available(mc_buf_t *buf) = 0;
};

struct mc_transaction_begin_callback_t {
    virtual ~mc_transaction_begin_callback_t() {}
    virtual void on_txn_begin(mc_transaction_t *txn) = 0;
};

struct mc_transaction_commit_callback_t {
    virtual ~mc_transaction_commit_callback_t() {}
    virtual void on_txn_commit(mc_transaction_t *txn) = 0;
};

class get_subtree_recencies_callback_t {
public:
    virtual void got_subtree_recencies() = 0;
protected:
    ~get_subtree_recencies_callback_t() { }
};

#endif // __BUFFER_CACHE_CALLBACKS_HPP__
