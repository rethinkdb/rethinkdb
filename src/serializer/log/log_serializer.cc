#include "log_serializer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

log_serializer_t::log_serializer_t(char *_db_path, size_t block_size)
    : block_size(block_size),
      state(state_unstarted),
      dbfd(INVALID_FD),
      extent_manager(EXTENT_SIZE),
      metablock_manager(&extent_manager),
      lba_index(&extent_manager),
      data_block_manager(&extent_manager, block_size),
      active_write_count(0) {
    
    assert(strlen(_db_path) <= MAX_DB_FILE_NAME - 1);   // "- 1" for the null-terminator
    strncpy(db_path, _db_path, MAX_DB_FILE_NAME);
}

log_serializer_t::~log_serializer_t() {
    assert(state == state_unstarted || state == state_shut_down);
    assert(active_write_count == 0);
}

/* The process of starting up the serializer is handled by the ls_start_fsm_t. This is not
necessary, because there is only ever one startup process for each serializer; the serializer could
handle its own startup process. It is done this way to make it clear which parts of the serializer
are involved in startup and which parts are not. */

struct ls_start_fsm_t :
    public mb_manager_t::metablock_read_callback_t,
    public lba_index_t::ready_callback_t,
    public log_serializer_t::write_txn_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, ls_start_fsm_t >
{
    
    ls_start_fsm_t(log_serializer_t *serializer)
        : ser(serializer), state(state_start) {
        
        // TODO: Allocation
        initial_superblock = (char *)malloc_aligned(serializer->block_size, DEVICE_BLOCK_SIZE);
        bzero(initial_superblock, serializer->block_size);
    }
    
    ~ls_start_fsm_t() {
    
        free(initial_superblock);
    }
    
    bool run(log_serializer_t::ready_callback_t *ready_cb) {
        
        assert(state == state_start);
        assert(ser->state == log_serializer_t::state_unstarted);
        ser->state = log_serializer_t::state_starting_up;

        struct stat file_stat;
        bzero((void*)&file_stat, sizeof(file_stat)); // make valgrind happy
        stat(ser->db_path, &file_stat);
        
        // Open the DB file
        // TODO: O_NOATIME requires root or owner priviledges, so for now we hack it.
        int flags = O_RDWR | O_CREAT | O_DIRECT | O_LARGEFILE;
        if (!S_ISBLK(file_stat.st_mode)) flags |= O_NOATIME;
        
        ser->dbfd = open(ser->db_path, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        check("Could not open database file", ser->dbfd == -1);
        
        state = state_find_metablock;
        ready_callback = NULL;
        if (next_starting_up_step()) {
            return true;
        } else {
            ready_callback = ready_cb;
            return false;
        }
    }
    
    bool next_starting_up_step() {
    
        if (state == state_find_metablock) {
            if (ser->metablock_manager.start(ser->dbfd, &metablock_found, &metablock_buffer, this)) {
                state = state_start_lba;
            } else {
                state = state_waiting_for_metablock;
                return false;
            }
        }
        
        if (state == state_start_lba) {
            if (metablock_found) {
                ser->extent_manager.start(ser->dbfd, &metablock_buffer.extent_manager_part);
                ser->data_block_manager.start(ser->dbfd, &metablock_buffer.data_block_manager_part);
                if (ser->lba_index.start(ser->dbfd, &metablock_buffer.lba_index_part, this)) {
                    state = state_finish;
                } else {
                    state = state_waiting_for_lba;
                    return false;
                }
            } else {
                ser->extent_manager.start(ser->dbfd);
                ser->data_block_manager.start(ser->dbfd);
                ser->lba_index.start(ser->dbfd);
                state = state_write_initial_superblock;
            }
        }
        
        if (state == state_write_initial_superblock) {
            
            log_serializer_t::write_t w;
            w.block_id = SUPERBLOCK_ID;
            w.buf = initial_superblock;
            w.callback = NULL;
            
            ser->state = log_serializer_t::state_ready;   // Backdoor around do_write()'s assertion
            bool write_done = ser->do_write(&w, 1, this);
            ser->state = log_serializer_t::state_starting_up;
            
            if (write_done) {
                state = state_finish;
            } else {
                state = state_waiting_for_initial_superblock;
                return false;
            }
        }
        
        if (state == state_finish) {
            state = state_done;
            assert(ser->state == log_serializer_t::state_starting_up);
            ser->state = log_serializer_t::state_ready;
            
            if (ready_callback) ready_callback->on_serializer_ready();
            
            delete this;
            
            return true;
        }
        
        fail("Invalid state.");
    }
    
    void on_metablock_read() {
        assert(state == state_waiting_for_metablock);
        state = state_start_lba;
        next_starting_up_step();
    }
    
    void on_lba_ready() {
        assert(state == state_waiting_for_lba);
        state = state_finish;
        next_starting_up_step();
    }
    
    void on_serializer_write_txn() {
        assert(state == state_waiting_for_initial_superblock);
        state = state_finish;
        next_starting_up_step();
    }
    
    log_serializer_t *ser;
    log_serializer_t::ready_callback_t *ready_callback;
    
    enum state_t {
        state_start,
        state_find_metablock,
        state_waiting_for_metablock,
        state_start_lba,
        state_waiting_for_lba,
        state_write_initial_superblock,
        state_waiting_for_initial_superblock,
        state_finish,
        state_done
    } state;
    
    bool metablock_found;
    log_serializer_t::metablock_t metablock_buffer;
    
    char *initial_superblock;
};

bool log_serializer_t::start(ready_callback_t *ready_cb) {

    assert(state == state_unstarted);
    
    ls_start_fsm_t *s = new ls_start_fsm_t(this);
    return s->run(ready_cb);
}

/* Each transaction written is handled by a new ls_write_fsm_t instance. This is so that
multiple writes can be handled concurrently -- if more than one cache uses the same serializer,
one cache should be able to start a flush before the previous cache's flush is done.

Within a transaction, each individual change is handled by a new ls_block_writer_t.*/

struct ls_block_writer_t :
    public iocallback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, ls_block_writer_t >
{
    log_serializer_t *ser;
    log_serializer_t::write_t write;
    iocallback_t *extra_cb;
    
    /* true if another write came along and changed the same buf again before we
    finished writing to disk. */
    bool superceded;
    
    ls_block_writer_t(
        log_serializer_t *ser,
        const log_serializer_t::write_t &write)
        : ser(ser), write(write) { }
    
    bool run(iocallback_t *cb) {
        extra_cb = NULL;
        if (do_write()) {
            return true;
        } else {
            extra_cb = cb;
            return false;
        }
    }
    
    bool do_write() {
        
        /* If there was another write currently in progress for the same block, then
        remove it from the block map because we are superceding it */
        log_serializer_t::block_writer_map_t::iterator it = ser->block_writer_map.find(write.block_id);
        if (it != ser->block_writer_map.end()) {
            ls_block_writer_t *writer_we_are_superceding = (*it).second;
            writer_we_are_superceding->superceded = true;
            ser->block_writer_map.erase(it);
        }
        
        if (write.buf) {
        
            off64_t new_offset;
            bool done = ser->data_block_manager.write(write.buf, &new_offset, this);
            ser->lba_index.set_block_offset(write.block_id, new_offset);
            
            /* Insert ourselves into the block_writer_map so that if a reader comes looking for the
            block before we finish writing it to disk, it will be able to find us to get the most
            recent version */
            ser->block_writer_map[write.block_id] = this;
            superceded = false;
            
            if (done) return do_finish();
            else return false;
        
        } else {
        
            /* Deletion */
            ser->lba_index.delete_block(write.block_id);
            
            return do_finish();
        }
    }
    
    void on_io_complete(event_t *e) {
        do_finish();
    }
    
    bool do_finish() {
        
        /* Now that the block is safely on disk, we remove ourselves from the block_writer_map; if
        a reader comes along looking for the block, it will get it from disk. */
        if (write.buf && !superceded) {
            log_serializer_t::block_writer_map_t::iterator it = ser->block_writer_map.find(write.block_id);
            assert((*it).second == this);
            ser->block_writer_map.erase(it);
        }
        
        if (write.callback) write.callback->on_serializer_write_block();
        if (extra_cb) extra_cb->on_io_complete(NULL);
        
        delete this;
        return true;
    }
};

struct ls_write_fsm_t :
    private iocallback_t,
    private lba_index_t::sync_callback_t,
    private mb_manager_t::metablock_write_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, ls_write_fsm_t >
{
    enum state_t {
        state_start,
        state_waiting_for_data_and_lba,
        state_waiting_for_metablock,
        state_done
    } state;
    
    log_serializer_t *ser;
    
    log_serializer_t::write_t *writes;
    int num_writes;
    
    ls_write_fsm_t(log_serializer_t *ser, log_serializer_t::write_t *writes, int num_writes)
        : state(state_start), ser(ser), writes(writes), num_writes(num_writes) {
        ser->active_write_count ++;
    }
    
    ~ls_write_fsm_t() {
        assert(state == state_done);
        ser->active_write_count --;
    }
    
    bool run(log_serializer_t::write_txn_callback_t *cb) {
        
        assert(state == state_start);
        
        callback = NULL;
        if (do_start_writes_and_lba()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool do_start_writes_and_lba() {
        
        state = state_waiting_for_data_and_lba;
        
        /* Launch each individual block writer */
        
        num_writes_waited_for = 0;
        for (int i = 0; i < num_writes; i ++) {
            ls_block_writer_t *writer = new ls_block_writer_t(ser, writes[i]);
            if (!writer->run(this)) num_writes_waited_for++;
        }
        
        /* Sync the LBA */
        
        offsets_were_written = ser->lba_index.sync(this);
        
        /* Prepare metablock now instead of in when we write it so that we will have the correct
        metablock information for this write even if another write starts before we finish writing
        our data and LBA. */
        
        ser->extent_manager.prepare_metablock(&mb_buffer.extent_manager_part);
        ser->data_block_manager.prepare_metablock(&mb_buffer.data_block_manager_part);
        ser->lba_index.prepare_metablock(&mb_buffer.lba_index_part);
        
        /* If we're already done, go on to the next step */
        
        if (offsets_were_written && num_writes_waited_for == 0) return do_write_metablock();
        else return false;
    }
    
    void on_io_complete(event_t *unused) {
        assert(state == state_waiting_for_data_and_lba);
        assert(num_writes_waited_for > 0);
        num_writes_waited_for --;
        if (offsets_were_written && num_writes_waited_for == 0) do_write_metablock();
    }
    
    void on_lba_sync() {
        assert(state == state_waiting_for_data_and_lba);
        assert(!offsets_were_written);
        offsets_were_written = true;
        if (offsets_were_written && num_writes_waited_for == 0) do_write_metablock();
    }
    
    bool do_write_metablock() {
        
        state = state_waiting_for_metablock;
        
        if (ser->metablock_manager.write_metablock(&mb_buffer, this)) return do_finish();
        else return false;
    }
    
    void on_metablock_write() {
        assert(state == state_waiting_for_metablock);
        do_finish();
    }
    
    bool do_finish() {
        
        state = state_done;
        
        // Must move 'callback' from the instance variable to the stack so we can delete this. The
        // reason we need to delete this before calling the callback is so that we decrement
        // active_write_count before calling the callback.
        log_serializer_t::write_txn_callback_t *cb = callback;
        delete this;
        if (cb) cb->on_serializer_write_txn();
        
        return true;
    }
    
private:
    bool offsets_were_written;
    int num_writes_waited_for;
    log_serializer_t::write_txn_callback_t *callback;
    
    log_serializer_t::metablock_t mb_buffer;
};

bool log_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    
    assert(state == state_ready);
    
    ls_write_fsm_t *w = new ls_write_fsm_t(this, writes, num_writes);
    return w->run(callback);
}

bool log_serializer_t::do_read(block_id_t block_id, void *buf, read_callback_t *callback) {
    
    assert(state == state_ready);
    
    /* See if we are currently in the process of writing the block */
    block_writer_map_t::iterator it = block_writer_map.find(block_id);
    
    if (it == block_writer_map.end()) {
    
        /* We are not currently writing the block; go to disk to get it */
        
        off64_t offset = lba_index.get_block_offset(block_id);
        assert(offset != DELETE_BLOCK);   // Make sure the block actually exists
        
        return data_block_manager.read(offset, buf, callback);
    
    } else {
        
        /* We are currently writing the block; we can just get it from memory */
        memcpy(buf, (*it).second->write.buf, block_size);
        return true;
    }
}

block_id_t log_serializer_t::gen_block_id() {
    assert(state == state_ready);
    return lba_index.gen_block_id();
}

bool log_serializer_t::shutdown(shutdown_callback_t *cb) {
    
    /* TODO: Instead of asserting we are done starting up, block until we are done starting up. */
    
    assert(state == state_ready);
    assert(active_write_count == 0);
    
    lba_index.shutdown();
    data_block_manager.shutdown();
    metablock_manager.shutdown();
    extent_manager.shutdown();

    state = state_shut_down;

    return true;
}
