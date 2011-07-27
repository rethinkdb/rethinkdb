#ifndef __SERIALIZER_SEMANTIC_CHECKING_HPP__
#define __SERIALIZER_SEMANTIC_CHECKING_HPP__

#include "errors.hpp"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/crc.hpp>
#include "arch/arch.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "containers/two_level_array.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

//#define SERIALIZER_DEBUG_PRINT 1

// TODO make this just an interface and move implementation into semantic_checking.tcc, then use
// tim's template hack to instantiate as necessary in a .cc file.

struct scc_block_info_t;
struct scc_persisted_block_info_t;

template<class inner_serializer_t>
class semantic_checking_serializer_t :
    public serializer_t
{
private:
    struct scc_block_token_t;
    struct reader_t;

    inner_serializer_t inner_serializer;
    two_level_array_t<scc_block_info_t, MAX_BLOCK_ID> blocks;
    int last_index_write_started, last_index_write_finished;
    int semantic_fd;

    // Helper functions
    uint32_t compute_crc(const void *buf);
    void update_block_info(block_id_t block_id, scc_block_info_t info);
    boost::shared_ptr<block_token_t> wrap_token(block_id_t block_id, scc_block_info_t info, boost::shared_ptr<block_token_t> inner_token);
    boost::shared_ptr<block_token_t> wrap_buf_token(block_id_t block_id, const void *buf, boost::shared_ptr<block_token_t> inner_token);
    void read_check_state(scc_block_token_t *token, const void *buf);

public:
    typedef typename inner_serializer_t::private_dynamic_config_t private_dynamic_config_t;
    typedef typename inner_serializer_t::dynamic_config_t dynamic_config_t;
    typedef typename inner_serializer_t::static_config_t static_config_t;
    typedef typename inner_serializer_t::public_static_config_t public_static_config_t;

    static void create(dynamic_config_t *config, private_dynamic_config_t *private_config, public_static_config_t *static_config);
    semantic_checking_serializer_t(dynamic_config_t *config, private_dynamic_config_t *private_config);
    ~semantic_checking_serializer_t();

    typedef typename inner_serializer_t::check_callback_t check_callback_t;
    static void check_existing(const char *db_path, check_callback_t *cb);

    void *malloc();
    void *clone(void *data);
    void free(void *ptr);

    file_t::account_t *make_io_account(int priority, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    boost::shared_ptr<block_token_t> index_read(block_id_t block_id);

    void block_read(boost::shared_ptr<block_token_t> token_, void *buf, file_t::account_t *io_account, iocallback_t *callback);
    void block_read(boost::shared_ptr<block_token_t> token_, void *buf, file_t::account_t *io_account);

    block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf);

    void index_write(const std::vector<index_write_op_t>& write_ops, file_t::account_t *io_account);

    boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account, iocallback_t *cb);
    boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account, iocallback_t *cb);
    boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account);
    boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account);

    block_size_t get_block_size();

    block_id_t max_block_id();

    repli_timestamp_t get_recency(block_id_t id);
    bool get_delete_bit(block_id_t id);

    void register_read_ahead_cb(UNUSED read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(UNUSED read_ahead_callback_t *cb);

public:
    typedef typename inner_serializer_t::gc_disable_callback_t gc_disable_callback_t;
    bool disable_gc(gc_disable_callback_t *cb);
    void enable_gc();

};

#endif /* __SERIALIZER_SEMANTIC_CHECKING_HPP__ */
