#ifndef SERIALIZER_LOG_LOG_SERIALIZER_HPP_
#define SERIALIZER_LOG_LOG_SERIALIZER_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#include "serializer/serializer.hpp"
#include "serializer/log/config.hpp"
#include "utils.hpp"
#include "concurrency/mutex.hpp"

#include "serializer/log/metablock_manager.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/lba/lba_list.hpp"
#include "serializer/log/data_block_manager.hpp"

#include "serializer/log/stats.hpp"

class cond_t;
struct block_magic_t;
class io_backender_t;
class log_serializer_t;

/**
 * This is the log-structured serializer, the holiest of holies of
 * RethinkDB. Please treat it with courtesy, professionalism, and
 * respect that it deserves.
 */

struct log_serializer_metablock_t {
    extent_manager_t::metablock_mixin_t extent_manager_part;
    lba_list_t::metablock_mixin_t lba_index_part;
    data_block_manager_t::metablock_mixin_t data_block_manager_part;
    block_sequence_id_t block_sequence_id;
};

//  Data to be serialized to disk with each block.  Changing this changes the disk format!
// TODO: This header data should maybe go to the cache
typedef metablock_manager_t<log_serializer_metablock_t> mb_manager_t;

// Used internally
struct ls_block_writer_t;
struct ls_read_fsm_t;
struct ls_start_new_fsm_t;
struct ls_start_existing_fsm_t;

class log_serializer_t :
#ifndef SEMANTIC_SERIALIZER_CHECK
    public serializer_t,
#else
    public home_thread_mixin_debug_t,
#endif  // SEMANTIC_SERIALIZER_CHECK
    private data_block_manager_t::shutdown_callback_t,
    private lba_list_t::shutdown_callback_t
{
    friend struct ls_block_writer_t;
    friend struct ls_read_fsm_t;
    friend struct ls_start_new_fsm_t;
    friend struct ls_start_existing_fsm_t;
    friend class data_block_manager_t;
    friend class dbm_read_ahead_fsm_t;
    friend class ls_block_token_pointee_t;

public:
    /* Serializer configuration. dynamic_config_t is everything that can be changed from run
    to run; static_config_t is the parameters that are set when the database is created and
    cannot be changed after that. */
    typedef log_serializer_private_dynamic_config_t private_dynamic_config_t;
    typedef log_serializer_dynamic_config_t dynamic_config_t;
    typedef log_serializer_static_config_t static_config_t;

    struct log_serializer_config_t {
        dynamic_config_t dynamic_config;
        private_dynamic_config_t private_dynamic_config;
        static_config_t static_config;

        explicit log_serializer_config_t(const std::string& file_name)
            : private_dynamic_config(file_name)
        { }

        log_serializer_config_t() { }
        RDB_MAKE_ME_SERIALIZABLE_3(dynamic_config, private_dynamic_config, static_config);
    };

    typedef log_serializer_config_t config_t;

public:

    /* Blocks. Does not check for an existing database--use check_existing for that. */
    static void create(io_backender_t *backender, private_dynamic_config_t private_dynamic_config, static_config_t static_config);

    /* Blocks. */
    log_serializer_t(dynamic_config_t dynamic_config, io_backender_t *io_backender, private_dynamic_config_t private_dynamic_config, perfmon_collection_t *perfmon_collection);

    /* Blocks. */
    virtual ~log_serializer_t();

    /* TODO Make this block too instead of using a callback */
    struct check_callback_t {
        virtual void on_serializer_check(bool is_existing) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const char *filename, io_backender_t *io_backend, check_callback_t *cb);

public:
    /* Implementation of the serializer_t API */
    void *malloc();
    void *clone(void*); // clones a buf
    void free(void*);

    file_account_t *make_io_account(int priority, int outstanding_requests_limit);

    void register_read_ahead_cb(serializer_read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb);
    block_id_t max_block_id();
    repli_timestamp_t get_recency(block_id_t id);

    bool get_delete_bit(block_id_t id);
    intrusive_ptr_t<ls_block_token_pointee_t> index_read(block_id_t block_id);

    void block_read(const intrusive_ptr_t<ls_block_token_pointee_t>& token, void *buf, file_account_t *io_account, iocallback_t *cb);

    void block_read(const intrusive_ptr_t<ls_block_token_pointee_t>& token, void *buf, file_account_t *io_account);

    void index_write(const std::vector<index_write_op_t>& write_ops, file_account_t *io_account);

    intrusive_ptr_t<ls_block_token_pointee_t> block_write(const void *buf, block_id_t block_id, file_account_t *io_account, iocallback_t *cb);
    intrusive_ptr_t<ls_block_token_pointee_t> block_write(const void *buf, file_account_t *io_account, iocallback_t *cb);
    intrusive_ptr_t<ls_block_token_pointee_t> block_write(const void *buf, block_id_t block_id, file_account_t *io_account);
    intrusive_ptr_t<ls_block_token_pointee_t> block_write(const void *buf, file_account_t *io_account);

    block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf) const;

    block_size_t get_block_size();

    bool coop_lock_and_check();

private:
    void register_block_token(ls_block_token_pointee_t *token, off64_t offset);
    bool tokens_exist_for_offset(off64_t off);
    void unregister_block_token(ls_block_token_pointee_t *token);
    void remap_block_to_new_offset(off64_t current_offset, off64_t new_offset);
    intrusive_ptr_t<ls_block_token_pointee_t> generate_block_token(off64_t offset);

    bool offer_buf_to_read_ahead_callbacks(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);
    bool should_perform_read_ahead();

    struct index_write_context_t {
        index_write_context_t() : next_metablock_write(NULL) { }
        extent_transaction_t extent_txn;
        cond_t *next_metablock_write;

    private:
        DISABLE_COPYING(index_write_context_t);
    };
    /* Starts a new transaction, updates perfmons etc. */
    void index_write_prepare(index_write_context_t *context, file_account_t *io_account);
    /* Finishes a write transaction */
    void index_write_finish(index_write_context_t *context, file_account_t *io_account);

    /* This mess is because the serializer is still mostly FSM-based */
    bool shutdown(cond_t *cb);
    bool next_shutdown_step();

    virtual void on_datablock_manager_shutdown();
    virtual void on_lba_shutdown();

public:
    // disable_gc should be called when you want to turn off the gc
    // temporarily.
    //
    // disable_gc will ALWAYS eventually call the callback.  It will
    // return 'true' (and will have already called the callback) if gc
    // is stopped immediately.
    typedef data_block_manager_t::gc_disable_callback_t gc_disable_callback_t;

    bool disable_gc(gc_disable_callback_t *cb);

    // enable_gc() should be called when you want to turn on the gc.
    // gc will be enabled immediately.  Always returns 'true' for
    // do_on_thread.  TODO: who uses this return value?
    void enable_gc();

private:
    typedef log_serializer_metablock_t metablock_t;
    void prepare_metablock(metablock_t *mb_buffer);

    void consider_start_gc();

    std::map<ls_block_token_pointee_t *, off64_t> token_offsets;
    std::multimap<off64_t, ls_block_token_pointee_t *> offset_tokens;
    scoped_ptr_t<log_serializer_stats_t> stats;
    perfmon_collection_t disk_stats_collection;
    perfmon_membership_t disk_stats_membership;

    // TODO: Just make this available in release mode?
#ifndef NDEBUG
    // Makes sure we get no tokens after we thought that
    bool expecting_no_more_tokens;
#endif

    std::vector<serializer_read_ahead_callback_t *> read_ahead_callbacks;

    const dynamic_config_t dynamic_config;
    const private_dynamic_config_t private_config;
    static_config_t static_config;

    cond_t *shutdown_callback;

    enum shutdown_state_t {
        shutdown_begin,
        shutdown_waiting_on_serializer,
        shutdown_waiting_on_datablock_manager,
        shutdown_waiting_on_block_tokens,
        shutdown_waiting_on_lba
    } shutdown_state;
    bool shutdown_in_one_shot;

    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shutting_down,
        state_shut_down
    } state;

    const char *const db_path;
    direct_file_t *dbfile;

    extent_manager_t *extent_manager;
    mb_manager_t *metablock_manager;
    lba_list_t *lba_index;
    data_block_manager_t *data_block_manager;

    /* The running index writes organize themselves into a list so that they can be sure to
    write their metablocks in the correct order. last_write points to the most recent
    transaction that started but did not finish; new index writes use it to find the
    end of the list so they can append themselves to it. */
    index_write_context_t *last_write;

    int active_write_count;

    block_sequence_id_t latest_block_sequence_id;

    io_backender_t *const io_backender;

    DISABLE_COPYING(log_serializer_t);
};

#endif /* SERIALIZER_LOG_LOG_SERIALIZER_HPP_ */
