// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/log/lba/extent.hpp"

#include <vector>

#include "arch/arch.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/stats.hpp"

struct extent_block_t :
    public extent_t::sync_callback_t,
    public iocallback_t
{
    char *data;
    extent_t *parent;
    size_t offset;
    std::vector< extent_t::sync_callback_t* > sync_cbs;
    bool waiting_for_prev, have_finished_sync, is_last_block;

    extent_block_t(extent_t *_parent, size_t _offset)
        : parent(_parent), offset(_offset) {
        data = reinterpret_cast<char *>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
    }
    ~extent_block_t() {
        free(data);
    }

    void write(file_account_t *io_account) {
        waiting_for_prev = true;
        have_finished_sync = false;

        parent->sync(this);

        if (parent->last_block) parent->last_block->is_last_block = false;
        parent->last_block = this;
        is_last_block = true;

        parent->file->write_async(parent->offset + offset, DEVICE_BLOCK_SIZE, data, io_account, this);
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
            parent->last_block = NULL;
        }
        delete this;
    }
};

extent_t::extent_t(extent_manager_t *_em, direct_file_t *_file)
    : offset(_em->gen_extent()), amount_filled(0), em(_em),
      file(_file), last_block(NULL), current_block(NULL) {
    ++em->stats->pm_serializer_lba_extents;
}

extent_t::extent_t(extent_manager_t *_em, direct_file_t *_file, off64_t loc, size_t size)
    : offset(loc), amount_filled(size), em(_em), file(_file), last_block(NULL), current_block(NULL)
{
    em->reserve_extent(offset);

    rassert(divides(DEVICE_BLOCK_SIZE, amount_filled));
    ++em->stats->pm_serializer_lba_extents;
}

void extent_t::destroy(extent_transaction_t *txn) {
    em->release_extent(offset, txn);
    shutdown();
}

void extent_t::shutdown() {
    delete this;
}

extent_t::~extent_t() {
    rassert(!current_block);
    if (last_block) last_block->is_last_block = false;
    --em->stats->pm_serializer_lba_extents;
}

void extent_t::read(size_t pos, size_t length, void *buffer, read_callback_t *cb) {
    rassert(!last_block);
    file->read_async(offset + pos, length, buffer, DEFAULT_DISK_ACCOUNT, cb);
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
        memcpy(current_block->data + (amount_filled % DEVICE_BLOCK_SIZE), buffer, chunk);
        amount_filled += chunk;

        if (amount_filled % DEVICE_BLOCK_SIZE == 0) {
            extent_block_t *b = current_block;
            current_block = NULL;
            b->write(io_account);
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

