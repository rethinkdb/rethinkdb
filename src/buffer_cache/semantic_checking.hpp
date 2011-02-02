#ifndef __BUFFER_CACHE_SEMANTIC_CHECKING_HPP__
#define __BUFFER_CACHE_SEMANTIC_CHECKING_HPP__

#include "buffer_cache/types.hpp"
#include "utils.hpp"
#include <boost/crc.hpp>
#include "containers/two_level_array.hpp"

/* The semantic-checking cache (scc_cache_t) is a wrapper around another cache that will
make sure that the inner cache obeys the proper semantics. */

template<class inner_cache_t> class scc_buf_t;
template<class inner_cache_t> class scc_transaction_t;
template<class inner_cache_t> class scc_cache_t;

typedef uint32_t crc_t;

/* Callbacks */

template<class inner_cache_t>
struct scc_transaction_begin_callback_t {
    virtual void on_txn_begin(scc_transaction_t<inner_cache_t> *txn) = 0;
    virtual ~scc_transaction_begin_callback_t() {}
};

template<class inner_cache_t>
struct scc_transaction_commit_callback_t {
    virtual void on_txn_commit(scc_transaction_t<inner_cache_t> *txn) = 0;
    virtual ~scc_transaction_commit_callback_t() {}
};

template<class inner_cache_t>
struct scc_block_available_callback_t {
    virtual void on_block_available(scc_buf_t<inner_cache_t> *buf) = 0;
    virtual ~scc_block_available_callback_t() {}
};

/* Buf */

template<class inner_cache_t>
class scc_buf_t :
    private inner_cache_t::block_available_callback_t
{
    typedef scc_block_available_callback_t<inner_cache_t> block_available_callback_t;

public:
    block_id_t get_block_id();
    bool is_dirty();
    const void *get_data_read();
    void *get_data_write();
    void mark_deleted();
    void release();

private:
    friend class scc_transaction_t<inner_cache_t>;
    typename inner_cache_t::buf_t *inner_buf;
    void on_block_available(typename inner_cache_t::buf_t *buf);
    block_available_callback_t *available_cb;
    explicit scc_buf_t(scc_cache_t<inner_cache_t> *);
    scc_cache_t<inner_cache_t> *cache;
private:
    crc_t compute_crc() {
        boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
        crc_computer.process_bytes((void *) inner_buf->get_data_read(), cache->get_block_size().value());
        return crc_computer.checksum();
    }
};

/* Transaction */

template<class inner_cache_t>
class scc_transaction_t :
    private inner_cache_t::transaction_begin_callback_t,
    private inner_cache_t::transaction_commit_callback_t,
    public home_thread_mixin_t
{
    typedef scc_buf_t<inner_cache_t> buf_t;
    typedef scc_transaction_begin_callback_t<inner_cache_t> transaction_begin_callback_t;
    typedef scc_transaction_commit_callback_t<inner_cache_t> transaction_commit_callback_t;
    typedef scc_block_available_callback_t<inner_cache_t> block_available_callback_t;

public:
    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback);
    buf_t *allocate();
    repli_timestamp get_subtree_recency(block_id_t block_id);

    scc_cache_t<inner_cache_t> *cache;

private:
    friend class scc_cache_t<inner_cache_t>;
    scc_transaction_t(access_t, scc_cache_t<inner_cache_t> *);
    access_t access;
    void on_txn_begin(typename inner_cache_t::transaction_t *txn);
    transaction_begin_callback_t *begin_cb;
    void on_txn_commit(typename inner_cache_t::transaction_t *txn);
    transaction_commit_callback_t *commit_cb;
    typename inner_cache_t::transaction_t *inner_transaction;
};

/* Cache */

template<class inner_cache_t>
class scc_cache_t : public home_thread_mixin_t {
public:
    typedef scc_buf_t<inner_cache_t> buf_t;
    typedef scc_transaction_t<inner_cache_t> transaction_t;
    typedef scc_transaction_begin_callback_t<inner_cache_t> transaction_begin_callback_t;
    typedef scc_transaction_commit_callback_t<inner_cache_t> transaction_commit_callback_t;
    typedef scc_block_available_callback_t<inner_cache_t> block_available_callback_t;
    
    scc_cache_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *config);
    
    typedef typename inner_cache_t::ready_callback_t ready_callback_t;
    bool start(ready_callback_t *cb);
    
    block_size_t get_block_size();
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback);
    
    typedef typename inner_cache_t::shutdown_callback_t shutdown_callback_t;
    bool shutdown(shutdown_callback_t *cb);

private:
    inner_cache_t inner_cache;

private:
    friend class scc_transaction_t<inner_cache_t>;
    friend class scc_buf_t<inner_cache_t>;

    /* CRC checking stuff */
    two_level_array_t<crc_t, MAX_BLOCK_ID> crc_map;
};

#include "buffer_cache/semantic_checking.tcc"

#endif /* __BUFFER_CACHE_SEMANTIC_CHECKING_HPP__ */

