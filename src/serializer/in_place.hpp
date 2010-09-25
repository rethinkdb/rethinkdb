
#ifndef __IN_PLACE_SERIALIZER_HPP__
#define __IN_PLACE_SERIALIZER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "arch/resource.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"

/* This is a serializer that writes blocks in place. It should be
 * efficient for rotational drives and flash drives with a very good
 * FTL. It's also a good sanity check that the rest of the system
 * isn't tightly coupled with a log-structured serializer. */

struct in_place_serializer_t {
    
    /* Note that in_place_serializer_t doesn't actually implement the entire serializer interface.
    The serializer is supposed to keep track of which block IDs are currently in use, and to
    recycle block IDs that have been deleted. However, in_place_serializer_t is naive and very
    simple, so it doesn't do this. Deleting a block has no effect; the serializer will not complain,
    but it will not reuse the block ID either. */
    
public:
    in_place_serializer_t(char *db_path, size_t _block_size);
    ~in_place_serializer_t();
    
    /* start() must be called before the serializer can be used. It will return 'true' if it is
    ready immediately; otherwise, it will return 'false' and then call the given callback later. */
public:
    struct ready_callback_t {
        ~ready_callback_t() {}
        virtual void on_serializer_ready() = 0;
    };
    bool start(ready_callback_t *ready_cb);
    
    /* do_read() reads the block with the given ID. It returns 'true' if the read completes
    immediately; otherwise, it will return 'false' and call the given callback later. */
public:
    struct read_callback_t : private iocallback_t {
        friend class in_place_serializer_t;
        ~read_callback_t() {}
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
    struct write_block_callback_t;
    struct write_txn_callback_t {
        friend class in_place_serializer_t;
        friend class write_block_callback_t;
        write_txn_callback_t() : blocks_outstanding(0) {}
        ~write_txn_callback_t() {}
        virtual void on_serializer_write_txn() = 0;
    private:
        int blocks_outstanding;
        void on_block_written() {
            blocks_outstanding --;
            if (blocks_outstanding == 0) on_serializer_write_txn();
        }
    };
    struct write_block_callback_t : private iocallback_t {
        friend class in_place_serializer_t;
        write_block_callback_t() : associated_txn(NULL) {}
        ~write_block_callback_t() {}
        virtual void on_serializer_write_block() = 0;
    private:
        write_txn_callback_t *associated_txn;
        void on_io_complete(event_t *unused) {
            on_serializer_write_block();
            associated_txn->on_block_written();
            associated_txn = NULL;
        }
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
    
    /* shutdown() should be called when you are done with the serializer. It is safe to call
    shutdown() before the serializer finishes starting up.
    
    If the shutdown is done immediately, shutdown() will return 'true'. Otherwise, it will return
    'false' and then call the given callback when the shutdown is done. */
public:
    struct shutdown_callback_t {
        ~shutdown_callback_t() {}
        virtual void on_serializer_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);
    
    size_t block_size;

private:
    off64_t id_to_offset(block_id_t id);
    
    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shut_down
    } state;
    
    char db_path[MAX_DB_FILE_NAME];
    direct_file_t *dbfile;
    off64_t dbsize;
};

#endif // __IN_PLACE_SERIALIZER_HPP__

