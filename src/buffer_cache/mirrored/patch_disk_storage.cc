// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/patch_disk_storage.hpp"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <string.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "serializer/serializer.hpp"

const block_magic_t log_block_magic = { { 'l', 'o', 'g', 'b' } };

const block_magic_t mc_config_block_t::expected_magic = { { 'm', 'c', 'f', 'g' } };

void patch_disk_storage_t::create(serializer_t *serializer, block_id_t start_id, mirrored_cache_static_config_t *config) {

    /* Prepare the config block */
    mc_config_block_t *c = reinterpret_cast<mc_config_block_t *>(serializer->malloc());
    memset(c, 0, serializer->get_block_size().value());
    c->magic = mc_config_block_t::expected_magic;
    c->cache = *config;

    /* Write it to the serializer */
    on_thread_t switcher(serializer->home_thread());

    index_write_op_t op(start_id);
    op.token = serializer->block_write(c, start_id, DEFAULT_DISK_ACCOUNT);
    op.recency = repli_timestamp_t::invalid;
    serializer_index_write(serializer, op, DEFAULT_DISK_ACCOUNT);

    serializer->free(c);
}

patch_disk_storage_t::patch_disk_storage_t(mc_cache_t *_cache, block_id_t start_id) :
    cache(_cache), first_block(start_id + 1)
{
    active_log_block = 0;
    next_patch_offset = 0;

    // Read the existing config block & determine which blocks are alive
    {
        on_thread_t switcher(cache->serializer->home_thread());

        // Load and parse config block
        mc_config_block_t *config_block = reinterpret_cast<mc_config_block_t *>(cache->serializer->malloc());
        cache->serializer->block_read(cache->serializer->index_read(start_id), config_block, DEFAULT_DISK_ACCOUNT);
        guarantee(mc_config_block_t::expected_magic == config_block->magic, "Invalid mirrored cache config block magic");
        guarantee(config_block->cache.n_patch_log_blocks == 0, "n_patch_log_blocks is %" PRIi32, config_block->cache.n_patch_log_blocks);
        cache->serializer->free(config_block);
    }
}

patch_disk_storage_t::~patch_disk_storage_t() {
    for (size_t i = 0; i < log_block_bufs.size(); ++i)
        delete log_block_bufs[i];
    log_block_bufs.clear();
}

// Loads on-disk data into memory
void patch_disk_storage_t::load_patches(UNUSED patch_memory_storage_t *in_memory_storage) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache->assert_thread();
    if (number_of_blocks == 0)
        return;
}

// Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
// This function never blocks and must only be called while the flush_lock is held.
bool patch_disk_storage_t::store_patch(buf_patch_t *patch, const block_sequence_id_t current_block_block_sequence_id) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache->assert_thread();
    rassert(patch->get_block_sequence_id() == NULL_BLOCK_SEQUENCE_ID);
    patch->set_block_sequence_id(current_block_block_sequence_id);

    if (number_of_blocks == 0)
        return false;
}

// This function might block while it acquires old blocks from disk.
void patch_disk_storage_t::clear_n_oldest_blocks(UNUSED unsigned int n) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache->assert_thread();

    if (number_of_blocks == 0)
        return;
}

void patch_disk_storage_t::compress_n_oldest_blocks(UNUSED unsigned int n) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache->assert_thread();

    if (number_of_blocks == 0)
        return;
}

unsigned int patch_disk_storage_t::get_number_of_log_blocks() const {
    return (unsigned int)number_of_blocks;
}

// TODO: Why do we not use space_required?
void patch_disk_storage_t::reclaim_space(UNUSED const size_t space_required) {
    block_id_t compress_block_id = select_log_block_for_compression();
    if (!block_is_empty[compress_block_id - first_block])
        compress_block(compress_block_id);
    set_active_log_block(compress_block_id);
}

block_id_t patch_disk_storage_t::select_log_block_for_compression() {
    block_id_t result = active_log_block + 1;
    if (result >= first_block + number_of_blocks) {
        result -= number_of_blocks;
    }
    return result;
}

void patch_disk_storage_t::compress_block(const block_id_t log_block_id) {
    cache->assert_thread();

    std::vector<buf_patch_t*> live_patches;
    live_patches.reserve(cache->get_block_size().value() / 30);

    // Scan over the block and save patches that we want to preserve
    mc_buf_lock_t *log_buf = log_block_bufs[log_block_id - first_block];
    void *buf_data = log_buf->get_data_major_write();
    guarantee(*reinterpret_cast<const block_magic_t *>(buf_data) == log_block_magic);
    uint16_t current_offset = sizeof(log_block_magic);
    bool log_block_changed = false;
    while (current_offset + buf_patch_t::get_min_serialized_size() < cache->get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch(reinterpret_cast<char *>(buf_data) + current_offset);
        if (!patch) {
            break;
        }

        current_offset += patch->get_serialized_size();

        // We want to preserve this patch iff it is >= the oldest patch that we have in the in-core storage
        if (cache->patch_memory_storage.has_patches_for_block(patch->get_block_id())
            && !patch->applies_before(cache->patch_memory_storage.first_patch(patch->get_block_id()))) {
            live_patches.push_back(patch);
        } else {
            delete patch;
            log_block_changed = true;
        }
    }

    if (log_block_changed) {
        // Wipe the log block
        init_log_block(log_block_id);

        // Write back live patches
        rassert(log_buf);
        buf_data = log_buf->get_data_major_write();

        guarantee(*reinterpret_cast<const block_magic_t *>(buf_data) == log_block_magic);
        current_offset = sizeof(log_block_magic);
        for (std::vector<buf_patch_t*>::iterator patch = live_patches.begin(); patch != live_patches.end(); ++patch) {
            (*patch)->serialize(reinterpret_cast<char *>(buf_data) + current_offset);
            current_offset += (*patch)->get_serialized_size();
            delete *patch;
        }
    } else {
        for (std::vector<buf_patch_t*>::iterator patch = live_patches.begin(); patch != live_patches.end(); ++patch) {
            delete *patch;
        }
    }
}

void patch_disk_storage_t::clear_block(const block_id_t log_block_id, coro_t* notify_coro) {
    cache->assert_thread();

    // Scan over the block
    mc_buf_lock_t *log_buf = log_block_bufs[log_block_id - first_block];
    const void *buf_data = log_buf->get_data_read();

    guarantee(*reinterpret_cast<const block_magic_t *>(buf_data) == log_block_magic);
    uint16_t current_offset = sizeof(log_block_magic);
    while (current_offset + buf_patch_t::get_min_serialized_size() < cache->get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch(reinterpret_cast<const char *>(buf_data) + current_offset);
        if (!patch) {
            break;
        } else {
            current_offset += patch->get_serialized_size();

            // For each patch, acquire the affected block and call ensure_flush()
            // We have to do this only if there is any potentially applicable patch in the in-core storage...
            // (Note: we rely on the fact that deleted blocks never show up in the in-core diff storage)
            if (cache->patch_memory_storage.has_patches_for_block(patch->get_block_id())) {
                // We never have to lock the buffer, as we neither really read nor write any data
                // We just have to make sure that the buffer cache loads the block into memory
                // and then make writeback write it back in the next flush
                mc_buf_lock_t *data_buf = mc_buf_lock_t::acquire_non_locking_lock(cache, patch->get_block_id());
                // Check in-core storage again, now that the block has been acquired (old patches might have been evicted from it by doing so)
                if (cache->patch_memory_storage.has_patches_for_block(patch->get_block_id())) {
                    data_buf->ensure_flush();
                }

                delete data_buf;
            }

            delete patch;
        }
    }

    // Wipe the log block
    init_log_block(log_block_id);
    block_is_empty[log_block_id - first_block] = true;

    --waiting_for_clear;
    if (waiting_for_clear == 0) {
        notify_coro->notify_later_ordered();
    }
}

void patch_disk_storage_t::set_active_log_block(const block_id_t log_block_id) {
    rassert (log_block_id >= first_block && log_block_id < first_block + number_of_blocks);
    active_log_block = log_block_id;

    if (!block_is_empty[log_block_id - first_block]) {
        // Scan through the block to determine next_patch_offset
        mc_buf_lock_t *log_buf = log_block_bufs[active_log_block - first_block];
        const void *buf_data = log_buf->get_data_read();
        rassert(*reinterpret_cast<const block_magic_t *>(buf_data) == log_block_magic);
        uint16_t current_offset = sizeof(log_block_magic);

        while (current_offset + buf_patch_t::get_min_serialized_size() < cache->get_block_size().value()) {
            uint16_t length = *reinterpret_cast<const uint16_t *>(reinterpret_cast<const char *>(buf_data) + current_offset);
            if (length == 0) {
                break;
            } else {
                current_offset += length;
            }
        }

        next_patch_offset = current_offset;
    } else {
        next_patch_offset = sizeof(log_block_magic);
    }
}

void patch_disk_storage_t::init_log_block(const block_id_t log_block_id) {
    mc_buf_lock_t *log_buf = log_block_bufs[log_block_id - first_block];
    void *buf_data = log_buf->get_data_major_write();

    *reinterpret_cast<block_magic_t *>(buf_data) = log_block_magic;
    memset(reinterpret_cast<char *>(buf_data) + sizeof(log_block_magic), 0, cache->serializer->get_block_size().value() - sizeof(log_block_magic));
}

