
#ifndef __LOG_SERIALIZER_HPP__
#define __LOG_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <vector>

#include "serializer/serializer.hpp"
#include "server/cmd_args.hpp"
#include "utils.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/mutex.hpp"

class log_serializer_t;

#include "metablock/metablock_manager.hpp"
#include "extents/extent_manager.hpp"
#include "lba/lba_list.hpp"
#include "data_block_manager.hpp"

struct block_magic_t;

/**
 * This is the log-structured serializer, the holiest of holies of
 * RethinkDB. Please treat it with courtesy, professionalism, and
 * respect that it deserves.
 */

typedef lba_list_t lba_index_t;

struct log_serializer_metablock_t {
    extent_manager_t::metablock_mixin_t extent_manager_part;
    lba_index_t::metablock_mixin_t lba_index_part;
    data_block_manager_t::metablock_mixin_t data_block_manager_part;
    ser_transaction_id_t transaction_id;
    ser_block_sequence_id_t block_sequence_id;
};

//  Data to be serialized to disk with each block.  Changing this changes the disk format!
struct ls_buf_data_t {
    /* The following data is not generally available on block_write.
    TODO: Consider alternatives for making extract work again
    */
    //ser_block_id_t block_id;
    //ser_transaction_id_t transaction_id;
    ser_block_sequence_id_t block_sequence_id;
} __attribute__((__packed__));

typedef metablock_manager_t<log_serializer_metablock_t> mb_manager_t;

// Used internally
struct ls_block_writer_t;
struct ls_read_fsm_t;
struct ls_start_new_fsm_t;
struct ls_start_existing_fsm_t;

struct log_serializer_t :
    public serializer_t,
    private data_block_manager_t::gc_writer_t,
    private data_block_manager_t::shutdown_callback_t,
    private lba_index_t::shutdown_callback_t
{
    friend class ls_block_writer_t;
    friend class ls_read_fsm_t;
    friend class ls_start_new_fsm_t;
    friend class ls_start_existing_fsm_t;
    friend class data_block_manager_t;
    friend class dbm_read_ahead_fsm_t;

public:
    /* Serializer configuration. dynamic_config_t is everything that can be changed from run
    to run; static_config_t is the parameters that are set when the database is created and
    cannot be changed after that. */
    typedef log_serializer_private_dynamic_config_t private_dynamic_config_t;
    typedef log_serializer_dynamic_config_t dynamic_config_t;
    typedef log_serializer_static_config_t static_config_t;
    
    dynamic_config_t *dynamic_config;
    private_dynamic_config_t *private_config;
    static_config_t static_config;

public:

    /* Blocks. Does not check for an existing database--use check_existing for that. */
    static void create(dynamic_config_t *dynamic_config, private_dynamic_config_t *private_dynamic_config, static_config_t *static_config);

    /* Blocks. */
    log_serializer_t(dynamic_config_t *dynamic_config, private_dynamic_config_t *private_dynamic_config);

    /* Blocks. */
    virtual ~log_serializer_t();

    /* TODO Make this block too instead of using a callback */
    struct check_callback_t {
        virtual void on_serializer_check(bool is_existing) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const char *filename, check_callback_t *cb);

public:
    class ls_block_token_t : public serializer_t::block_token_t {
        friend class log_serializer_t;

        ls_block_token_t(log_serializer_t *serializer, off64_t initial_offset) : serializer(serializer) {
            serializer->assert_thread();
            serializer->register_block_token(this, initial_offset);
        }
        log_serializer_t *serializer;

    public:
        virtual ~ls_block_token_t() {
            on_thread_t(serializer->home_thread);
            serializer->unregister_block_token(this);
        }
    };

    /* Implementation of the serializer_t API */
    void *malloc();
    void *clone(void*); // clones a buf
    void free(void*);

    void register_read_ahead_cb(read_ahead_callback_t *cb);
    void unregister_read_ahead_cb(read_ahead_callback_t *cb);virtual void block_read(boost::shared_ptr<block_token_t> token, void *buf);
    void index_read(boost::shared_ptr<block_token_t> token, void *buf);
    boost::shared_ptr<block_token_t> index_read(ser_block_id_t block_id);
    void index_write(const std::vector<index_write_op_t*>& write_ops);
    boost::shared_ptr<block_token_t> block_write(const void *buf, iocallback_t *cb);
    ser_block_sequence_id_t get_block_sequence_id(ser_block_id_t block_id, const void* buf);
    block_size_t get_block_size();
    ser_block_id_t max_block_id();
    bool block_in_use(ser_block_id_t id);
    repli_timestamp get_recency(ser_block_id_t id);

private:
    std::map<ls_block_token_t*, off64_t> token_offsets;
    std::multimap<off64_t, ls_block_token_t*> offset_tokens;
    void register_block_token(ls_block_token_t *token, off64_t offset);
    void unregister_block_token(ls_block_token_t *token);
    void remap_block_to_new_offset(off64_t current_offset, off64_t new_offset);

    std::vector<read_ahead_callback_t*> read_ahead_callbacks;
    bool offer_buf_to_read_ahead_callbacks(ser_block_id_t block_id, void *buf);
    bool should_perform_read_ahead();

    struct index_write_context_t {
        index_write_context_t() : next_metablock_write(NULL) { }
        extent_manager_t::transaction_t *extent_txn;
        cond_t *next_metablock_write;
    };
    /* Starts a new transaction, updates perfmons etc. */
    void index_write_prepare(index_write_context_t &context);
    /* Finishes a write transaction */
    void index_write_finish(index_write_context_t &context);

    /* Called by the data block manager when it wants us to rewrite some blocks */
    bool write_gcs(data_block_manager_t::gc_write_t *writes, int num_writes, data_block_manager_t::gc_write_callback_t *cb);

    /* This mess is because the serializer is still mostly FSM-based */
    bool shutdown(cond_t *cb);
    bool next_shutdown_step();
    cond_t *shutdown_callback;
    
    enum shutdown_state_t {
        shutdown_begin,
        shutdown_waiting_on_serializer,
        shutdown_waiting_on_datablock_manager,
        shutdown_waiting_on_lba
    } shutdown_state;
    bool shutdown_in_one_shot;

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

    // The magic value used for "zero" buffers written upon deletion.
    static const block_magic_t zerobuf_magic;

private:
    typedef log_serializer_metablock_t metablock_t;
    void prepare_metablock(metablock_t *mb_buffer);

    void consider_start_gc();

    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shutting_down,
        state_shut_down,
    } state;

    const char *db_path;
    direct_file_t *dbfile;
    
    extent_manager_t *extent_manager;
    mb_manager_t *metablock_manager;
    lba_index_t *lba_index;
    data_block_manager_t *data_block_manager;
    
    /* The running index writes organize themselves into a list so that they can be sure to
    write their metablocks in the correct order. last_write points to the most recent
    transaction that started but did not finish; new index writes use it to find the
    end of the list so they can append themselves to it. */
    index_write_context_t *last_write;
    
    int active_write_count;

    ser_transaction_id_t current_transaction_id;
    ser_block_sequence_id_t latest_block_sequence_id;
#ifndef NDEBUG
    metablock_t debug_mb_buffer;
#endif
};

#endif /* __LOG_SERIALIZER_HPP__ */
