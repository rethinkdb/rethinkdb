#include "log_serializer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buffer_cache/types.hpp"

const block_magic_t log_serializer_t::zerobuf_magic = { { 'z', 'e', 'r', 'o' } };

log_serializer_t::log_serializer_t(dynamic_config_t *config, private_dynamic_config_t *private_dynamic_config)
    : dynamic_config(config),
      private_config(private_dynamic_config),
      shutdown_callback(NULL),
      state(state_unstarted),
      db_path(private_dynamic_config->db_filename.c_str()),
      dbfile(NULL),
      extent_manager(NULL),
      metablock_manager(NULL),
      lba_index(NULL),
      data_block_manager(NULL),
      last_write(NULL),
      active_write_count(0) {
}

log_serializer_t::~log_serializer_t() {
    rassert(state == state_unstarted || state == state_shut_down);
    rassert(last_write == NULL);
    rassert(active_write_count == 0);
}

void ls_check_existing(const char *filename, log_serializer_t::check_callback_t *cb) {
    direct_file_t df(filename, file_t::mode_read);
    cb->on_serializer_check(static_header_check(&df));
}

void log_serializer_t::check_existing(const char *filename, check_callback_t *cb) {
    coro_t::spawn(boost::bind(ls_check_existing, filename, cb));
}

/* The process of starting up the serializer is handled by the ls_start_*_fsm_t. This is not
necessary, because there is only ever one startup process for each serializer; the serializer could
handle its own startup process. It is done this way to make it clear which parts of the serializer
are involved in startup and which parts are not. */

void log_serializer_t::ls_start_new(static_config_t *config, ready_callback_t *ready_cb) {
    rassert(state == log_serializer_t::state_unstarted);
    state = log_serializer_t::state_starting_up;
    static_config = *config;
    dbfile = new direct_file_t(db_path, file_t::mode_read | file_t::mode_write | file_t::mode_create);
    co_static_header_write(dbfile, &static_config, sizeof(static_config));
    
    log_serializer_t::metablock_t metablock_buffer;
    extent_manager = new extent_manager_t(dbfile, &static_config, dynamic_config);
    extent_manager->reserve_extent(0);   /* For static header */
   
    metablock_manager = new mb_manager_t(extent_manager);
    lba_index = new lba_index_t(extent_manager);
    data_block_manager = new data_block_manager_t(this, dynamic_config, extent_manager, &static_config);
    
    metablock_manager->start_new(dbfile);
    lba_index->start_new(dbfile);
    data_block_manager->start_new(dbfile);
    
    extent_manager->start_new();

    current_transaction_id = FIRST_SER_TRANSACTION_ID;

#ifndef NDEBUG
    prepare_metablock(&debug_mb_buffer);
#endif
    prepare_metablock(&metablock_buffer);
    
    metablock_manager->co_write_metablock(&metablock_buffer);
    
    rassert(state == log_serializer_t::state_starting_up);
    state = log_serializer_t::state_ready;

    ready_cb->on_serializer_ready(this);
};

bool log_serializer_t::start_new(static_config_t *config, ready_callback_t *ready_cb) {
    rassert(state == state_unstarted);
    assert_thread();
    coro_t::spawn(boost::bind(&log_serializer_t::ls_start_new, this, config, ready_cb));
    return false;
}

struct ls_start_existing_fsm_t :
    public static_header_read_callback_t,
    public mb_manager_t::metablock_read_callback_t,
    public lba_index_t::ready_callback_t
{
    explicit ls_start_existing_fsm_t(log_serializer_t *serializer)
        : ser(serializer), state(state_start) {
    }
    
    ~ls_start_existing_fsm_t() {
    }
    
    bool run(log_serializer_t::ready_callback_t *ready_cb) {
        rassert(state == state_start);
        rassert(ser->state == log_serializer_t::state_unstarted);
        ser->state = log_serializer_t::state_starting_up;
        
        ser->dbfile = new direct_file_t(ser->db_path, file_t::mode_read | file_t::mode_write);
        
        state = state_read_static_header;
        ready_callback = NULL;
        if (next_starting_up_step()) {
            return true;
        } else {
            ready_callback = ready_cb;
            return false;
        }
    }
    
    bool next_starting_up_step() {
        if (state == state_read_static_header) {
            if (static_header_read(ser->dbfile, &ser->static_config, sizeof(ser->static_config), this)) {
                state = state_find_metablock;
            } else {
                state = state_waiting_for_static_header;
                return false;
            }
        }
        
        if (state == state_find_metablock) {
            ser->extent_manager = new extent_manager_t(ser->dbfile, &ser->static_config, ser->dynamic_config);
            ser->extent_manager->reserve_extent(0);   /* For static header */
            
            ser->metablock_manager = new mb_manager_t(ser->extent_manager);
            ser->lba_index = new lba_index_t(ser->extent_manager);
            ser->data_block_manager = new data_block_manager_t(ser, ser->dynamic_config, ser->extent_manager, &ser->static_config);
            
            if (ser->metablock_manager->start_existing(ser->dbfile, &metablock_found, &metablock_buffer, this)) {
                state = state_start_lba;
            } else {
                state = state_waiting_for_metablock;
                return false;
            }
        }
        
        if (state == state_start_lba) {
            guarantee(metablock_found, "Could not find any valid metablock.");
            
#ifndef NDEBUG
            memcpy(&ser->debug_mb_buffer, &metablock_buffer, sizeof(metablock_buffer));
#endif

            ser->current_transaction_id = metablock_buffer.transaction_id;

            if (ser->lba_index->start_existing(ser->dbfile, &metablock_buffer.lba_index_part, this)) {
                state = state_reconstruct;
            } else {
                state = state_waiting_for_lba;
                return false;
            }
        }
        
        if (state == state_reconstruct) {
            ser->data_block_manager->start_reconstruct();
            for (ser_block_id_t::number_t id = 0; id < ser->lba_index->max_block_id().value; id++) {
                flagged_off64_t offset = ser->lba_index->get_block_offset(ser_block_id_t::make(id));
                if (flagged_off64_t::can_be_gced(offset)) {
                    ser->data_block_manager->mark_live(offset.parts.value);
                }
            }
            ser->data_block_manager->end_reconstruct();
            ser->data_block_manager->start_existing(ser->dbfile, &metablock_buffer.data_block_manager_part);
            
            ser->extent_manager->start_existing(&metablock_buffer.extent_manager_part);
            
            state = state_finish;
        }
        
        if (state == state_finish) {
            state = state_done;
            rassert(ser->state == log_serializer_t::state_starting_up);
            ser->state = log_serializer_t::state_ready;
            if(ready_callback)
                ready_callback->on_serializer_ready(ser);

#ifndef NDEBUG
            {
                log_serializer_t::metablock_t debug_mb_buffer;
                ser->prepare_metablock(&debug_mb_buffer);
                // Make sure that the metablock has not changed since the last
                // time we recorded it
                rassert(memcmp(&debug_mb_buffer, &ser->debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
            }
#endif
   
            delete this;
            return true;
        }
        
        unreachable("Invalid state.");
    }
    
    void on_static_header_read() {
        rassert(state == state_waiting_for_static_header);
        state = state_find_metablock;
        next_starting_up_step();
    }
    
    void on_metablock_read() {
        rassert(state == state_waiting_for_metablock);
        state = state_start_lba;
        next_starting_up_step();
    }
    
    void on_lba_ready() {
        rassert(state == state_waiting_for_lba);
        state = state_reconstruct;
        next_starting_up_step();
    }
    
    log_serializer_t *ser;
    log_serializer_t::ready_callback_t *ready_callback;
    
    enum state_t {
        state_start,
        state_read_static_header,
        state_waiting_for_static_header,
        state_find_metablock,
        state_waiting_for_metablock,
        state_start_lba,
        state_waiting_for_lba,
        state_reconstruct,
        state_finish,
        state_done
    } state;
    
    bool metablock_found;
    log_serializer_t::metablock_t metablock_buffer;
};

bool log_serializer_t::start_existing(ready_callback_t *ready_cb) {
    rassert(state == state_unstarted);
    assert_thread();
    
    ls_start_existing_fsm_t *s = new ls_start_existing_fsm_t(this);
    return s->run(ready_cb);
}

void *log_serializer_t::malloc() {
    rassert(state == state_ready || state == state_shutting_down);
    
    // TODO: we shouldn't use malloc_aligned here, we should use our
    // custom allocation system instead (and use corresponding
    // free). This is tough because serializer object may not be on
    // the same core as the cache that's using it, so we should expose
    // the malloc object in a different way.
    char *data = (char *)malloc_aligned(static_config.block_size().ser_value(), DEVICE_BLOCK_SIZE);
    data += sizeof(buf_data_t);
    return (void *)data;
}

void *log_serializer_t::clone(void *_data) {
    rassert(state == state_ready || state == state_shutting_down);
    
    // TODO: we shouldn't use malloc_aligned here, we should use our
    // custom allocation system instead (and use corresponding
    // free). This is tough because serializer object may not be on
    // the same core as the cache that's using it, so we should expose
    // the malloc object in a different way.
    char *data = (char *)malloc_aligned(static_config.block_size().ser_value(), DEVICE_BLOCK_SIZE);
    memcpy(data, (char *)_data - sizeof(buf_data_t), static_config.block_size().ser_value());
    data += sizeof(buf_data_t);
    return (void *)data;
}

void log_serializer_t::free(void *ptr) {
    rassert(state == state_ready || state == state_shutting_down);
    
    char *data = (char *)ptr;
    data -= sizeof(buf_data_t);
    ::free((void *)data);
}

/* Each transaction written is handled by a new ls_write_fsm_t instance. This is so that
multiple writes can be handled concurrently -- if more than one cache uses the same serializer,
one cache should be able to start a flush before the previous cache's flush is done.

Within a transaction, each individual change is handled by a new ls_block_writer_t.*/

struct ls_block_writer_t :
    public iocallback_t
{
    log_serializer_t *ser;
    log_serializer_t::write_t write;
    iocallback_t *extra_cb;

    // A buffer that's zeroed out (except in the beginning, where we
    // write the block id), which we write upon deletion.  Can be NULL.
    void *zerobuf;

    ls_block_writer_t(log_serializer_t *ser,
                      const log_serializer_t::write_t &write)
        : ser(ser), write(write), zerobuf(NULL) { }
    
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
        if (write.buf_specified) {

            /* mark the garbage */
            flagged_off64_t gc_offset = ser->lba_index->get_block_offset(write.block_id);
            if (flagged_off64_t::can_be_gced(gc_offset))
                ser->data_block_manager->mark_garbage(gc_offset.parts.value);

            bool done;

            repli_timestamp recency = write.recency_specified ? write.recency : ser->lba_index->get_block_recency(write.block_id);


            if (write.buf) {
                off64_t new_offset;
                // TODO! preserve TID
                done = ser->data_block_manager->write(write.buf, write.block_id, write.assign_transaction_id ? ser->current_transaction_id : NULL_SER_TRANSACTION_ID, &new_offset, this);

                ser->lba_index->set_block_offset(write.block_id, recency, flagged_off64_t::real(new_offset));
            } else {
                /* Deletion */

                /* We tell the data_block_manager to write a zero block to
                   make recovery from a corrupted file more likely. */

                // We write a zero buffer with the given block_id at the front.
                zerobuf = ser->malloc();
                bzero(zerobuf, ser->get_block_size().value());
                memcpy(zerobuf, &log_serializer_t::zerobuf_magic, sizeof(block_magic_t));

                off64_t new_offset;
                done = ser->data_block_manager->write(zerobuf, write.block_id, ser->current_transaction_id, &new_offset, this);
                ser->lba_index->set_block_offset(write.block_id, recency, flagged_off64_t::deleteblock(new_offset));
            }
            if (done) {
                return do_finish();
            } else {
                return false;
            }
        } else {
            // It doesn't make sense for a write to not specify a
            // recency _or_ a buffer, since such a write does not
            // actually do anything.
            rassert(write.recency_specified);

            if (write.recency_specified) {
                ser->lba_index->set_block_offset(write.block_id, write.recency, ser->lba_index->get_block_offset(write.block_id));
            }
            return do_finish();
        }
    }

    void on_io_complete() {
        do_finish();
    }

    bool do_finish() {
        if (write.callback) write.callback->on_serializer_write_block();
        if (extra_cb) extra_cb->on_io_complete();

        if (zerobuf) {
            ser->free(zerobuf);
        }
        delete this;
        return true;
    }
};

perfmon_duration_sampler_t pm_serializer_writes("serializer_writes", secs_to_ticks(1));

struct ls_write_fsm_t :
    private iocallback_t,
    private lba_index_t::sync_callback_t,
    private mb_manager_t::metablock_write_callback_t
{
    enum state_t {
        state_start,
        state_waiting_for_data_and_lba,
        state_waiting_for_metablock,
        state_done
    } state;
    
    ticks_t start_time;
    
    log_serializer_t *ser;

    log_serializer_t::write_t *writes;
    int num_writes;
    
    extent_manager_t::transaction_t *extent_txn;
    
    /* We notify this after we have submitted our metablock to the metablock manager; it
    waits for our notification before submitting its metablock to the metablock
    manager. This way we guarantee that the metablocks are ordered correctly. */
    ls_write_fsm_t *next_write;
    
    ls_write_fsm_t(log_serializer_t *ser, log_serializer_t::write_t *writes, int num_writes)
        : state(state_start), ser(ser), writes(writes), num_writes(num_writes), next_write(NULL)
    {
        pm_serializer_writes.begin(&start_time);
    }
    
    ~ls_write_fsm_t() {
        rassert(state == state_start || state == state_done);
        pm_serializer_writes.end(&start_time);
    }
    
    bool run(log_serializer_t::write_txn_callback_t *cb) {
        rassert(state == state_start);
        
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
        
        /* Start an extent manager transaction so we can allocate and release extents */
        extent_txn = ser->extent_manager->begin_transaction();
        
        state = state_waiting_for_data_and_lba;
        
        /* Just to make sure that the LBA GC gets exercised */
        
        ser->lba_index->consider_gc();
        
        /* Launch each individual block writer */
        
        num_writes_waited_for = 0;
        for (int i = 0; i < num_writes; i ++) {
            ls_block_writer_t *writer = new ls_block_writer_t(ser, writes[i]);
            if (!writer->run(this)) num_writes_waited_for++;
        }
        
        /* Sync the LBA */
        
        offsets_were_written = ser->lba_index->sync(this);
        
        /* Prepare metablock now instead of in when we write it so that we will have the correct
        metablock information for this write even if another write starts before we finish writing
        our data and LBA. */

        ser->prepare_metablock(&mb_buffer);
        
        /* Stop the extent manager transaction so another one can start, but don't commit it
        yet */
        ser->extent_manager->end_transaction(extent_txn);
        
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
    
    void on_io_complete() {
        rassert(state == state_waiting_for_data_and_lba);
        rassert(num_writes_waited_for > 0);
        num_writes_waited_for--;
        maybe_write_metablock();
    }
    
    void on_lba_sync() {
        rassert(state == state_waiting_for_data_and_lba);
        rassert(!offsets_were_written);
        offsets_were_written = true;
        maybe_write_metablock();
    }
    
    void on_prev_write_submitted_metablock() {
        rassert(state == state_waiting_for_data_and_lba);
        rassert(waiting_for_prev_write);
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
        
        bool done = ser->metablock_manager->write_metablock(&mb_buffer, this);
        
        /* If there was another transaction waiting for us to write our metablock so it could
        write its metablock, notify it now so it can write its metablock. */
        if (next_write) {
            next_write->on_prev_write_submitted_metablock();
        } else {
            rassert(this == ser->last_write);
            ser->last_write = NULL;
        }
        
        if (done) return do_finish();
        else return false;
    }
    
    void on_metablock_write() {
        rassert(state == state_waiting_for_metablock);
        do_finish();
    }
    
    bool do_finish() {
        ser->active_write_count--;

        /* End the extent manager transaction so the extents can actually get reused. */
        ser->extent_manager->commit_transaction(extent_txn);
        
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
    
    assert_thread();

#ifndef NDEBUG
    {
        metablock_t _debug_mb_buffer;
        prepare_metablock(&_debug_mb_buffer);
        // Make sure that the metablock has not changed since the last
        // time we recorded it
        rassert(memcmp(&_debug_mb_buffer, &debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
    }
#endif

    current_transaction_id++;

    ls_write_fsm_t *w = new ls_write_fsm_t(this, writes, num_writes);
    bool res = w->run(callback);

#ifndef NDEBUG
    prepare_metablock(&debug_mb_buffer);
#endif

    return res;
}

perfmon_duration_sampler_t pm_serializer_reads("serializer_reads", secs_to_ticks(1));

struct ls_read_fsm_t :
    private iocallback_t
{
    ticks_t start_time;
    log_serializer_t *ser;
    ser_block_id_t block_id;
    void *buf;
    
    ls_read_fsm_t(log_serializer_t *ser, ser_block_id_t block_id, void *buf)
        : ser(ser), block_id(block_id), buf(buf)
    {
        pm_serializer_reads.begin(&start_time);
    }
    
    ~ls_read_fsm_t() {
        pm_serializer_reads.end(&start_time);
    }
    
    serializer_t::read_callback_t *read_callback;
    
    bool run(serializer_t::read_callback_t *cb) {
        read_callback = NULL;
        if (do_read()) {
            return true;
        } else {
            read_callback = cb;
            return false;
        }
    }
    
    bool do_read() {
        
        flagged_off64_t offset = ser->lba_index->get_block_offset(block_id);
        rassert(!offset.parts.is_delete);   // Make sure the block actually exists
        
        if (ser->data_block_manager->read(offset.parts.value, buf, this)) {
            return done();
        } else {
            return false;
        }
    }
    
    void on_io_complete() {
        done();
    }
    
    bool done() {
        if (read_callback) read_callback->on_serializer_read();
        delete this;
        return true;
    }
};

bool log_serializer_t::do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
    rassert(state == state_ready);
    assert_thread();
    
    ls_read_fsm_t *fsm = new ls_read_fsm_t(this, block_id, buf);
    return fsm->run(callback);
}

block_size_t log_serializer_t::get_block_size() {
    return static_config.block_size();
}

ser_block_id_t log_serializer_t::max_block_id() {
    rassert(state == state_ready);
    assert_thread();
    
    return lba_index->max_block_id();
}

bool log_serializer_t::block_in_use(ser_block_id_t id) {
    
    // State is state_shutting_down if we're called from the data block manager during a GC
    // during shutdown.
    rassert(state == state_ready || state == state_shutting_down);
    
    assert_thread();
    
    return !(lba_index->get_block_offset(id).parts.is_delete);
}

repli_timestamp log_serializer_t::get_recency(ser_block_id_t id) {
    return lba_index->get_block_recency(id);
}

bool log_serializer_t::shutdown(shutdown_callback_t *cb) {
    rassert(cb);
    rassert(state == state_ready);
    assert_thread();
    shutdown_callback = cb;

    shutdown_state = shutdown_begin;
    shutdown_in_one_shot = true;
    
    return next_shutdown_step();
}

bool log_serializer_t::next_shutdown_step() {
    assert_thread();
    
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
        if(!data_block_manager->shutdown(this)) {
            shutdown_in_one_shot = false;
            return false;
        }
    }

    if(shutdown_state == shutdown_waiting_on_datablock_manager) {
        shutdown_state = shutdown_waiting_on_lba;
        if(!lba_index->shutdown(this)) {
            shutdown_in_one_shot = false;
            return false;
        }
    }

    if(shutdown_state == shutdown_waiting_on_lba) {
        metablock_manager->shutdown();
        extent_manager->shutdown();
        
        delete lba_index;
        lba_index = NULL;
        
        delete data_block_manager;
        data_block_manager = NULL;
        
        delete metablock_manager;
        metablock_manager = NULL;
        
        delete extent_manager;
        extent_manager = NULL;
        
        delete dbfile;
        dbfile = NULL;
        
        state = state_shut_down;

        // Don't call the callback if we went through the entire
        // shutdown process in one synchronous shot.
        if (!shutdown_in_one_shot && shutdown_callback) {
            do_later(boost::bind(&shutdown_callback_t::on_serializer_shutdown, shutdown_callback, this));
        }

        return true;
    }

    unreachable("Invalid state.");
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
    extent_manager->prepare_metablock(&mb_buffer->extent_manager_part);
    data_block_manager->prepare_metablock(&mb_buffer->data_block_manager_part);
    lba_index->prepare_metablock(&mb_buffer->lba_index_part);
    mb_buffer->transaction_id = current_transaction_id;
}


void log_serializer_t::consider_start_gc() {
    if (data_block_manager->do_we_want_to_start_gcing() && state == log_serializer_t::state_ready) {
        // We do not do GC if we're not in the ready state
        // (i.e. shutting down)
        data_block_manager->start_gc();
    }
}


bool log_serializer_t::disable_gc(gc_disable_callback_t *cb) {
    return data_block_manager->disable_gc(cb);
}

void log_serializer_t::enable_gc() {
    data_block_manager->enable_gc();
}

