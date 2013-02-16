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

patch_disk_storage_t::patch_disk_storage_t(UNUSED mc_cache_t *_cache, UNUSED block_id_t start_id) { }

patch_disk_storage_t::~patch_disk_storage_t() { }

// Loads on-disk data into memory
void patch_disk_storage_t::load_patches(UNUSED patch_memory_storage_t *in_memory_storage) {
}

// Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
// This function never blocks and must only be called while the flush_lock is held.
bool patch_disk_storage_t::store_patch(buf_patch_t *patch, const block_sequence_id_t current_block_block_sequence_id) {
    rassert(patch->get_block_sequence_id() == NULL_BLOCK_SEQUENCE_ID);
    patch->set_block_sequence_id(current_block_block_sequence_id);

    return false;
}

void patch_disk_storage_t::compress_n_oldest_blocks(UNUSED unsigned int n) {
}

unsigned int patch_disk_storage_t::get_number_of_log_blocks() const {
    return (unsigned int)number_of_blocks;
}
