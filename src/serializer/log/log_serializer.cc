// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/log_serializer.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>

#include "arch/io/disk.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/new_mutex.hpp"
#include "logger.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/log/data_block_manager.hpp"

filepath_file_opener_t::filepath_file_opener_t(const serializer_filepath_t &filepath,
                                               io_backender_t *backender)
    : filepath_(filepath),
      backender_(backender),
      opened_temporary_(false) { }

filepath_file_opener_t::~filepath_file_opener_t() { }

std::string filepath_file_opener_t::file_name() const {
    return filepath_.permanent_path();
}

std::string filepath_file_opener_t::temporary_file_name() const {
#ifdef _WIN32
    // TODO WINDOWS: use temporary files
    return filepath_.permanent_path();
#else
    return filepath_.temporary_path();
#endif
}

std::string filepath_file_opener_t::current_file_name() const {
    return opened_temporary_ ? temporary_file_name() : file_name();
}

void filepath_file_opener_t::open_serializer_file(const std::string &path, int extra_flags, scoped_ptr_t<file_t> *file_out) {
    const file_open_result_t res = open_file(path.c_str(),
                                             linux_file_t::mode_read | linux_file_t::mode_write | extra_flags,
                                             backender_,
                                             file_out);
    if (res.outcome == file_open_result_t::ERROR) {
        crash_due_to_inaccessible_database_file(path.c_str(), res);
    }

    if (res.outcome == file_open_result_t::BUFFERED_FALLBACK) {
        logWRN("Could not turn off filesystem caching for database file: \"%s\" "
               "(Is the file located on a filesystem that doesn't support direct I/O "
               "(e.g. some encrypted or journaled file systems)?) "
               "This can cause performance problems.",
               path.c_str());
    }
}

void filepath_file_opener_t::open_serializer_file_create_temporary(scoped_ptr_t<file_t> *file_out) {
    mutex_assertion_t::acq_t acq(&reentrance_mutex_);
    open_serializer_file(temporary_file_name(), linux_file_t::mode_create | linux_file_t::mode_truncate, file_out);
    opened_temporary_ = true;
}

void filepath_file_opener_t::move_serializer_file_to_permanent_location() {
    // TODO: Make caller not require that this not block, run ::rename in a blocker pool.
    ASSERT_NO_CORO_WAITING;

    mutex_assertion_t::acq_t acq(&reentrance_mutex_);

    guarantee(opened_temporary_);

#ifdef _WIN32
    // TODO WINDOWS: temporary files are not used because, by default,
    // files cannot be renamed while still open
#else
    const int res = ::rename(temporary_file_name().c_str(), file_name().c_str());

    if (res != 0) {
        crash("Could not rename database file %s to permanent location %s (%s)\n",
              temporary_file_name().c_str(), file_name().c_str(),
              errno_string(errno).c_str());
    }

    warn_fsync_parent_directory(file_name().c_str());
#endif

    opened_temporary_ = false;
}

void filepath_file_opener_t::open_serializer_file_existing(scoped_ptr_t<file_t> *file_out) {
    mutex_assertion_t::acq_t acq(&reentrance_mutex_);
    open_serializer_file(current_file_name(), 0, file_out);
}

void filepath_file_opener_t::unlink_serializer_file() {
    // TODO: Make caller not require that this not block, run ::unlink in a blocker pool.
    ASSERT_NO_CORO_WAITING;

    mutex_assertion_t::acq_t acq(&reentrance_mutex_);
    guarantee(opened_temporary_);
    const int res = ::unlink(current_file_name().c_str());
    guarantee_err(res == 0, "unlink() failed");
}



log_serializer_stats_t::log_serializer_stats_t(perfmon_collection_t *parent)
    : serializer_collection(),
      pm_serializer_block_reads(secs_to_ticks(1)),
      pm_serializer_index_reads(),
      pm_serializer_block_writes(),
      pm_serializer_index_writes(secs_to_ticks(1)),
      pm_serializer_index_writes_size(secs_to_ticks(1), false),
      pm_serializer_read_bytes_per_sec(secs_to_ticks(1)),
      pm_serializer_read_bytes_total(),
      pm_serializer_written_bytes_per_sec(secs_to_ticks(1)),
      pm_serializer_written_bytes_total(),
      pm_extents_in_use(),
      pm_bytes_in_use(),
      pm_serializer_lba_extents(),
      pm_serializer_data_extents(),
      pm_serializer_data_extents_allocated(),
      pm_serializer_data_extents_gced(),
      pm_serializer_old_garbage_block_bytes(),
      pm_serializer_old_total_block_bytes(),
      pm_serializer_lba_gcs(),
      parent_collection_membership(parent, &serializer_collection, "serializer"),
      stats_membership(&serializer_collection,
          &pm_serializer_block_reads, "serializer_block_reads",
          &pm_serializer_index_reads, "serializer_index_reads",
          &pm_serializer_block_writes, "serializer_block_writes",
          &pm_serializer_index_writes, "serializer_index_writes",
          &pm_serializer_index_writes_size, "serializer_index_writes_size",
          &pm_serializer_read_bytes_per_sec, "serializer_read_bytes_per_sec",
          &pm_serializer_read_bytes_total, "serializer_read_bytes_total",
          &pm_serializer_written_bytes_per_sec, "serializer_written_bytes_per_sec",
          &pm_serializer_written_bytes_total, "serializer_written_bytes_total",
          &pm_extents_in_use, "serializer_extents_in_use",
          &pm_bytes_in_use, "serializer_bytes_in_use",
          &pm_serializer_lba_extents, "serializer_lba_extents",
          &pm_serializer_data_extents, "serializer_data_extents",
          &pm_serializer_data_extents_allocated, "serializer_data_extents_allocated",
          &pm_serializer_data_extents_gced, "serializer_data_extents_gced",
          &pm_serializer_old_garbage_block_bytes, "serializer_old_garbage_block_bytes",
          &pm_serializer_old_total_block_bytes, "serializer_old_total_block_bytes",
          &pm_serializer_lba_gcs, "serializer_lba_gcs")
{ }

void log_serializer_stats_t::bytes_read(size_t count) {
    pm_serializer_read_bytes_per_sec.record(count);
    pm_serializer_read_bytes_total += count;
}

void log_serializer_stats_t::bytes_written(size_t count) {
    pm_serializer_written_bytes_per_sec.record(count);
    pm_serializer_written_bytes_total += count;
}

void log_serializer_t::create(serializer_file_opener_t *file_opener, static_config_t static_config) {
    log_serializer_on_disk_static_config_t *on_disk_config = &static_config;

    scoped_ptr_t<file_t> file;
    file_opener->open_serializer_file_create_temporary(&file);

    co_static_header_write(file.get(), on_disk_config, sizeof(*on_disk_config));

    metablock_t metablock;
    memset(&metablock, 0, sizeof(metablock));

    extent_manager_t::prepare_initial_metablock(&metablock.extent_manager_part);

    data_block_manager_t::prepare_initial_metablock(&metablock.data_block_manager_part);
    lba_list_t::prepare_initial_metablock(&metablock.lba_index_part);

    mb_manager_t::create(file.get(), static_config.extent_size(), &metablock);
}

/* The process of starting up the serializer is handled by the ls_start_*_fsm_t. This is not
necessary, because there is only ever one startup process for each serializer; the serializer could
handle its own startup process. It is done this way to make it clear which parts of the serializer
are involved in startup and which parts are not. */

struct ls_start_existing_fsm_t :
    public static_header_read_callback_t,
    public mb_manager_t::metablock_read_callback_t,
    public lba_list_t::ready_callback_t,
    public thread_message_t
{
    explicit ls_start_existing_fsm_t(log_serializer_t *serializer)
        : ser(serializer), start_existing_state(state_start) {
    }

    ~ls_start_existing_fsm_t() {
    }

    bool run(cond_t *to_signal, serializer_file_opener_t *file_opener) {
        // STATE A
        rassert(start_existing_state == state_start);
        rassert(ser->state == log_serializer_t::state_unstarted);
        ser->state = log_serializer_t::state_starting_up;

        scoped_ptr_t<file_t> dbfile;
        file_opener->open_serializer_file_existing(&dbfile);
        ser->dbfile = dbfile.release();
        ser->index_writes_io_account.init(
            new file_account_t(ser->dbfile, INDEX_WRITE_IO_PRIORITY));

        start_existing_state = state_read_static_header;
        // STATE A above implies STATE B here
        to_signal_when_done = nullptr;
        if (next_starting_up_step()) {
            return true;
        } else {
            to_signal_when_done = to_signal;
            return false;
        }
    }

    bool next_starting_up_step() {
        if (start_existing_state == state_read_static_header) {
            // STATE B
            static_header_read(ser->dbfile,
                &ser->static_config,
                sizeof(log_serializer_on_disk_static_config_t),
                &ser->static_header_needs_migration,
                this);
            start_existing_state = state_waiting_for_static_header;
            // STATE B above implies STATE C here
            return false;
        }

        rassert(start_existing_state != state_waiting_for_static_header);

        if (start_existing_state == state_find_metablock) {
            // STATE D
            ser->extent_manager = new extent_manager_t(ser->dbfile, &ser->static_config,
                                                       ser->stats.get());
            {
                // We never end up releasing the static header extent reference.  Nobody says we
                // have to.
                extent_reference_t extent_ref
                    = ser->extent_manager->reserve_extent(0);  // For static header.
                UNUSED int64_t extent = extent_ref.release();
            }

            ser->metablock_manager = new mb_manager_t(ser->extent_manager);
            ser->lba_index = new lba_list_t(ser->extent_manager,
                    std::bind(&log_serializer_t::write_metablock_sans_pipelining,
                              ser, ph::_1, ph::_2));
            ser->data_block_manager
                = new data_block_manager_t(ser->extent_manager, ser,
                                           &ser->static_config, ser->stats.get());

            // STATE E
            if (ser->metablock_manager->start_existing(ser->dbfile, &metablock_found, &metablock_buffer, this)) {
                crash("metablock_manager_t::start_existing always returns false");
                // start_existing_state = state_start_lba;
            } else {
                // STATE E
                start_existing_state = state_waiting_for_metablock;
                return false;
            }
        }

        if (start_existing_state == state_start_lba) {
            // STATE G
            guarantee(metablock_found, "Could not find any valid metablock.");

            // STATE H
            if (ser->lba_index->start_existing(ser->dbfile, &metablock_buffer.lba_index_part, this)) {
                start_existing_state = state_reconstruct;
                // STATE J
            } else {
                // STATE H
                start_existing_state = state_waiting_for_lba;
                // STATE I
                return false;
            }
        }

        if (start_existing_state == state_reconstruct) {
            ser->data_block_manager->start_reconstruct();
            start_existing_state = state_reconstruct_ongoing;
            next_block_to_reconstruct = 0;
            // Fall through into state_reconstruct_ongoing
        }

        if (start_existing_state == state_reconstruct_ongoing) {
            int batch = 0;
            while (true) {
                // Once we are done with the normal blocks, switch over to the aux blocks.
                if (!is_aux_block_id(next_block_to_reconstruct)
                    && next_block_to_reconstruct >= ser->lba_index->end_block_id()) {
                    next_block_to_reconstruct = FIRST_AUX_BLOCK_ID;
                }
                if (next_block_to_reconstruct >= ser->lba_index->end_aux_block_id()) {
                    break;
                }

                flagged_off64_t offset =
                    ser->lba_index->get_block_offset(next_block_to_reconstruct);
                if (offset.has_value()) {
                    ser->data_block_manager->mark_live(offset.get_value(),
                        ser->lba_index->get_block_size(next_block_to_reconstruct));
                }

                ++next_block_to_reconstruct;
                ++batch;
                if (batch >= LBA_RECONSTRUCTION_BATCH_SIZE) {
                    call_later_on_this_thread(this);
                    return false;
                }
            }
            ser->data_block_manager->end_reconstruct();
            ser->data_block_manager->start_existing(ser->dbfile, &metablock_buffer.data_block_manager_part);

            ser->extent_manager->start_existing(&metablock_buffer.extent_manager_part);

            start_existing_state = state_finish;
        }

        if (start_existing_state == state_finish) {
            start_existing_state = state_done;
            rassert(ser->state == log_serializer_t::state_starting_up);
            ser->state = log_serializer_t::state_ready;

            if (to_signal_when_done) to_signal_when_done->pulse();

            delete this;
            return true;
        }

        unreachable("Invalid state %d.", start_existing_state);
    }

    void on_static_header_read() {
        rassert(start_existing_state == state_waiting_for_static_header);
        // STATE C
        start_existing_state = state_find_metablock;
        // STATE C above implies STATE D here
        next_starting_up_step();
    }

    void on_metablock_read() {
        rassert(start_existing_state == state_waiting_for_metablock);
        // state after F, state before G
        start_existing_state = state_start_lba;
        // STATE G
        next_starting_up_step();
    }

    void on_lba_ready() {
        rassert(start_existing_state == state_waiting_for_lba);
        start_existing_state = state_reconstruct;
        next_starting_up_step();
    }

    void on_thread_switch() {
        // Continue a previously started LBA reconstruction
        rassert(start_existing_state == state_reconstruct_ongoing);
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
        state_reconstruct_ongoing,
        state_finish,
        state_done
    } start_existing_state;

    // When in state_reconstruct_ongoing, we keep track of how many blocks we
    // already have reconstructed.
    block_id_t next_block_to_reconstruct;

    bool metablock_found;
    log_serializer_t::metablock_t metablock_buffer;

private:
    DISABLE_COPYING(ls_start_existing_fsm_t);
};

log_serializer_t::log_serializer_t(dynamic_config_t _dynamic_config, serializer_file_opener_t *file_opener, perfmon_collection_t *_perfmon_collection)
    : stats(new log_serializer_stats_t(_perfmon_collection)),  // can block in a perfmon_collection_t::add call.
      disk_stats_collection(),
      disk_stats_membership(_perfmon_collection, &disk_stats_collection, "disk"),  // can block in a perfmon_collection_t::add call.
#ifndef NDEBUG
      expecting_no_more_tokens(false),
#endif
      dynamic_config(_dynamic_config),
      shutdown_callback(nullptr),
      shutdown_state(shutdown_not_started),
      state(state_unstarted),
      static_header_needs_migration(false),
      dbfile(nullptr),
      extent_manager(nullptr),
      metablock_manager(nullptr),
      lba_index(nullptr),
      data_block_manager(nullptr),
      active_write_count(0) {
    // STATE A
    /* This is because the serializer is not completely converted to coroutines yet. */
    ls_start_existing_fsm_t *s = new ls_start_existing_fsm_t(this);
    cond_t cond;
    if (!s->run(&cond, file_opener)) cond.wait();
}

log_serializer_t::~log_serializer_t() {
    assert_thread();

    cond_t cond;
    shutdown(&cond);
    cond.wait();

    rassert(state == state_unstarted || state == state_shut_down);
    rassert(metablock_waiter_queue.empty());
    rassert(active_write_count == 0);
}

file_account_t *log_serializer_t::make_io_account(int priority, int outstanding_requests_limit) {
    assert_thread();
    rassert(dbfile);
    return new file_account_t(dbfile, priority, outstanding_requests_limit);
}

buf_ptr_t log_serializer_t::block_read(const counted_t<ls_block_token_pointee_t> &token,
                                     file_account_t *io_account) {
    assert_thread();
    guarantee(token.has());
    guarantee(state == state_ready);

    ticks_t pm_time;
    stats->pm_serializer_block_reads.begin(&pm_time);

    buf_ptr_t ret = data_block_manager->read(token->offset_, token->block_size(),
                                           io_account);

    stats->pm_serializer_block_reads.end(&pm_time);
    return ret;
}

// God this is such a hack.
#ifndef SEMANTIC_SERIALIZER_CHECK
counted_t<ls_block_token_pointee_t>
get_ls_block_token(const counted_t<ls_block_token_pointee_t> &tok) {
    return tok;
}
#else
counted_t<ls_block_token_pointee_t>
get_ls_block_token(const counted_t<scs_block_token_t<log_serializer_t> > &tok) {
    if (tok) {
        return tok->inner_token;
    } else {
        return counted_t<ls_block_token_pointee_t>();
    }
}
#endif  // SEMANTIC_SERIALIZER_CHECK


void log_serializer_t::index_write(new_mutex_in_line_t *mutex_acq,
                                   const std::function<void()> &on_writes_reflected,
                                   const std::vector<index_write_op_t> &write_ops) {
    assert_thread();
    ticks_t pm_time;
    stats->pm_serializer_index_writes.begin(&pm_time);
    stats->pm_serializer_index_writes_size.record(write_ops.size());

    extent_transaction_t txn;
    index_write_prepare(&txn);

    {
        // The in-memory index updates, at least due to the needs of
        // data_block_manager_t garbage collection, needs to be
        // atomic.
        ASSERT_NO_CORO_WAITING;

        for (std::vector<index_write_op_t>::const_iterator write_op_it = write_ops.begin();
             write_op_it != write_ops.end();
             ++write_op_it) {
            const index_write_op_t &op = *write_op_it;
            flagged_off64_t offset = lba_index->get_block_offset(op.block_id);
            uint32_t ser_block_size = lba_index->get_ser_block_size(op.block_id);

            if (op.token) {
                // Update the offset pointed to, and mark garbage/liveness as necessary.
                counted_t<ls_block_token_pointee_t> token
                    = get_ls_block_token(op.token.get());

                // Mark old offset as garbage
                if (offset.has_value()) {
                    data_block_manager->mark_garbage(offset.get_value(), &txn);
                }

                // Write new token to index, or remove from index as appropriate.
                if (token.has()) {
                    offset = flagged_off64_t::make(token->offset_);
                    ser_block_size = token->block_size().ser_value();

                    /* mark the life */
                    data_block_manager->mark_live(offset.get_value(), token->block_size());
                } else {
                    offset = flagged_off64_t::unused();
                    ser_block_size = 0;
                }
            }

            repli_timestamp_t recency = op.recency ? op.recency.get()
                : lba_index->get_block_recency(op.block_id);

            lba_index->set_block_info(op.block_id, recency,
                                      offset, ser_block_size,
                                      index_writes_io_account.get(), &txn);
        }
    }

    // All changes are now in the in-memory LBA
    on_writes_reflected();

    // Before we fully commit the write to disk, we must migrate the static header
    // if necessary.
    // Note that this is early enough for upgrading from the 1.13 serializer
    // version to 2.2, since only the format of the LBA changed.
    // Future serializer format changes might require this step to happen earlier.
    {
        new_mutex_acq_t acq(&static_header_migration_mutex);
        if (static_header_needs_migration) {
            static_header_needs_migration = false;
            migrate_static_header(dbfile, sizeof(log_serializer_on_disk_static_config_t));
        }
    }

    index_write_finish(mutex_acq, &txn, index_writes_io_account.get());

    stats->pm_serializer_index_writes.end(&pm_time);
}

void log_serializer_t::index_write_prepare(extent_transaction_t *txn) {
    assert_thread();
    active_write_count++;

    /* Just to make sure that the LBA GC gets exercised */
    lba_index->consider_gc();

    /* Start an extent manager transaction so we can allocate and release extents */
    extent_manager->begin_transaction(txn);
}

void log_serializer_t::index_write_finish(new_mutex_in_line_t *mutex_acq,
                                          extent_transaction_t *txn,
                                          file_account_t *io_account) {
    /* Sync the LBA */
    struct : public cond_t, public lba_list_t::sync_callback_t {
        void on_lba_sync() { pulse(); }
    } on_lba_sync;
    lba_index->sync(io_account, &on_lba_sync);

    /* Stop the extent manager transaction so another one can start, but don't commit it
    yet */
    extent_manager->end_transaction(txn);

    /* Write the metablock */
    write_metablock(mutex_acq, &on_lba_sync, io_account);

    active_write_count--;

    /* End the extent manager transaction so the extents can actually get reused. */
    extent_manager->commit_transaction(txn);

    /* Note that it's important that we don't start GCing before we finish the first
    index write, because of how we migrate the static header of the serializer file
    (see the call to `migrate_static_header` above). We don't want to migrate it before
    we get an *explicit* index write. Specifically we don't want to migrate the static
    header due to garbage collection. */
    consider_start_gc();

    // If we were in the process of shutting down and this is the
    // last transaction, shut ourselves down for good.
    if (state == log_serializer_t::state_shutting_down
        && shutdown_state == log_serializer_t::shutdown_waiting_on_serializer
        && metablock_waiter_queue.empty()
        && active_write_count == 0) {

        next_shutdown_step();
    }
}

void log_serializer_t::write_metablock(new_mutex_in_line_t *mutex_acq,
                                       const signal_t *safe_to_write_cond,
                                       file_account_t *io_account) {
    assert_thread();
    metablock_t mb_buffer;

    /* Prepare metablock now instead of in when we write it so that we will have the correct
    metablock information for this write even if another write starts before we finish
    waiting on `safe_to_write_cond`. */
    prepare_metablock(&mb_buffer);

    /* Get in line for the metablock manager */
    bool waiting_for_prev_write = !metablock_waiter_queue.empty();
    cond_t on_prev_write_submitted_metablock;
    metablock_waiter_queue.push_back(&on_prev_write_submitted_metablock);

    // This operation is in line with the metablock manager.  Now another index write
    // may commence.
    mutex_acq->reset();

    safe_to_write_cond->wait();
    if (waiting_for_prev_write) {
        on_prev_write_submitted_metablock.wait();
    }
    guarantee(metablock_waiter_queue.front() == &on_prev_write_submitted_metablock);

    struct : public cond_t, public mb_manager_t::metablock_write_callback_t {
        void on_metablock_write() { pulse(); }
    } on_metablock_write;
    const bool done_with_metablock =
        metablock_manager->write_metablock(&mb_buffer, io_account, &on_metablock_write);

    /* Remove ourselves from the list of metablock waiters. */
    metablock_waiter_queue.pop_front();

    /* If there was another transaction waiting for us to write our metablock so it could
    write its metablock, notify it now so it can write its metablock. */
    if (!metablock_waiter_queue.empty()) {
        metablock_waiter_queue.front()->pulse();
    }

    if (!done_with_metablock) on_metablock_write.wait();
}

void log_serializer_t::write_metablock_sans_pipelining(const signal_t *safe_to_write_cond,
                                                       file_account_t *io_account) {
    new_mutex_in_line_t dummy_acq;
    write_metablock(&dummy_acq, safe_to_write_cond, io_account);

}

counted_t<ls_block_token_pointee_t>
log_serializer_t::generate_block_token(int64_t offset, block_size_t block_size) {
    assert_thread();
    counted_t<ls_block_token_pointee_t> ret(new ls_block_token_pointee_t(this, offset, block_size));
    return ret;
}

std::vector<counted_t<ls_block_token_pointee_t> >
log_serializer_t::block_writes(const std::vector<buf_write_info_t> &write_infos,
                               file_account_t *io_account, iocallback_t *cb) {
    assert_thread();
    stats->pm_serializer_block_writes += write_infos.size();

    std::vector<counted_t<ls_block_token_pointee_t> > result
        = data_block_manager->many_writes(write_infos, io_account, cb);
    guarantee(result.size() == write_infos.size());
    return result;
}

void log_serializer_t::register_block_token(ls_block_token_pointee_t *token, int64_t offset) {
    assert_thread();
    rassert(token->offset_ == offset);  // Assert *token was constructed properly.

    auto location = offset_tokens.find(offset);
    if (location == offset_tokens.end()) {
        data_block_manager->mark_live_tokenwise_with_offset(offset);
    }

    offset_tokens.insert(location, std::make_pair(offset, token));
}

bool log_serializer_t::tokens_exist_for_offset(int64_t off) {
    assert_thread();
    return offset_tokens.find(off) != offset_tokens.end();
}

void log_serializer_t::unregister_block_token(ls_block_token_pointee_t *token) {
    assert_thread();

    ASSERT_NO_CORO_WAITING;

    rassert(!expecting_no_more_tokens);

    {
        typedef std::multimap<int64_t, ls_block_token_pointee_t *>::iterator ot_iter;
        ot_iter erase_it = offset_tokens.end();
        for (std::pair<ot_iter, ot_iter> range = offset_tokens.equal_range(token->offset_);
             range.first != range.second;
             ++range.first) {
            if (range.first->second == token) {
                erase_it = range.first;
                break;
            }
        }

        guarantee(erase_it != offset_tokens.end(), "We probably tried unregistering the same token twice.");
        offset_tokens.erase(erase_it);
    }

    const bool last_token_for_offset = offset_tokens.find(token->offset_) == offset_tokens.end();
    if (last_token_for_offset) {
        // Mark offset garbage in GC
        data_block_manager->mark_garbage_tokenwise_with_offset(token->offset_);
    }

    if (offset_tokens.empty() && state == state_shutting_down && shutdown_state == shutdown_waiting_on_block_tokens) {
#ifndef NDEBUG
        expecting_no_more_tokens = true;
#endif
        next_shutdown_step();
    }
}

void log_serializer_t::remap_block_to_new_offset(int64_t current_offset, int64_t new_offset) {
    assert_thread();
    ASSERT_NO_CORO_WAITING;

    rassert(new_offset != current_offset);

    typedef std::multimap<int64_t, ls_block_token_pointee_t *>::iterator ot_iter;
    std::pair<ot_iter, ot_iter> range = offset_tokens.equal_range(current_offset);

    if (range.first != range.second) {
        // We need this weird inclusive-range logic because we modify offset_tokens
        // while we iterate over the range.
        --range.second;

        bool last_time = false;
        while (!last_time) {
            last_time = (range.first == range.second);
            ls_block_token_pointee_t *const token = range.first->second;
            guarantee(token->offset_ == current_offset);

            token->offset_ = new_offset;
            offset_tokens.insert(std::pair<int64_t, ls_block_token_pointee_t *>(new_offset, token));

            ot_iter prev = range.first;
            ++range.first;
            offset_tokens.erase(prev);
        }

        data_block_manager->mark_garbage_tokenwise_with_offset(current_offset);
        data_block_manager->mark_live_tokenwise_with_offset(new_offset);
    }
}

max_block_size_t log_serializer_t::max_block_size() const {
    return static_config.max_block_size();
}

bool log_serializer_t::coop_lock_and_check() {
    assert_thread();
    rassert(dbfile != nullptr);
    return dbfile->coop_lock_and_check();
}

bool log_serializer_t::is_gc_active() const {
    return data_block_manager->is_gc_active() || lba_index->is_any_gc_active();
}

block_id_t log_serializer_t::end_block_id() {
    assert_thread();
    rassert(state == state_ready);

    return lba_index->end_block_id();
}

block_id_t log_serializer_t::end_aux_block_id() {
    assert_thread();
    rassert(state == state_ready);

    return lba_index->end_aux_block_id();
}

counted_t<ls_block_token_pointee_t> log_serializer_t::index_read(block_id_t block_id) {
    assert_thread();
    ++stats->pm_serializer_index_reads;

    rassert(state == state_ready);

    if ((is_aux_block_id(block_id) && block_id >= lba_index->end_aux_block_id())
        || (!is_aux_block_id(block_id) && block_id >= lba_index->end_block_id())) {
        return counted_t<ls_block_token_pointee_t>();
    }

    index_block_info_t info = lba_index->get_block_info(block_id);
    if (info.offset.has_value()) {
        return generate_block_token(info.offset.get_value(), block_size_t::unsafe_make(info.ser_block_size));
    } else {
        return counted_t<ls_block_token_pointee_t>();
    }
}

bool log_serializer_t::get_delete_bit(block_id_t id) {
    assert_thread();
    rassert(state == state_ready);

    flagged_off64_t offset = lba_index->get_block_offset(id);
    return !offset.has_value();
}

segmented_vector_t<repli_timestamp_t>
log_serializer_t::get_all_recencies(block_id_t first, block_id_t step) {
    assert_thread();
    return lba_index->get_block_recencies(first, step);
}

void log_serializer_t::shutdown(cond_t *cb) {
    assert_thread();
    rassert(coro_t::self());

    rassert(cb);
    rassert(state == state_ready);
    shutdown_callback = cb;

    rassert(shutdown_state == shutdown_not_started);
    shutdown_state = shutdown_begin;

    // We must shutdown the LBA GC before we shut down
    // the data_block_manager or metablock_manager, because the LBA GC
    // uses our `write_metablock()` method which depends on those.
    // `shutdown_gc()` might block, but uses coroutine waiting in contrast
    // to most of the remaining shutdown process which is still FSM-based.
    lba_index->shutdown_gc();

    // Additionally we tell the data block manager to stop GCing.
    // Not doing this doesn't hurt correctness, but it will delay the shutdown
    // process because GC writes are contributing to `active_write_count` that
    // stops us from getting through the remaining shutdown process quickly.
    data_block_manager->disable_gc();

    next_shutdown_step();
}

void log_serializer_t::next_shutdown_step() {
    assert_thread();

    if (shutdown_state == shutdown_begin) {
        // First shutdown step
        shutdown_state = shutdown_waiting_on_serializer;
        if (!metablock_waiter_queue.empty() || active_write_count > 0) {
            state = state_shutting_down;
            return;
        }
        state = state_shutting_down;
    }

    if (shutdown_state == shutdown_waiting_on_serializer) {
        shutdown_state = shutdown_waiting_on_datablock_manager;
        if (!data_block_manager->shutdown(this)) {
            return;
        }
    }

    // The datablock manager uses block tokens, so it goes before.
    if (shutdown_state == shutdown_waiting_on_datablock_manager) {
        shutdown_state = shutdown_waiting_on_block_tokens;
        if (!offset_tokens.empty()) {
            return;
        } else {
#ifndef NDEBUG
            expecting_no_more_tokens = true;
#endif
        }
    }

    rassert(expecting_no_more_tokens);

    if (shutdown_state == shutdown_waiting_on_block_tokens) {
        lba_index->shutdown();
        metablock_manager->shutdown();
        extent_manager->shutdown();

        delete lba_index;
        lba_index = nullptr;

        delete data_block_manager;
        data_block_manager = nullptr;

        delete metablock_manager;
        metablock_manager = nullptr;

        delete extent_manager;
        extent_manager = nullptr;

        shutdown_state = shutdown_waiting_on_dbfile_destruction;
        coro_t::spawn_sometime(std::bind(&log_serializer_t::delete_dbfile_and_continue_shutdown,
                                         this));
        return;
    }

    rassert(dbfile == nullptr);

    if (shutdown_state == shutdown_waiting_on_dbfile_destruction) {
        state = state_shut_down;

        if (shutdown_callback) {
            shutdown_callback->pulse();
        }
        return;
    }

    unreachable("Invalid state.");
}

void log_serializer_t::delete_dbfile_and_continue_shutdown() {
    index_writes_io_account.reset();
    rassert(dbfile != nullptr);
    delete dbfile;
    dbfile = nullptr;
    next_shutdown_step();
}

void log_serializer_t::on_datablock_manager_shutdown() {
    assert_thread();
    next_shutdown_step();
}

void log_serializer_t::prepare_metablock(metablock_t *mb_buffer) {
    assert_thread();
    memset(mb_buffer, 0, sizeof(*mb_buffer));
    extent_manager->prepare_metablock(&mb_buffer->extent_manager_part);
    data_block_manager->prepare_metablock(&mb_buffer->data_block_manager_part);
    lba_index->prepare_metablock(&mb_buffer->lba_index_part);
}


void log_serializer_t::consider_start_gc() {
    assert_thread();
    if (data_block_manager->do_we_want_to_start_gcing() && state == log_serializer_t::state_ready) {
        // We do not do GC if we're not in the ready state
        // (i.e. shutting down)
        data_block_manager->start_gc();
    }
}


void log_serializer_t::register_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
    assert_thread();

    read_ahead_callbacks.push_back(cb);
}

void log_serializer_t::unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
    assert_thread();

    auto it = std::find(read_ahead_callbacks.begin(), read_ahead_callbacks.end(), cb);
    guarantee(it != read_ahead_callbacks.end());
    read_ahead_callbacks.erase(it);
}

void log_serializer_t::offer_buf_to_read_ahead_callbacks(
        block_id_t block_id,
        buf_ptr_t &&buf,
        const counted_t<standard_block_token_t> &token) {
    assert_thread();

    buf_ptr_t local_buf = std::move(buf);
    for (size_t i = 0; local_buf.has() && i < read_ahead_callbacks.size(); ++i) {
        read_ahead_callbacks[i]->offer_read_ahead_buf(block_id,
                                                      &local_buf,
                                                      token);
    }
}

bool log_serializer_t::should_perform_read_ahead() {
    assert_thread();
    return dynamic_config.read_ahead && !read_ahead_callbacks.empty();
}

ls_block_token_pointee_t::ls_block_token_pointee_t(log_serializer_t *serializer,
                                                   int64_t initial_offset,
                                                   block_size_t initial_block_size)
    : serializer_(serializer), ref_count_(0),
      block_size_(initial_block_size), offset_(initial_offset) {
    serializer_->assert_thread();
    serializer_->register_block_token(this, initial_offset);
}

void ls_block_token_pointee_t::do_destroy() {
    serializer_->assert_thread();
    rassert(ref_count_ == 0);
    serializer_->unregister_block_token(this);
    delete this;
}

void debug_print(printf_buffer_t *buf,
                 const counted_t<ls_block_token_pointee_t> &token) {
    if (token.has()) {
        buf->appendf("ls_block_token{%" PRIi64 ", +%" PRIu32 "}",
                     token->offset(), token->block_size().ser_value());
    } else {
        buf->appendf("nil");
    }
}

void counted_add_ref(ls_block_token_pointee_t *p) {
    DEBUG_VAR intptr_t res = ++(p->ref_count_);
    rassert(res > 0);
}

void counted_release(ls_block_token_pointee_t *p) {
    struct destroyer_t : public linux_thread_message_t {
        void on_thread_switch() {
            rassert(p->ref_count_ == 0);
            p->do_destroy();
            delete this;
        }
        ls_block_token_pointee_t *p;
    };

    intptr_t res = --(p->ref_count_);
    rassert(res >= 0);
    if (res == 0) {
        if (get_thread_id() == p->serializer_->home_thread()) {
            p->do_destroy();
        } else {
            destroyer_t *destroyer = new destroyer_t;
            destroyer->p = p;
            DEBUG_VAR bool cont = continue_on_thread(p->serializer_->home_thread(),
                                                     destroyer);
            rassert(!cont);
        }
    }
}
