// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/lba/extent.hpp"

#include <vector>

#include "arch/arch.hpp"
#include "math.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/stats.hpp"

struct extent_block_t :
    public extent_t::sync_callback_t,
    public iocallback_t
{
    scoped_device_block_aligned_ptr_t<char> data;
    extent_t *parent;
    size_t offset;
    std::vector< extent_t::sync_callback_t* > sync_cbs;
    bool waiting_for_prev, have_finished_sync, is_last_block;

    extent_block_t(extent_t *_parent, size_t _offset)
        : data(DEVICE_BLOCK_SIZE), parent(_parent), offset(_offset) { }

    void write(file_account_t *io_account) {
        waiting_for_prev = true;
        have_finished_sync = false;

        parent->sync(this);

        if (parent->last_block) parent->last_block->is_last_block = false;
        parent->last_block = this;
        is_last_block = true;

        parent->file->write_async(parent->extent_ref.offset() + offset, DEVICE_BLOCK_SIZE,
                                  data.get(), io_account, this, file_t::NO_DATASYNCS);
    }

    void on_extent_sync() {
        rassert(waiting_for_prev);
        waiting_for_prev = false;
        if (have_finished_sync) done();
    }

    void on_io_complete() {
        rassert(!have_finished_sync);
        have_finished_sync = true;
        if (!waiting_for_prev) done();
    }

    void done() {
        for (unsigned i = 0; i < sync_cbs.size(); i++) {
            sync_cbs[i]->on_extent_sync();
        }
        if (is_last_block) {
            rassert(this == parent->last_block);
            parent->last_block = nullptr;
        }
        delete this;
    }
};

extent_t::extent_t(extent_manager_t *_em, file_t *_file)
    : amount_filled(0), em(_em),
      file(_file), last_block(nullptr), current_block(nullptr) {
    extent_ref = em->gen_extent();
    ++em->stats->pm_serializer_lba_extents;
}

extent_t::extent_t(extent_manager_t *_em, file_t *_file, int64_t loc, size_t size)
    : amount_filled(size), em(_em), file(_file), last_block(nullptr), current_block(nullptr)
{
    extent_ref = em->reserve_extent(loc);

    rassert(divides(DEVICE_BLOCK_SIZE, amount_filled));
    ++em->stats->pm_serializer_lba_extents;
}

void extent_t::destroy(extent_transaction_t *txn) {
    em->release_extent_into_transaction(std::move(extent_ref), txn);
    delete this;
}

void extent_t::shutdown() {
    UNUSED int64_t extent = extent_ref.release();
    delete this;
}

extent_t::~extent_t() {
    rassert(!current_block);
    if (last_block) last_block->is_last_block = false;
    --em->stats->pm_serializer_lba_extents;
}

void extent_t::read(size_t pos, size_t length, void *buffer, read_callback_t *cb) {
    rassert(!last_block);
    file->read_async(extent_ref.offset() + pos, length, buffer, DEFAULT_DISK_ACCOUNT, cb);

    // Ideally we would count these stats when the io operation completes,
    // but this is more generic than doing it in each callback
    em->stats->bytes_read(length);
}

void extent_t::append(void *buffer, size_t length, file_account_t *io_account) {
    rassert(amount_filled + length <= em->extent_size);

    while (length > 0) {
        size_t room_in_block;
        if (amount_filled % DEVICE_BLOCK_SIZE == 0) {
            rassert(!current_block);
            current_block = new extent_block_t(this, amount_filled);
            room_in_block = DEVICE_BLOCK_SIZE;
        } else {
            rassert(current_block);
            room_in_block = DEVICE_BLOCK_SIZE - amount_filled % DEVICE_BLOCK_SIZE;
        }

        size_t chunk = std::min(length, room_in_block);
        memcpy(current_block->data.get() + (amount_filled % DEVICE_BLOCK_SIZE), buffer, chunk);
        amount_filled += chunk;

        if (amount_filled % DEVICE_BLOCK_SIZE == 0) {
            extent_block_t *b = current_block;
            current_block = nullptr;
            b->write(io_account);
            em->stats->bytes_written(DEVICE_BLOCK_SIZE);
        }

        length -= chunk;
        buffer = reinterpret_cast<char *>(buffer) + chunk;
    }
}

void extent_t::sync(sync_callback_t *cb) {
    rassert(divides(DEVICE_BLOCK_SIZE, amount_filled));
    rassert(!current_block);
    if (last_block) {
        last_block->sync_cbs.push_back(cb);
    } else {
        cb->on_extent_sync();
    }
}

