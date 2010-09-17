
#ifndef __LOG_SERIALIZER_HPP__
#define __LOG_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>
#include <map>

#include "arch/resource.hpp"
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"

#include "log_serializer_callbacks.hpp"
#include "metablock/metablock_manager.hpp"
#include "extents/extent_manager.hpp"
#include "lba/lba_list.hpp"
#include "data_block_manager.hpp"

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
};

typedef metablock_manager_t<log_serializer_metablock_t> mb_manager_t;

// Used internally
struct ls_block_writer_t;
struct ls_write_fsm_t;
struct ls_start_fsm_t;

struct log_serializer_t
{
    friend class ls_block_writer_t;
    friend class ls_write_fsm_t;
    friend class ls_start_fsm_t;
    
public:
    log_serializer_t(char *db_path, size_t _block_size);
    ~log_serializer_t();

public:
    /* data to be serialized with each data block */
    typedef data_block_manager_t::buf_data_t buf_data_t;

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
    typedef _write_txn_callback_t write_txn_callback_t;

    typedef _write_block_callback_t write_block_callback_t;
    
    struct write_t {
        block_id_t block_id;
        void *buf;   /* If NULL, a deletion */
        write_block_callback_t *callback;
    };
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback);
    void do_outstanding_writes();

private:
    struct write_record_t /* : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, write_record_t> */ {
        write_t *writes;
        int num_writes;
        write_txn_callback_t *callback;
        write_record_t(write_t *writes, int num_writes, write_txn_callback_t *callback)
            : writes(writes), num_writes(num_writes), callback(callback)
        {}
        ~write_record_t()
        {}
    };
    std::queue<write_record_t , std::deque<write_record_t , gnew_alloc<write_record_t> > > outstanding_writes;
    
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
        state_write, /* !< doing a write */
        state_shut_down,
    } state;

    int gc_counter; /* ticks off when it's time to do gc */
    
    char db_path[MAX_DB_FILE_NAME];
    fd_t dbfd;
    
    typedef log_serializer_metablock_t metablock_t;
    
    extent_manager_t extent_manager;
    mb_manager_t metablock_manager;
    data_block_manager_t data_block_manager;
    lba_index_t lba_index;
    
    int active_write_count;   // For debugging
    
    /* Keeps track of buffers that are currently being written, so that if we get a read
    for a block ID that we are currently writing but is not on disk yet, we can return
    the most current version. */
    typedef std::map<
        block_id_t, ls_block_writer_t*,
        std::less<block_id_t>,
        gnew_alloc<std::pair<block_id_t, ls_block_writer_t*> >
        > block_writer_map_t;
    block_writer_map_t block_writer_map;
};

#endif /* __LOG_SERIALIZER_HPP__ */
