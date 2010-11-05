
#ifndef __LOG_SERIALIZER_HPP__
#define __LOG_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>

#include "serializer/serializer.hpp"
#include "config/cmd_args.hpp"
#include "config/alloc.hpp"
#include "utils.hpp"

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

/*
TODO: Consider the following situation:
1. Block A is stored at address X.
2. Client issues a read for block A at address X. It gets hung up in the OS somewhere.
3. Client issues a write for block A. Address Y is chosen. The write completes quickly.
4. The garbage collector recognizes that block A is no longer at address X, so it releases the
    extent containing address X.
5. Client issues a write for block B. Address X, which is now free, is chosen. The write completes
    quickly.
6. The read from step #2 finally gets performed, but because block B is now at address X, it gets
    the contents of block B instead of block A.
*/

/*
TODO: Consider the following situation:
1. The data block manager's current extent is X. From X to X+Y have been filled.
2. The data block manager fills the range from X+Y to X+Y+Z.
3. The server crashes before the metablock has been written
4. On restart, the server only remembers that there is data from X to X+Y.
5. The data block manager re-fills the range from X+Y to X+Y+Z.
6. The disk experiences fragmentation, possibly causing a slowdown.
*/

typedef lba_list_t lba_index_t;

struct log_serializer_metablock_t {
    extent_manager_t::metablock_mixin_t extent_manager_part;
    lba_index_t::metablock_mixin_t lba_index_part;
    data_block_manager_t::metablock_mixin_t data_block_manager_part;
    ser_transaction_id_t transaction_id;
};

typedef metablock_manager_t<log_serializer_metablock_t> mb_manager_t;

// Used internally
struct ls_block_writer_t;
struct ls_write_fsm_t;
struct ls_read_fsm_t;
struct ls_start_new_fsm_t;
struct ls_start_existing_fsm_t;

struct log_serializer_t :
    public serializer_t,
    private data_block_manager_t::shutdown_callback_t,
    private lba_index_t::shutdown_callback_t
{
    friend class ls_block_writer_t;
    friend class ls_write_fsm_t;
    friend class ls_read_fsm_t;
    friend class ls_start_new_fsm_t;
    friend class ls_start_existing_fsm_t;

public:
    /* Serializer configuration. dynamic_config_t is everything that can be changed from run
    to run; static_config_t is the parameters that are set when the database is created and
    cannot be changed after that. */
    typedef log_serializer_dynamic_config_t dynamic_config_t;
    typedef log_serializer_static_config_t static_config_t;
    
    dynamic_config_t *dynamic_config;
    static_config_t static_config;

public:
    log_serializer_t(const char *filename, dynamic_config_t *dynamic_config);
    virtual ~log_serializer_t();

public:
    /* start_new() or start_existing() must be called before the serializer can be used. If
    start_new() is called, a new database will be created. If start_existing() is called, the
    serializer will read from an existing database. */
    struct ready_callback_t {
        virtual void on_serializer_ready(log_serializer_t *) = 0;
    };
    bool start_new(static_config_t *static_config, ready_callback_t *ready_cb);
    bool start_existing(ready_callback_t *ready_cb);

public:
    /* Implementation of the serializer_t API */
    void *malloc();
    void free(void*);
    bool do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback);
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback);
    size_t get_block_size();
    ser_block_id_t max_block_id();
    bool block_in_use(ser_block_id_t id);

public:
    /* shutdown() should be called when you are done with the serializer.
    
    If the shutdown is done immediately, shutdown() will return 'true'. Otherwise, it will return
    'false' and then call the given callback when the shutdown is done. */
    struct shutdown_callback_t {
        virtual void on_serializer_shutdown(log_serializer_t *) = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

private:
    bool next_shutdown_step();
    shutdown_callback_t *shutdown_callback;
    
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
    // do_on_cpu.
    bool enable_gc();

public:
    // The magic value used for "zero" buffers written upon deletion.
    static const block_magic_t zerobuf_magic;

private:
    typedef log_serializer_metablock_t metablock_t;
    void prepare_metablock(metablock_t *mb_buffer);

    void consider_start_gc();

private:
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
    
    /* The ls_write_fsm_ts organize themselves into a list so that they can be sure to
    write their metablocks in the correct order. last_write points to the most recent
    transaction that started but did not finish; new ls_write_fsm_ts use it to find the
    end of the list so they can append themselves to it. */
    ls_write_fsm_t *last_write;
    
    int active_write_count;


    ser_transaction_id_t current_transaction_id;
    
    /* Keeps track of buffers that are currently being written, so that if we get a read
    for a block ID that we are currently writing but is not on disk yet, we can return
    the most current version. */
    typedef std::map<
        ser_block_id_t, ls_block_writer_t*,
        std::less<ser_block_id_t>,
        gnew_alloc<std::pair<ser_block_id_t, ls_block_writer_t*> >
        > block_writer_map_t;
    block_writer_map_t block_writer_map;

#ifndef NDEBUG
    metablock_t debug_mb_buffer;
#endif
};

#endif /* __LOG_SERIALIZER_HPP__ */
