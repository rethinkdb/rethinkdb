
#ifndef __LOG_SERIALIZER_HPP__
#define __LOG_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "arch/resource.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"

#include "metablock/naive.hpp"
#include "extents/extent_manager.hpp"
#include "lba/lba_list.hpp"
#include "data_block_manager.hpp"

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
};

typedef naive_metablock_manager_t<log_serializer_metablock_t> metablock_manager_t;

// Used internally
struct ls_write_fsm_t;
struct ls_start_fsm_t;

struct log_serializer_t
{
    friend class ls_write_fsm_t;
    friend class ls_start_fsm_t;
    
public:
    log_serializer_t(char *db_path, size_t _block_size);
    ~log_serializer_t();

    /* start() must be called before the serializer can be used. It will return 'true' if it is
    ready immediately; otherwise, it will return 'false' and then call the given callback later. */
public:
    struct ready_callback_t {
        virtual void on_serializer_ready() = 0;
    };
    bool start(ready_callback_t *ready_cb);

    /* do_read() reads the block with the given ID. It returns 'true' if the read completes
    immediately; otherwise, it will return 'false' and call the given callback later. */
public:
    struct read_callback_t : private iocallback_t {
        friend class log_serializer_t;
        virtual void on_serializer_read() = 0;
    private:
        void on_io_complete(event_t *unused) {
            on_serializer_read();
        }
    };
    bool do_read(block_id_t block_id, void *buf, read_callback_t *callback);
    
    /* do_write() updates or deletes a group of bufs.
    
    Each write_t passed to do_write() identifies an update or deletion. If 'buf' is NULL, then it
    represents a deletion. If 'buf' is non-NULL, then it identifies an update, and the given
    callback will be called as soon as the data has been copied out of 'buf'. If the entire
    transaction completes immediately, it will return 'true'; otherwise, it will return 'false' and
    call the given callback at a later date.
    
    'writes' can be freed as soon as do_write() returns. */
public:
    struct write_txn_callback_t {
        virtual void on_serializer_write_txn() = 0;
    };
    struct write_block_callback_t {
        virtual void on_serializer_write_block() = 0;
    };
    struct write_t {
        block_id_t block_id;
        void *buf;
        write_block_callback_t *callback;
    };
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback);
    
public:
    /* Generates a unique block id. */
    block_id_t gen_block_id();
    
    /* shutdown() should be called when you are done with the serializer. You should only call
    shutdown() when the serializer has finished starting up and there are no active write
    operations.
    
    If the shutdown is done immediately, shutdown() will return 'true'. Otherwise, it will return
    'false' and then call the given callback when the shutdown is done. */
public:
    struct shutdown_callback_t {
        virtual void on_serializer_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

public:
    size_t block_size;

private:
    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shut_down
    } state;
    
    char db_path[MAX_DB_FILE_NAME];
    fd_t dbfd;
    
    typedef log_serializer_metablock_t metablock_t;
    
    extent_manager_t extent_manager;
    metablock_manager_t metablock_manager;
    lba_index_t lba_index;
    data_block_manager_t data_block_manager;
    
    int active_write_count;
};

#endif /* __LOG_SERIALIZER_HPP__ */
