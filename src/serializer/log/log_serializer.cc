#include "log_serializer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

log_serializer_t::log_serializer_t(char *_db_path, size_t block_size)
    : block_size(block_size),
      state(state_unstarted),
      gc_counter(0),
      dbfd(INVALID_FD),
      extent_manager(EXTENT_SIZE),
      metablock_manager(&extent_manager),
      data_block_manager(this, &extent_manager, block_size),
      lba_index(&data_block_manager, &extent_manager),
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
        if (S_ISBLK(file_stat.st_mode)) {
            ser->dbfd = open(ser->db_path,
                    O_RDWR | O_CREAT | O_DIRECT | O_LARGEFILE,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        } else {
            /* the file only exists at this point if it's a block device */
            ser->dbfd = open(ser->db_path,
                    O_RDWR | O_CREAT | O_DIRECT | O_LARGEFILE | O_NOATIME,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        }
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

bool log_serializer_t::do_read(block_id_t block_id, void *buf, read_callback_t *callback) {
    
    assert(state == state_ready || state == state_write);
    
    off64_t offset = lba_index.get_block_offset(block_id);
    assert(offset != DELETE_BLOCK); // Make sure we're not trying to read a deleted block
    
    return data_block_manager.read(offset, buf, callback);
}

/* Each transaction written is handled by a new ls_write_fsm_t instance. This is so that
multiple writes can be handled concurrently -- if more than one cache uses the same serializer,
one cache should be able to start a flush before the previous cache's flush is done. */

struct ls_write_fsm_t :
    private lba_index_t::sync_callback_t,
    private mb_manager_t::metablock_write_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, ls_write_fsm_t >
{
    struct block_writer_t :
        public iocallback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, block_writer_t >
    {
        ls_write_fsm_t *txn_writer;
        log_serializer_t::write_block_callback_t *cb;
        block_id_t block_id;
        block_writer_t(ls_write_fsm_t *txn_writer, log_serializer_t::write_block_callback_t *cb)
            : txn_writer(txn_writer), cb(cb) { }
        void on_io_complete(event_t *e) {
            if (cb) cb->on_serializer_write_block();
            txn_writer->on_write_block();
            delete this;
        }
    };
    
    ls_write_fsm_t(log_serializer_t *serializer)
        : state(state_start), ser(serializer) {
        ser->active_write_count ++;
    }
    
    ~ls_write_fsm_t() {
        assert(state == state_done);
        ser->active_write_count --;
    }
    
    bool run(log_serializer_t::write_t *writes, int num_writes, log_serializer_t::write_txn_callback_t *cb) {
        
        assert(state == state_start);
        
        num_writes_waited_for = 0;
        
        /* TODO: Allocation */
        lba_entries = (lba_index_t::entry_t*)malloc(sizeof(lba_index_t::entry_t) * num_writes);
        
        for (int i = 0; i < num_writes; i ++) {
            
            off64_t new_offset;
            
            if (writes[i].buf) {
            
                block_writer_t *writer = new block_writer_t(this, writes[i].callback);
                writer->block_id = writes[i].block_id;   // For debugging
                *(block_id_t *) writes[i].buf = writes[i].block_id; //copy the block_id to be serialized
                if (ser->data_block_manager.write(writes[i].buf, &new_offset, writer)) {
                    delete writer;   // CB not necessary after all
                } else {
                    num_writes_waited_for ++;
                }
                
            } else {
            
                /* Deletion */
                new_offset = DELETE_BLOCK;
                
                // Deletion happens instantaneously, so there is no callback. We assert the callback
                // is NULL in case the caller was expecting a callback because they didn't
                // understand the API.
                assert(writes[i].callback == NULL);
            }
            
            lba_entries[i].block_id = writes[i].block_id;
            lba_entries[i].offset = new_offset;
        }

        /* mark the garbage */
        for (int i = 0; i < num_writes; i++) {
            off64_t gc_offset = ser->lba_index.get_block_offset(lba_entries[i].block_id);
            if (gc_offset != -1)
                ser->data_block_manager.mark_garbage(gc_offset);
        }
        
        offsets_were_written = ser->lba_index.write(lba_entries, num_writes, this);
        
        /* Prepare metablock now instead of in next_write_step() so that we will have the correct
        metablock information for this write even if another write starts before we finish writing
        our data and LBA. */
        
        ser->extent_manager.prepare_metablock(&mb_buffer.extent_manager_part);
        ser->data_block_manager.prepare_metablock(&mb_buffer.data_block_manager_part);
        ser->lba_index.prepare_metablock(&mb_buffer.lba_index_part);

        if (offsets_were_written && num_writes_waited_for == 0) {
            callback = NULL;
            state = state_write_metablock;
            if (next_write_step()) {
                return true;
            } else {
                callback = cb;
                return false;
            }
        } else {
            state = state_waiting_for_data_and_lba;
            callback = cb;
            return false;
        }
    }
    
    bool next_write_step() {
        
        if (state == state_write_metablock) {
            if (ser->metablock_manager.write_metablock(&mb_buffer, this)) {
                state = state_finish;
            } else {
                state = state_waiting_for_metablock;
                return false;
            }
        }
        
        if (state == state_finish) {
            state = state_done;
            
            // Must call the callback after deleting ourselves so that we decrement
            // active_write_count.


            log_serializer_t *_ser = ser;
            log_serializer_t::write_txn_callback_t *cb = callback;
            delete this;
            if (cb) cb->on_serializer_write_txn();

            //TODO I'm kind of unhappy that we're calling this from in here we should figure out better where to trigger gc
            _ser->gc_counter = (_ser->gc_counter + 1) % 5;
            if (_ser->gc_counter == 0)
                _ser->data_block_manager.start_gc();

            //HACKY, this should be done with a callback
            _ser->do_outstanding_writes();

            return true;
        }
        
        fail("Invalid state.");
    }
    
    void on_write_block() {
        assert(state == state_waiting_for_data_and_lba);
        assert(num_writes_waited_for > 0);
        num_writes_waited_for --;
        if (offsets_were_written && num_writes_waited_for == 0) {
            state = state_write_metablock;
            next_write_step();
        }
    }
    
    void on_lba_sync() {
        assert(state == state_waiting_for_data_and_lba);
        assert(!offsets_were_written);
        offsets_were_written = true;
        free(lba_entries);
        if (offsets_were_written && num_writes_waited_for == 0) {
            state = state_write_metablock;
            next_write_step();
        }
    }
    
    void on_metablock_write() {
        assert(state == state_waiting_for_metablock);
        state = state_finish;
        next_write_step();
    }
    
private:
    enum state_t {
        state_start,
        state_waiting_for_data_and_lba,
        state_write_metablock,
        state_waiting_for_metablock,
        state_finish,
        state_done
    } state;
    
    log_serializer_t *ser;
    
    lba_index_t::entry_t *lba_entries;
    
    bool offsets_were_written;
    int num_writes_waited_for;
    log_serializer_t::write_txn_callback_t *callback;
    
    log_serializer_t::metablock_t mb_buffer;
};

bool log_serializer_t::do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    assert(state == state_ready || state == state_write);
    
    bool res;

    if (state == state_ready) {
        state = state_write;
        ls_write_fsm_t *w = new ls_write_fsm_t(this);
        res = w->run(writes, num_writes, callback);
    } else {
        write_t *_writes = (write_t *) _gmalloc(sizeof(write_t) * num_writes);
        for (int i = 0; i < num_writes; i++) {
            _writes[i] = writes[i];
        }
        outstanding_writes.push(write_record_t(_writes, num_writes, callback));
        res = false;
    }
    return res;
}

void log_serializer_t::do_outstanding_writes() {
    if (!outstanding_writes.empty()) {
        write_record_t rec = outstanding_writes.front();

        outstanding_writes.pop();
        assert(state_write);
        ls_write_fsm_t *w = new ls_write_fsm_t(this);
        w->run(rec.writes, rec.num_writes, rec.callback);
        delete rec.writes;
    } else {
        state = state_ready;
    }
}

block_id_t log_serializer_t::gen_block_id() {
    assert(state == state_ready || state == state_write);
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
