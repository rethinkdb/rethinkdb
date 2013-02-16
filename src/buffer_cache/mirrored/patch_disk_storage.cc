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
