#ifndef SERIALIZER_SEMANTIC_CHECKING_HPP_
#define SERIALIZER_SEMANTIC_CHECKING_HPP_

#include <sys/types.h>
#include <sys/stat.h>

#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/crc.hpp>

#include "arch/types.hpp"
#include "config/args.hpp"
#include "containers/two_level_array.hpp"
#include "serializer/serializer.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

//#define SERIALIZER_DEBUG_PRINT 1

// TODO make this just an interface and move implementation into semantic_checking.tcc, then use
// tim's template hack to instantiate as necessary in a .cc file.

struct scs_block_info_t;
struct scs_persisted_block_info_t;

template<class inner_serializer_t>
class semantic_checking_serializer_t :
    public serializer_t
{
private:
    struct reader_t;

    inner_serializer_t inner_serializer;
    two_level_array_t<scs_block_info_t, MAX_BLOCK_ID> blocks;
    int last_index_write_started, last_index_write_finished;
    int semantic_fd;
    std::set<const void *> malloced_bufs;

    // Helper functions
    uint32_t compute_crc(const void *buf);
    void update_block_info(block_id_t block_id, scs_block_info_t info);

    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > wrap_token(block_id_t block_id, scs_block_info_t info, intrusive_ptr_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token);
    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > wrap_buf_token(block_id_t block_id, const void *buf, intrusive_ptr_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token);

    void read_check_state(scs_block_token_t<inner_serializer_t> *token, const void *buf);

public:
    typedef typename inner_serializer_t::private_dynamic_config_t private_dynamic_config_t;
    typedef typename inner_serializer_t::dynamic_config_t dynamic_config_t;
    typedef typename inner_serializer_t::static_config_t static_config_t;
    typedef typename inner_serializer_t::config_t config_t;

    static void create(dynamic_config_t config, io_backender_t *backender, private_dynamic_config_t private_config, static_config_t static_config);
    semantic_checking_serializer_t(dynamic_config_t config, io_backender_t *backender, private_dynamic_config_t private_config);
    ~semantic_checking_serializer_t();

    typedef typename inner_serializer_t::check_callback_t check_callback_t;
    static void check_existing(const char *db_path, check_callback_t *cb);

    void *malloc();
    void *clone(void *data);
    void free(void *ptr);

    file_account_t *make_io_account(int priority, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > index_read(block_id_t block_id);

    void block_read(const intrusive_ptr_t< scs_block_token_t<inner_serializer_t> >& token_, void *buf, file_account_t *io_account, iocallback_t *callback);
    void block_read(const intrusive_ptr_t< scs_block_token_t<inner_serializer_t> >& token_, void *buf, file_account_t *io_account);

    block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf);

    void index_write(const std::vector<index_write_op_t>& write_ops, file_account_t *io_account);

    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > block_write(const void *buf, block_id_t block_id, file_account_t *io_account, iocallback_t *cb);
    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > block_write(const void *buf, file_account_t *io_account, iocallback_t *cb);
    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > block_write(const void *buf, block_id_t block_id, file_account_t *io_account);
    intrusive_ptr_t< scs_block_token_t<inner_serializer_t> > block_write(const void *buf, file_account_t *io_account);

    block_size_t get_block_size();

    bool coop_lock_and_check();

    block_id_t max_block_id();

    repli_timestamp_t get_recency(block_id_t id);
    bool get_delete_bit(block_id_t id);

    void register_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(UNUSED serializer_read_ahead_callback_t *cb);

public:
    typedef typename inner_serializer_t::gc_disable_callback_t gc_disable_callback_t;
    bool disable_gc(gc_disable_callback_t *cb);
    void enable_gc();

};

#endif /* SERIALIZER_SEMANTIC_CHECKING_HPP_ */
