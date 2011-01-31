#include "buffer_cache/mirrored/diff_out_of_core_storage.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/buffer_cache.hpp"

diff_oocore_storage_t::diff_oocore_storage_t(mc_cache_t &cache) : cache(cache) {
    first_block = 0;
    number_of_blocks = 0;

    active_log_block = 0;
    next_patch_offset = 0;
}

void diff_oocore_storage_t::init(const block_id_t first_block, const block_id_t number_of_blocks) {
    cache.assert_thread();
    this->first_block = first_block;
    this->number_of_blocks = number_of_blocks;

    if (number_of_blocks == 0)
        return;

    // TODO! Initialize the set of log blocks
    // TODO! (no transaction required) acquire all blocks, if new: zero and write magic. If existing: check magic

    set_active_log_block(first_block);
}

// Loads on-disk data into memory
void diff_oocore_storage_t::load_patches(diff_core_storage_t &in_core_storage) {
    cache.assert_thread();
    if (number_of_blocks == 0)
        return;

    std::map<block_id_t, std::list<buf_patch_t*> > patch_map;

    // Scan through all log blocks, build a map block_id -> patch list
    for (block_id_t current_block = first_block; current_block < first_block + number_of_blocks; ++current_block) {
        mc_buf_t *log_buf = acquire_block_no_locking(current_block);
        void *buf_data = log_buf->get_data_major_write();
        guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
        uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
        while (current_offset < cache.serializer->get_block_size().value()) {
            buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
            if (!patch) {
                break;
            }
            else {
                current_offset += patch->get_serialized_size();
                // Only store the patch if the corresponding block still exists
                // (otherwise we'd get problems when flushing the log, as deleted blocks would magically reappear from the dead or at least cause an error)
                if (cache.serializer->block_in_use(patch->get_block_id()))
                    patch_map[patch->get_block_id()].push_back(patch);
                else
                    delete patch;
            }
        }
        log_buf->release();
        delete log_buf;
    }

    for (std::map<block_id_t, std::list<buf_patch_t*> >::iterator patch_list = patch_map.begin(); patch_list != patch_map.end(); ++patch_list) {
        // Sort the list to get patches in the right order
        patch_list->second.sort(dereferencing_compare_t<buf_patch_t>());

        // Store list into in_core_storage
        in_core_storage.load_block_patch_list(patch_list->first, patch_list->second);
    }
}

// Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
// This function never blocks and must only be called while the flush_lock is held.
bool diff_oocore_storage_t::store_patch(buf_patch_t &patch) {
    cache.assert_thread();
    // TODO! assert flush_in_progress?

    if (number_of_blocks == 0)
        return false;

    // Check if we have sufficient free space in the current log block to store the patch
    const size_t patch_serialized_size = patch.get_serialized_size();
    rassert(cache.serializer->get_block_size().value() >= (size_t)next_patch_offset);
    size_t free_space = cache.serializer->get_block_size().value() - (size_t)next_patch_offset;
    if (patch_serialized_size > free_space) {
        // Try reclaiming some space (this usually switches to another log block)
        reclaim_space(patch_serialized_size);
        free_space = cache.serializer->get_block_size().value() - (size_t)next_patch_offset;

        // Check if enough space could be reclaimed
        if (patch_serialized_size > free_space)
            return false; // No success :-(
    }

    // Serialize patch at next_patch_offset, increase offset
    mc_buf_t *log_buf = acquire_block_no_locking(active_log_block);
    rassert(log_buf);

    void *buf_data = log_buf->get_data_major_write();
    patch.serialize((char*)buf_data + next_patch_offset);
    next_patch_offset += patch.get_serialized_size();

    log_buf->release();
    delete log_buf;
    
    return true;
}

// This function might block while it acquires old blocks from disk.
void diff_oocore_storage_t::flush_n_oldest_blocks(unsigned int n) {
    // TODO! assert flush_in_progress
    cache.assert_thread();

    if (number_of_blocks == 0)
        return;

    n = std::min(number_of_blocks, n);

    // Flush the n oldest blocks
    for (block_id_t i = 0; i < n; ++i) {
        block_id_t current_block = active_log_block + i;
        if (current_block >= first_block + number_of_blocks)
            current_block -= number_of_blocks;
        flush_block(current_block);
    }

    // If we affected the active block, we have to reset next_patch_offset
    if (n == number_of_blocks)
        set_active_log_block(active_log_block);
}

void diff_oocore_storage_t::reclaim_space(const size_t space_required) {
    block_id_t compress_block_id = select_log_block_for_compression();
    compress_block(compress_block_id);
    set_active_log_block(compress_block_id);
}

block_id_t diff_oocore_storage_t::select_log_block_for_compression() {
    block_id_t result = active_log_block + 1;
    if (result >= first_block + number_of_blocks)
        result -= number_of_blocks;
    return result;
}

void diff_oocore_storage_t::compress_block(const block_id_t log_block_id) {
    cache.assert_thread();

    std::vector<buf_patch_t*> live_patches;
    live_patches.reserve(cache.serializer->get_block_size().value() / 50);

    // Scan over the block and save patches that we want to preserve
    mc_buf_t *log_buf = acquire_block_no_locking(log_block_id);
    void *buf_data = log_buf->get_data_major_write();
    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
    while (current_offset < cache.serializer->get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
        if (!patch) {
            break;
        }
        else {
            current_offset += patch->get_serialized_size();

            // We want to preserve this patch iff it is >= the oldest patch that we have in the in-core storage
            // TODO!
            if (true)
                live_patches.push_back(patch);
            else
                delete patch;
        }
    }
    log_buf->release();
    delete log_buf;

    // Wipe the log block
    init_log_block(log_block_id);

    // Write back live patches
    log_buf = acquire_block_no_locking(log_block_id);
    rassert(log_buf);
    buf_data = log_buf->get_data_major_write();

    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    current_offset = sizeof(LOG_BLOCK_MAGIC);
    for (std::vector<buf_patch_t*>::const_iterator patch = live_patches.begin(); patch != live_patches.end(); ++patch) {
        (*patch)->serialize((char*)buf_data + current_offset);
        current_offset += (*patch)->get_serialized_size();
    }

    log_buf->release();
    delete log_buf;
}

void diff_oocore_storage_t::flush_block(const block_id_t log_block_id) {
    cache.assert_thread();

    // Scan over the block
    mc_buf_t *log_buf = acquire_block_no_locking(log_block_id);
    void *buf_data = log_buf->get_data_major_write();
    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
    while (current_offset < cache.serializer->get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
        if (!patch) {
            break;
        }
        else {
            current_offset += patch->get_serialized_size();

            // For each patch, acquire the affected block and call ensure_flush()
            // We have to do this only if there is any potentially applicable patch in the in-core storage...
            // (Note: we rely on the fact that deleted blocks never show up in the in-core diff storage)
            // TODO!
            if (true) {
                // We never have to lock the buffer, as we neither really read nor write any data
                // We just have to make sure that the buffer cache loads the block into memory
                // and then make writeback write it back in the next flush
                mc_buf_t *data_buf = acquire_block_no_locking(patch->get_block_id());
                // TODO! Check in-core storage again, now that the block has been acquired (old patches might have been evicted from it by doing so)
                if (true)
                    data_buf->ensure_flush();

                data_buf->release();
                delete data_buf;
            }

            delete patch;
        }
    }
    log_buf->release();
    delete log_buf;

    // Wipe the log block
    init_log_block(log_block_id);
}

void diff_oocore_storage_t::set_active_log_block(const block_id_t log_block_id) {
    rassert (log_block_id >= first_block && log_block_id < first_block + number_of_blocks);
    active_log_block = log_block_id;

    // Scan through the block to determine next_patch_offset
    mc_buf_t *log_buf = acquire_block_no_locking(active_log_block);
    void* buf_data = log_buf->get_data_major_write();
    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
    while (current_offset < cache.serializer->get_block_size().value()) {
        buf_patch_t *tmp_patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
        if (!tmp_patch) {
            break;
        }
        else {
            current_offset += tmp_patch->get_serialized_size();
            delete tmp_patch;
        }
    }
    log_buf->release();
    delete log_buf;
    
    next_patch_offset = current_offset;
}

void diff_oocore_storage_t::init_log_block(const block_id_t log_block_id) {
    // TODO! Place magic, zero remaining data
}

// Just the same as in buffer_cache/co_functions.cc (TODO: Refactor)
struct co_block_available_callback_t : public mc_block_available_callback_t {
    coro_t *self;
    mc_buf_t *value;

    virtual void on_block_available(mc_buf_t *block) {
        value = block;
        self->notify();
    }

    mc_buf_t *join() {
        self = coro_t::self();
        coro_t::wait();
        return value;
    }
};

mc_buf_t* diff_oocore_storage_t::acquire_block_no_locking(const block_id_t log_block_id) {
    cache.assert_thread();

    mc_inner_buf_t *inner_buf = cache.page_map.find(log_block_id);
    if (!inner_buf) {
        /* The buf isn't in the cache and must be loaded from disk */
        inner_buf = new mc_inner_buf_t(&cache, log_block_id);
    }

    mc_buf_t *buf = new mc_buf_t(inner_buf, rwi_write, true);

    if (buf->ready) {
        return buf;
    } else {
        co_block_available_callback_t cb;
        buf->callback = &cb;
        return cb.join();
    }
}
