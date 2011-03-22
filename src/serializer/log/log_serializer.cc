#include "log_serializer.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "buffer_cache/types.hpp"

const block_magic_t log_serializer_t::zerobuf_magic = { { 'z', 'e', 'r', 'o' } };

void log_serializer_t::create(dynamic_config_t *dynamic_config, private_dynamic_config_t *private_dynamic_config, static_config_t *static_config) {

    direct_file_t df(private_dynamic_config->db_filename.c_str(), file_t::mode_read | file_t::mode_write | file_t::mode_create, dynamic_config->io_backend);

    co_static_header_write(&df, static_config, sizeof(*static_config));

    metablock_t metablock;
    bzero(&metablock, sizeof(metablock));

    /* The extent manager's portion of the metablock includes a number indicating how many extents
    are in use. We have to initialize that to the actual number of extents in use for an empty
    database, which is the same as the number of metablock extents. */
    extent_manager_t::prepare_initial_metablock(&metablock.extent_manager_part, MB_NEXTENTS);

    data_block_manager_t::prepare_initial_metablock(&metablock.data_block_manager_part);
    lba_index_t::prepare_initial_metablock(&metablock.lba_index_part);

    metablock.transaction_id = FIRST_SER_TRANSACTION_ID;

    mb_manager_t::create(&df, static_config->extent_size(), &metablock);
}

/* The process of starting up the serializer is handled by the ls_start_*_fsm_t. This is not
necessary, because there is only ever one startup process for each serializer; the serializer could
handle its own startup process. It is done this way to make it clear which parts of the serializer
are involved in startup and which parts are not. TODO: Coroutines. */

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
    
    bool run(cond_t *to_signal) {
        rassert(state == state_start);
        rassert(ser->state == log_serializer_t::state_unstarted);
        ser->state = log_serializer_t::state_starting_up;
        
        ser->dbfile = new direct_file_t(ser->db_path, file_t::mode_read | file_t::mode_write, ser->dynamic_config->io_backend);
        
        state = state_read_static_header;
        to_signal_when_done = NULL;
        if (next_starting_up_step()) {
            return true;
        } else {
            to_signal_when_done = to_signal;
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
            ser->data_block_manager = new data_block_manager_t(ser, ser->dynamic_config, ser->extent_manager, ser, &ser->static_config);
            
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

#ifndef NDEBUG
            {
                log_serializer_t::metablock_t debug_mb_buffer;
                ser->prepare_metablock(&debug_mb_buffer);
                // Make sure that the metablock has not changed since the last
                // time we recorded it
                rassert(memcmp(&debug_mb_buffer, &ser->debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
            }
#endif
            if (to_signal_when_done) to_signal_when_done->pulse();

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
    cond_t *to_signal_when_done;
    
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

    /* This is because the serializer is not completely converted to coroutines yet. */
    ls_start_existing_fsm_t *s = new ls_start_existing_fsm_t(this);
    cond_t cond;
    if (!s->run(&cond)) cond.wait();
}

log_serializer_t::~log_serializer_t() {

    cond_t cond;
    if (!shutdown(&cond)) cond.wait();

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

void *log_serializer_t::malloc() {
    rassert(state == state_ready || state == state_shutting_down);
    
    // TODO: we shouldn't use malloc_aligned here, we should use our
    // custom allocation system instead (and use corresponding
    // free). This is tough because serializer object may not be on
    // the same core as the cache that's using it, so we should expose
    // the malloc object in a different way.
    char *data = (char *)malloc_aligned(static_config.block_size().ser_value(), DEVICE_BLOCK_SIZE);

    // Initialize the transaction id...
    ((buf_data_t*)data)->transaction_id = NULL_SER_TRANSACTION_ID;

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

/*  TODO! Comment is outdated
Each transaction written is handled by a new ls_write_fsm_t instance. This is so that
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
                done = ser->data_block_manager->write(write.buf, write.block_id, write.assign_transaction_id ? ser->current_transaction_id : NULL_SER_TRANSACTION_ID, &new_offset, this);

                ser->lba_index->set_block_offset(write.block_id, recency, flagged_off64_t::real(new_offset));
            } else {
                /* Deletion */

                /* We tell the data_block_manager to write a zero block to
                   make recovery from a corrupted file more likely. */

                // We write a zero buffer with the given block_id at the front, if necessary.
                if (write.write_empty_deleted_block) {
                    zerobuf = ser->malloc();
                    bzero(zerobuf, ser->get_block_size().value());
                    memcpy(zerobuf, &log_serializer_t::zerobuf_magic, sizeof(block_magic_t));

                    off64_t new_offset;
                    done = ser->data_block_manager->write(zerobuf, write.block_id, ser->current_transaction_id, &new_offset, this);
                    ser->lba_index->set_block_offset(write.block_id, recency, flagged_off64_t::deleteblock(new_offset));
                } else {
                    done = true;
                    // TODO this should not be equal to flagged_off64_t::padding(), use a different constant.
                    ser->lba_index->set_block_offset(write.block_id, recency, flagged_off64_t::delete_id());
                }
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

// TODO! Update? Change names?
perfmon_duration_sampler_t pm_serializer_writes("serializer_writes", secs_to_ticks(1));
perfmon_sampler_t pm_serializer_write_size("serializer_write_size", secs_to_ticks(2));

bool log_serializer_t::do_write(UNUSED write_t *writes, UNUSED int num_writes, UNUSED write_txn_callback_t *callback, UNUSED write_tid_callback_t *tid_callback) {
    // Even if state != state_ready we might get a do_write from the
    // datablock manager on gc (because it's writing the final gc as
    // we're shutting down). That is ok, which is why we don't assert
    // on state here.
    
    assert_thread();

    // TODO! Either wrap or remove...

    return true;
}

bool log_serializer_t::write_gcs(data_block_manager_t::gc_write_t *gc_writes, int num_writes, data_block_manager_t::gc_write_callback_t *cb) {

    struct gc_callback_wrapper_t : public write_txn_callback_t {
        virtual void on_serializer_write_txn() {
            target->on_gc_write_done();
            delete this;
        }
        std::vector<write_t> writes;
        data_block_manager_t::gc_write_callback_t *target;
    } *cb_wrapper = new gc_callback_wrapper_t;
    cb_wrapper->target = cb;

    for (int i = 0; i < num_writes; i++) {
        /* If the block is not currently in use, we must write a deletion. Otherwise we
        would write the zero-buf that was written when we first deleted it. */
        if (block_in_use(gc_writes[i].block_id)) {
            /* make_internal() makes a write_t that will not change the timestamp. */
            cb_wrapper->writes.push_back(write_t::make_internal(gc_writes[i].block_id, gc_writes[i].buf, NULL));
        } else {
            // TODO: Does this assert fail for large values now?
            rassert(memcmp(gc_writes[i].buf, "zero", 4) == 0);   // Check for zerobuf magic
            cb_wrapper->writes.push_back(write_t::make_internal(gc_writes[i].block_id, NULL, NULL));
        }
    }

    if (do_write(cb_wrapper->writes.data(), cb_wrapper->writes.size(), cb_wrapper)) {
        delete cb_wrapper;
        return true;
    } else {
        return false;
    }
}

perfmon_duration_sampler_t pm_serializer_reads("serializer_reads", secs_to_ticks(1));

/*struct ls_read_fsm_t :
    private iocallback_t,
    private lock_available_callback_t
{
    ticks_t start_time;
    log_serializer_t *ser;
    ser_block_id_t block_id;
    void *buf;
    
    ls_read_fsm_t(log_serializer_t *ser, ser_block_id_t block_id, void *buf)
        : ser(ser), block_id(block_id), buf(buf)
    {
        pm_serializer_reads.begin(&start_time);
        callback = NULL;
        done = false;
    }
    
    ~ls_read_fsm_t() {
        pm_serializer_reads.end(&start_time);
    }
    
    serializer_t::read_callback_t *callback;
    bool done;
    
    void run() {
        ser->main_mutex.lock(this);
    }

    void on_lock_available() {

        // Should not cause anything else to go immediately
        ser->main_mutex.unlock();
        
        flagged_off64_t offset = ser->lba_index->get_block_offset(block_id);
        rassert(!offset.parts.is_delete);   // Make sure the block actually exists

        if (ser->data_block_manager->read(offset.parts.value, buf, this)) {
            on_io_complete();
        }
    }

    void on_io_complete() {
        done = true;

        if (callback) {
            callback->on_serializer_read();
            delete this;
        }
    }
};

bool log_serializer_t::do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
    rassert(state == state_ready);
    assert_thread();

    ls_read_fsm_t *fsm = new ls_read_fsm_t(this, block_id, buf);
    fsm->run();
    if (fsm->done) {
        delete fsm;
        return true;
    } else {
        fsm->callback = callback;
        return false;
    }
}
*/

void log_serializer_t::block_read(boost::shared_ptr<block_token_t> token, void *buf) {
    ls_block_token_t *ls_token = dynamic_cast<ls_block_token_t*>(token.get());
    rassert(ls_token);
    assert_thread();
    rassert(state == state_ready);
    rassert(token_offsets.find(ls_token) != token_offsets.end());

    // Is this necessary?
    co_lock_mutex(&main_mutex);

    const off64_t offset = token_offsets[ls_token];

    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    if (!data_block_manager->read(offset, buf, &cb)) cb.wait();

    main_mutex.unlock();
}

boost::shared_ptr<serializer_t::block_token_t> log_serializer_t::index_read(ser_block_id_t block_id) {
    assert_thread();
    rassert(state == state_ready);
    
    co_lock_mutex(&main_mutex);

    flagged_off64_t offset = lba_index->get_block_offset(block_id);
    rassert(!offset.parts.is_delete);   // Make sure the block actually exists

    main_mutex.unlock();

    return boost::shared_ptr<block_token_t>(new ls_block_token_t(this, offset.parts.value));
}

void log_serializer_t::index_write(const std::vector<index_write_op_t*>& write_ops) {
    index_write_context_t context;

    index_write_prepare(context);

    for (std::vector<index_write_op_t*>::const_iterator write_op_it = write_ops.begin();
            write_op_it != write_ops.end(); ++write_op_it) {

        /*
          struct index_write_delete_t : public index_write_op_t
          struct index_write_recency_t : public index_write_op_t
          struct index_write_block_t : public index_write_op_t
         */
        // Use RTTI to find out what we have... (we don't have many options, so performance should be fine)
        if (dynamic_cast<const index_write_delete_t*>(*write_op_it)) {
            // deletion
            const index_write_delete_t &delete_op = dynamic_cast<const index_write_delete_t&>(**write_op_it);

            lba_index->set_block_offset(delete_op.block_id, repli_timestamp::invalid, flagged_off64_t::delete_id());

        } else if (dynamic_cast<const index_write_recency_t*>(*write_op_it)) {
            // write recency
            const index_write_recency_t &recency_op = dynamic_cast<const index_write_recency_t&>(**write_op_it);

            lba_index->set_block_offset(recency_op.block_id, recency_op.recency, lba_index->get_block_offset(recency_op.block_id));

        } else if (dynamic_cast<const index_write_block_t*>(*write_op_it)) {
            // update the offset or add a new index for the block
            const index_write_block_t &block_op = dynamic_cast<const index_write_block_t&>(**write_op_it);

            /* mark the garbage */
            flagged_off64_t gc_offset = lba_index->get_block_offset(block_op.block_id);
            if (flagged_off64_t::can_be_gced(gc_offset)) {
                data_block_manager->mark_garbage(gc_offset.parts.value);
            }

            /* Keep recency intact. If a new recency should be set, a separate write_recency_op is required */
            const repli_timestamp recency = lba_index->get_block_recency(block_op.block_id);

            ls_block_token_t *ls_token = dynamic_cast<ls_block_token_t*>(block_op.token.get());
            rassert(ls_token);
            rassert(token_offsets.find(ls_token) != token_offsets.end());
            const off64_t offset = token_offsets[ls_token];

            lba_index->set_block_offset(block_op.block_id, recency, flagged_off64_t::real(offset));

        } else {
            unreachable("Unhandled write operation supplied to index_write()");
        }
    }

    index_write_finish(context);
}

void log_serializer_t::index_write_prepare(index_write_context_t &context) {
#ifndef NDEBUG
    {
        log_serializer_t::metablock_t _debug_mb_buffer;
        prepare_metablock(&_debug_mb_buffer);
        // Make sure that the metablock has not changed since the last
        // time we recorded it
        rassert(memcmp(&_debug_mb_buffer, &debug_mb_buffer, sizeof(debug_mb_buffer)) == 0);
    }
#endif

    current_transaction_id++;
    active_write_count++;

    /* Start an extent manager transaction so we can allocate and release extents */
    context.extent_txn = extent_manager->begin_transaction();

    /* Just to make sure that the LBA GC gets exercised */
    lba_index->consider_gc();
}

void log_serializer_t::index_write_finish(index_write_context_t &context) {
    metablock_t mb_buffer;

    /* Sync the LBA */
    struct : public cond_t, public lba_index_t::sync_callback_t {
        void on_lba_sync() { pulse(); }
    } on_lba_sync;
    const bool offsets_were_written = lba_index->sync(&on_lba_sync);

    /* Prepare metablock now instead of in when we write it so that we will have the correct
    metablock information for this write even if another write starts before we finish writing
    our data and LBA. */
    prepare_metablock(&mb_buffer);

    /* Stop the extent manager transaction so another one can start, but don't commit it
    yet */
    extent_manager->end_transaction(context.extent_txn);

    /* Get in line for the metablock manager */
    bool waiting_for_prev_write;
    cond_t on_prev_write_submitted_metablock;
    if (last_write) {
        last_write->next_metablock_write = &on_prev_write_submitted_metablock;
        waiting_for_prev_write = true;
    } else {
        waiting_for_prev_write = false;
    }
    last_write = &context;

    if (!offsets_were_written) on_lba_sync.wait();
    if (waiting_for_prev_write) on_prev_write_submitted_metablock.wait();

#ifndef NDEBUG
    prepare_metablock(&debug_mb_buffer);
#endif

    struct : public cond_t, public mb_manager_t::metablock_write_callback_t {
        void on_metablock_write() { pulse(); }
    } on_metablock_write;
    const bool done_with_metablock = metablock_manager->write_metablock(&mb_buffer, &on_metablock_write);

    /* If there was another transaction waiting for us to write our metablock so it could
    write its metablock, notify it now so it can write its metablock. */
    if (context.next_metablock_write) {
        context.next_metablock_write->pulse();
    } else {
        rassert(&context == last_write);
        last_write = NULL;
    }

    if (!done_with_metablock) on_metablock_write.wait();

    active_write_count--;

    /* End the extent manager transaction so the extents can actually get reused. */
    extent_manager->commit_transaction(context.extent_txn);

    //TODO I'm kind of unhappy that we're calling this from in here we should figure out better where to trigger gc
    consider_start_gc();

    // If we were in the process of shutting down and this is the
    // last transaction, shut ourselves down for good.
    if(state == log_serializer_t::state_shutting_down
       && shutdown_state == log_serializer_t::shutdown_waiting_on_serializer
       && last_write == NULL
       && active_write_count == 0) {
        
        next_shutdown_step();
    }
}

boost::shared_ptr<serializer_t::block_token_t> log_serializer_t::block_write(void *buf) {
    const off64_t offset = data_block_manager->write(buf, true);
    return boost::shared_ptr<block_token_t>(new ls_block_token_t(this, offset));
}

void log_serializer_t::register_block_token(ls_block_token_t *token, off64_t offset) {
    rassert(token_offsets.find(token) == token_offsets.end());
    token_offsets[token] = offset;

    const bool first_token_for_offset = offset_tokens.find(offset) == offset_tokens.end();
    if (first_token_for_offset) {
        // Mark offset live in GC
        data_block_manager->mark_token_live(offset);
    }

    offset_tokens.insert(std::pair<off64_t, ls_block_token_t*>(offset, token));
}

void log_serializer_t::unregister_block_token(ls_block_token_t *token) {    
    std::map<ls_block_token_t*, off64_t>::iterator token_offset_it = token_offsets.find(token);
    rassert(token_offset_it != token_offsets.end());

    for (std::multimap<off64_t, ls_block_token_t*>::iterator offset_token_it = offset_tokens.find(token_offset_it->second);
            offset_token_it != offset_tokens.end() && offset_token_it->first == token_offset_it->second;
            ++offset_token_it) {

        if (offset_token_it->second == token) {
            offset_tokens.erase(offset_token_it);
            break;
        }
    }

    const bool last_token_for_offset = offset_tokens.find(token_offset_it->second) == offset_tokens.end();
    if (last_token_for_offset) {
        // Mark offset garbage in GC
        data_block_manager->mark_token_garbage(token_offset_it->second);
    }
    
    token_offsets.erase(token_offset_it);
}

void log_serializer_t::remap_block_to_new_offset(off64_t current_offset, off64_t new_offset) {
    rassert(new_offset != current_offset);
    for (std::multimap<off64_t, ls_block_token_t*>::iterator offset_token_it = offset_tokens.find(current_offset);
            offset_token_it != offset_tokens.end() && offset_token_it->first == current_offset;
            ++offset_token_it) {

        rassert(token_offsets[offset_token_it->second] == current_offset);
        token_offsets[offset_token_it->second] = new_offset;
        offset_tokens.insert(std::pair<off64_t, ls_block_token_t*>(new_offset, offset_token_it->second));
        offset_tokens.erase(offset_token_it);
    }
}

// The block_id is there to keep the interface independent from the serializer
// implementation. This method should work even if there's no transaction id in
// buf.
ser_transaction_id_t log_serializer_t::get_current_transaction_id(UNUSED ser_block_id_t block_id, const void* buf) {
    buf_data_t *ser_data = (buf_data_t*)buf;
    ser_data--;
    rassert(block_id == ser_data->block_id);
    return ser_data->transaction_id;
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

bool log_serializer_t::shutdown(cond_t *cb) {
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
        if (!shutdown_in_one_shot && shutdown_callback) shutdown_callback->pulse();

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

void log_serializer_t::register_read_ahead_cb(read_ahead_callback_t *cb) {
    if (get_thread_id() != home_thread) {
        do_on_thread(home_thread, boost::bind(&log_serializer_t::register_read_ahead_cb, this, cb));
        return;
    }

    read_ahead_callbacks.push_back(cb);
}

void log_serializer_t::unregister_read_ahead_cb(read_ahead_callback_t *cb) {
    if (get_thread_id() != home_thread) {
        do_on_thread(home_thread, boost::bind(&log_serializer_t::unregister_read_ahead_cb, this, cb));
        return;
    }

    for (std::vector<read_ahead_callback_t*>::iterator cb_it = read_ahead_callbacks.begin(); cb_it != read_ahead_callbacks.end(); ++cb_it) {
        if (*cb_it == cb) {
            read_ahead_callbacks.erase(cb_it);
            break;
        }
    }
}

bool log_serializer_t::offer_buf_to_read_ahead_callbacks(ser_block_id_t block_id, void *buf) {
    for (size_t i = 0; i < read_ahead_callbacks.size(); ++i) {
        if (read_ahead_callbacks[i]->offer_read_ahead_buf(block_id, buf)) {
            return true;
        }
    }
    return false;
}

bool log_serializer_t::should_perform_read_ahead() {
    assert_thread();
    return dynamic_config->read_ahead && !read_ahead_callbacks.empty();
}

