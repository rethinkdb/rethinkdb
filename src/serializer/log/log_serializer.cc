#include "log_serializer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

log_serializer_t::log_serializer_t(cmd_config_t *cmd_config, char *_db_path, size_t block_size)
    : shutdown_callback(NULL),
      block_size(block_size),
      state(state_unstarted),
      dbfile(NULL),
      extent_manager(EXTENT_SIZE),
      metablock_manager(&extent_manager),
      data_block_manager(this, cmd_config, &extent_manager, block_size),
      lba_index(&data_block_manager, &extent_manager),
      last_write(NULL),
      active_write_count(0) {
    
    assert(strlen(_db_path) <= MAX_DB_FILE_NAME - 1);   // "- 1" for the null-terminator
    strncpy(db_path, _db_path, MAX_DB_FILE_NAME);
}

log_serializer_t::~log_serializer_t() {
    assert(state == state_unstarted || state == state_shut_down);
    assert(last_write == NULL);
    assert(active_write_count == 0);
}

/* The process of starting up the serializer is handled by the ls_start_fsm_t. This is not
necessary, because there is only ever one startup process for each serializer; the serializer could
handle its own startup process. It is done this way to make it clear which parts of the serializer
are involved in startup and which parts are not. */

struct ls_start_fsm_t :
    public mb_manager_t::metablock_read_callback_t,
    public lba_index_t::ready_callback_t,
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
        
        ser->dbfile = new direct_file_t(ser->db_path, direct_file_t::mode_read|direct_file_t::mode_write);
        
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
            if (ser->metablock_manager.start(ser->dbfile, &metablock_found, &metablock_buffer, this)) {
                state = state_start_lba;
            } else {
                state = state_waiting_for_metablock;
                return false;
            }
        }
        
        if (state == state_start_lba) {
            if (metablock_found) {
#ifndef NDEBUG
                memcpy(&ser->debug_mb_buffer, &metablock_buffer, sizeof(metablock_buffer));
#endif
                ser->extent_manager.start(ser->dbfile, &metablock_buffer.extent_manager_part);
                ser->data_block_manager.start(ser->dbfile, &metablock_buffer.data_block_manager_part);
                if (ser->lba_index.start(ser->dbfile, &metablock_buffer.lba_index_part, this)) {
                    state = state_finish;
                } else {
                    state = state_waiting_for_lba;
                    return false;
                }
            } else {
                ser->extent_manager.start(ser->dbfile);
                ser->data_block_manager.start(ser->dbfile);
                ser->lba_index.start(ser->dbfile);
                state = state_finish;
#ifndef NDEBUG
                ser->prepare_metablock(&ser->debug_mb_buffer);
#endif
            }
        }
        
        if (state == state_finish) {
            state = state_done;
            assert(ser->state == log_serializer_t::state_starting_up);
            ser->state = log_serializer_t::state_ready;
            if(ready_callback)
                ready_callback->on_serializer_ready(ser);

#ifndef NDEBUG
            if(metablock_found) {
                log_serializer_t::metablock_t debug_mb_buffer;
                ser->prepare_metablock(&debug_mb_buffer);
                // Make sure that the metablock has not changed since the last
                // time we recorded it
                assert(memcmp(&debug_mb_buffer, &ser->debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
            }
#endif
   
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
    
    log_serializer_t *ser;
    log_serializer_t::ready_callback_t *ready_callback;
    
    enum state_t {
        state_start,
        state_find_metablock,
        state_waiting_for_metablock,
        state_start_lba,
        state_waiting_for_lba,
        state_finish,
        state_done
    } state;
    
    bool metablock_found;
    log_serializer_t::metablock_t metablock_buffer;
    
    char *initial_superblock;
};

bool log_serializer_t::start(ready_callback_t *ready_cb) {

    assert(state == state_unstarted);
    assert_cpu();
    
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

        /* mark the garbage */
        off64_t gc_offset = ser->lba_index.get_block_offset(write.block_id);
        if (gc_offset != -1)
            ser->data_block_manager.mark_garbage(gc_offset);
        
        if (write.buf) {
        
            off64_t new_offset;
            *(ser_block_id_t *) write.buf = write.block_id;
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
            ser->lba_index.set_block_offset(write.block_id, DELETE_BLOCK);
            
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
    
    /* We notify this after we have submitted our metablock to the metablock manager; it
    waits for our notification before submitting its metablock to the metablock
    manager. This way we guarantee that the metablocks are ordered correctly. */
    ls_write_fsm_t *next_write;
    
    ls_write_fsm_t(log_serializer_t *ser, log_serializer_t::write_t *writes, int num_writes)
        : state(state_start), ser(ser), writes(writes), num_writes(num_writes), next_write(NULL) {
    }
    
    ~ls_write_fsm_t() {
        assert(state == state_start || state == state_done);
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
        
        ser->active_write_count++;
        
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

        ser->prepare_metablock(&mb_buffer);
        
        /* Get in line for the metablock manager */
        
        if (ser->last_write) {
            ser->last_write->next_write = this;
            waiting_for_prev_write = true;
        } else {
            waiting_for_prev_write = false;
        }
        ser->last_write = this;
        
        /* If we're already done, go on to the next step */
        
        return maybe_write_metablock();
    }
    
    void on_io_complete(event_t *unused) {
        assert(state == state_waiting_for_data_and_lba);
        assert(num_writes_waited_for > 0);
        num_writes_waited_for --;
        maybe_write_metablock();
    }
    
    void on_lba_sync() {
        assert(state == state_waiting_for_data_and_lba);
        assert(!offsets_were_written);
        offsets_were_written = true;
        maybe_write_metablock();
    }
    
    void on_prev_write_submitted_metablock() {
        assert(state == state_waiting_for_data_and_lba);
        assert(waiting_for_prev_write);
        waiting_for_prev_write = false;
        maybe_write_metablock();
    }
    
    bool maybe_write_metablock() {
        if (offsets_were_written && num_writes_waited_for == 0 && !waiting_for_prev_write) {
            return do_write_metablock();
        } else {
            return false;
        }
    }
    
    bool do_write_metablock() {
        
        state = state_waiting_for_metablock;
        
        bool done = ser->metablock_manager.write_metablock(&mb_buffer, this);
        
        /* If there was another transaction waiting for us to write our metablock so it could
        write its metablock, notify it now so it can write its metablock. */
        if (next_write) {
            next_write->on_prev_write_submitted_metablock();
        } else {
            assert(this == ser->last_write);
            ser->last_write = NULL;
        }
        
        if (done) return do_finish();
        else return false;
    }
    
    void on_metablock_write() {
        assert(state == state_waiting_for_metablock);
        do_finish();
    }
    
    bool do_finish() {
        
        ser->active_write_count--;

        ser->extent_manager.on_metablock_comitted(&mb_buffer.extent_manager_part);
        
        state = state_done;
        
        //TODO I'm kind of unhappy that we're calling this from in here we should figure out better where to trigger gc
        ser->consider_start_gc();
        
        if (callback) callback->on_serializer_write_txn();
        
        // If we were in the process of shutting down and this is the
        // last transaction, shut ourselves down for good.
        if(ser->state == log_serializer_t::state_shutting_down
           && ser->shutdown_state == log_serializer_t::shutdown_waiting_on_serializer
           && ser->last_write == NULL
           && ser->active_write_count == 0)
        {
            ser->next_shutdown_step();
        }

        delete this;
        return true;
    }
    
private:
    bool offsets_were_written;
    int num_writes_waited_for;
    bool waiting_for_prev_write;
    log_serializer_t::write_txn_callback_t *callback;
    
    log_serializer_t::metablock_t mb_buffer;
};


bool log_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {

    // Even if state != state_ready we might get a do_write from the
    // datablock manager on gc (because it's writing the final gc as
    // we're shutting down). That is ok, which is why we don't assert
    // on state here.
    
    assert_cpu();
    
#ifndef NDEBUG
    metablock_t _debug_mb_buffer;
    prepare_metablock(&_debug_mb_buffer);
    // Make sure that the metablock has not changed since the last
    // time we recorded it
    assert(memcmp(&_debug_mb_buffer, &debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
#endif
                
    ls_write_fsm_t *w = new ls_write_fsm_t(this, writes, num_writes);
    bool res = w->run(callback);

#ifndef NDEBUG
    prepare_metablock(&debug_mb_buffer);
#endif

    return res;
}

bool log_serializer_t::do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
    
    assert(state == state_ready);
    assert_cpu();
    
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

ser_block_id_t log_serializer_t::max_block_id() {
    
    assert(state == state_ready);
    assert_cpu();
    
    return lba_index.max_block_id();
}

bool log_serializer_t::block_in_use(ser_block_id_t id) {
    
    assert(state == state_ready);
    assert_cpu();
    
    return lba_index.get_block_offset(id) != DELETE_BLOCK;
}

bool log_serializer_t::shutdown(shutdown_callback_t *cb) {

    assert(cb);
    assert(state == state_ready);
    assert_cpu();
    shutdown_callback = cb;

    shutdown_state = shutdown_begin;
    shutdown_in_one_shot = true;
    
    return next_shutdown_step();
}

bool log_serializer_t::next_shutdown_step() {
    
    assert_cpu();
    
    if(shutdown_state == shutdown_begin) {
        // First shutdown step
        shutdown_state = shutdown_waiting_on_serializer;
        if (last_write || active_write_count > 0) {
            state = state_shutting_down;
            shutdown_in_one_shot = false;
            return false;
        }
        state = state_shutting_down;
    }

    if(shutdown_state == shutdown_waiting_on_serializer) {
        shutdown_state = shutdown_waiting_on_datablock_manager;
        if(!data_block_manager.shutdown(this)) {
            shutdown_in_one_shot = false;
            return false;
        }
    }

    if(shutdown_state == shutdown_waiting_on_datablock_manager) {
        shutdown_state = shutdown_waiting_on_lba;
        if(!lba_index.shutdown(this)) {
            shutdown_in_one_shot = false;
            return false;
        }
    }

    if(shutdown_state == shutdown_waiting_on_lba) {
        metablock_manager.shutdown();
        extent_manager.shutdown();
        
        delete dbfile;
        dbfile = NULL;
        
        state = state_shut_down;

        // Don't call the callback if we went through the entire
        // shutdown process in one synchronous shot.
        if(!shutdown_in_one_shot && shutdown_callback) {
            do_later(shutdown_callback, &shutdown_callback_t::on_serializer_shutdown, this);
        }

        return true;
    }

    fail("Invalid state.");
    return true; // make compiler happy
}

void log_serializer_t::on_datablock_manager_shutdown() {
    next_shutdown_step();
}

void log_serializer_t::on_lba_shutdown() {
    next_shutdown_step();
}

void log_serializer_t::prepare_metablock(metablock_t *mb_buffer) {
    bzero(mb_buffer, sizeof(*mb_buffer));
    extent_manager.prepare_metablock(&mb_buffer->extent_manager_part);
    data_block_manager.prepare_metablock(&mb_buffer->data_block_manager_part);
    lba_index.prepare_metablock(&mb_buffer->lba_index_part);
}


void log_serializer_t::consider_start_gc() {
    if (data_block_manager.do_we_want_to_start_gcing() && state == log_serializer_t::state_ready) {
        // We do not do GC if we're not in the ready state
        // (i.e. shutting down)
        data_block_manager.start_gc();
    }
}
