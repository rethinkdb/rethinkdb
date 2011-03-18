#include "buffer_cache/mirrored/patch_disk_storage.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/buffer_cache.hpp"

const block_magic_t mc_config_block_t::expected_magic = { { 'm','c','f','g' } };

void patch_disk_storage_t::create(translator_serializer_t *serializer, block_id_t start_id, mirrored_cache_static_config_t *config) {

    /* Prepare the config block */
    mc_config_block_t *c = reinterpret_cast<mc_config_block_t *>(serializer->malloc());
    bzero((void*)c, serializer->get_block_size().value());
    c->magic = mc_config_block_t::expected_magic;
    c->cache = *config;

    translator_serializer_t::write_t write = translator_serializer_t::write_t::make(
        start_id,
        repli_timestamp::invalid,
        (void*)c,
        false,
        NULL);

    /* Write it to the serializer */
    on_thread_t switcher(serializer->home_thread);

    struct : public serializer_t::write_txn_callback_t, public cond_t {
        void on_serializer_write_txn() { pulse(); }
    } cb;
    if (!serializer->do_write(&write, 1, &cb)) cb.wait();

    serializer->free((void*)c);
}

patch_disk_storage_t::patch_disk_storage_t(mc_cache_t &cache, block_id_t start_id) :
    cache(cache), first_block(start_id + 1)
{
    active_log_block = 0;
    next_patch_offset = 0;

    // Read the existing config block
    mc_config_block_t *config_block = reinterpret_cast<mc_config_block_t *>(cache.serializer->malloc());
    {
        on_thread_t switcher(cache.serializer->home_thread);
        struct : public serializer_t::read_callback_t, public cond_t {
            void on_serializer_read() { pulse(); }
        } cb;
        if (!cache.serializer->do_read(start_id, (void*)config_block, &cb)) cb.wait();
    }
    guarantee(check_magic<mc_config_block_t>(config_block->magic), "Invalid mirrored cache config block magic");
    number_of_blocks = config_block->cache.n_patch_log_blocks;
    cache.serializer->free((void*)config_block);
    
    if ((unsigned long long)number_of_blocks > (unsigned long long)cache.dynamic_config->max_size / cache.get_block_size().ser_value()) {
        fail_due_to_user_error("The cache of size %d blocks is too small to hold this database's diff log of %d blocks.",
            (int)(cache.dynamic_config->max_size / cache.get_block_size().ser_value()),
            (int)(number_of_blocks));
    }

    block_is_empty.resize(number_of_blocks, false);

    if (number_of_blocks == 0)
        return;

    // Preload blocks into memory
    {
        on_thread_t switcher(cache.serializer->home_thread);
        for (block_id_t current_block = first_block; current_block < first_block + number_of_blocks; ++current_block) {
            if (cache.serializer->block_in_use(current_block)) {
                coro_t::spawn(boost::bind(&patch_disk_storage_t::preload_block, this, current_block));
            } else {
                block_is_empty[current_block - first_block] = true;
            }
        }
    }

    // TODO: Somebody answer this question.
    coro_t::yield();   // Why?

    // Load all log blocks into memory
    for (block_id_t current_block = first_block; current_block < first_block + number_of_blocks; ++current_block) {
        
        if (block_is_empty[current_block - first_block]) {
            // Initialize a new log block here (we rely on the properties of block_id assignment)
            mc_inner_buf_t *new_ibuf = new mc_inner_buf_t(&cache, repli_timestamp::invalid);
            guarantee(new_ibuf->block_id == current_block);

            log_block_bufs.push_back(acquire_block_no_locking(current_block));

            init_log_block(current_block);

        } else {
            log_block_bufs.push_back(acquire_block_no_locking(current_block));
    
            // Check that this is a valid log block
            mc_buf_t *log_buf = log_block_bufs[current_block - first_block];
            void *buf_data = log_buf->get_data_major_write();
            guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
        }
    }
    rassert(log_block_bufs.size() == number_of_blocks);

    set_active_log_block(first_block);
}

patch_disk_storage_t::~patch_disk_storage_t() {
    for (size_t i = 0; i < log_block_bufs.size(); ++i)
        log_block_bufs[i]->release();
    log_block_bufs.clear();
}

// Loads on-disk data into memory
void patch_disk_storage_t::load_patches(patch_memory_storage_t &in_memory_storage) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache.assert_thread();
    if (number_of_blocks == 0)
        return;

    std::map<block_id_t, std::list<buf_patch_t*> > patch_map;

    // Scan through all log blocks, build a map block_id -> patch list
    for (block_id_t current_block = first_block; current_block < first_block + number_of_blocks; ++current_block) {
        mc_buf_t *log_buf = log_block_bufs[current_block - first_block];
        const void *buf_data = log_buf->get_data_read();
        guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
        uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
        while (current_offset + buf_patch_t::get_min_serialized_size() < cache.get_block_size().value()) {
            buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
            if (!patch) {
                break;
            }
            else {
                current_offset += patch->get_serialized_size();
                // Only store the patch if the corresponding block still exists
                // (otherwise we'd get problems when flushing the log, as deleted blocks would cause an error)
                coro_t::move_to_thread(cache.serializer->home_thread);
                bool block_in_use = cache.serializer->block_in_use(patch->get_block_id());
                coro_t::move_to_thread(cache.home_thread);
                if (block_in_use)
                    patch_map[patch->get_block_id()].push_back(patch);
                else
                    delete patch;
            }
        }
    }

    for (std::map<block_id_t, std::list<buf_patch_t*> >::iterator patch_list = patch_map.begin(); patch_list != patch_map.end(); ++patch_list) {
        // Sort the list to get patches in the right order
        patch_list->second.sort(dereferencing_compare_t<buf_patch_t>());

        // Store list into in_core_storage
        in_memory_storage.load_block_patch_list(patch_list->first, patch_list->second);
    }
}

// Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
// This function never blocks and must only be called while the flush_lock is held.
bool patch_disk_storage_t::store_patch(buf_patch_t &patch, const ser_transaction_id_t current_block_transaction_id) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache.assert_thread();
    rassert(patch.get_transaction_id() == NULL_SER_TRANSACTION_ID);
    patch.set_transaction_id(current_block_transaction_id);

    if (number_of_blocks == 0)
        return false;

    // Check if we have sufficient free space in the current log block to store the patch
    const size_t patch_serialized_size = patch.get_serialized_size();
    rassert(cache.get_block_size().value() >= (size_t)next_patch_offset);
    size_t free_space = cache.get_block_size().value() - (size_t)next_patch_offset;
    if (patch_serialized_size > free_space) {
        // Try reclaiming some space (this usually switches to another log block)
        const block_id_t initial_log_block = active_log_block;
        const uint16_t initial_next_patch_offset = next_patch_offset;

        reclaim_space(patch_serialized_size);

        free_space = cache.get_block_size().value() - (size_t)next_patch_offset;

        // Enforce a certain fraction of the block to be freed up. If that is not possible
        // we rather fail than continue trying to free up space for the following patches,
        // as the latter might make everything very inefficient.
        const size_t min_reclaimed_space = cache.get_block_size().value() / 3;

        // Check if enough space could be reclaimed
        if (std::max(patch_serialized_size, min_reclaimed_space) > free_space) {
            // No success :-(
            // We go back to the initial block to make sure that this one gets flushed
            // when flush_n_oldest_blocks is called next (as it is obviously full)...
            active_log_block = initial_log_block;
            next_patch_offset = initial_next_patch_offset;
            return false;
        }
    }

    // Serialize patch at next_patch_offset, increase offset
    mc_buf_t *log_buf = log_block_bufs[active_log_block - first_block];
    rassert(log_buf);
    block_is_empty[active_log_block - first_block] = false;

    void *buf_data = log_buf->get_data_major_write();
    patch.serialize((char*)buf_data + next_patch_offset);
    next_patch_offset += patch_serialized_size;

    return true;
}

// This function might block while it acquires old blocks from disk.
void patch_disk_storage_t::clear_n_oldest_blocks(unsigned int n) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache.assert_thread();

    if (number_of_blocks == 0)
        return;

    n = std::min(number_of_blocks, n);

    waiting_for_clear = 0;
    // Flush the n oldest blocks
    for (block_id_t i = 1; i <= n; ++i) {
        block_id_t current_block = active_log_block + i;
        if (current_block >= first_block + number_of_blocks)
            current_block -= number_of_blocks;

        if (!block_is_empty[current_block - first_block]) {
            ++waiting_for_clear;
            coro_t::spawn(boost::bind(&patch_disk_storage_t::clear_block, this, current_block, coro_t::self()));
        }
    }
    if (waiting_for_clear > 0)
        coro_t::wait();

    // If we affected the active block, we have to reset next_patch_offset
    if (n == number_of_blocks)
        set_active_log_block(active_log_block);
}

void patch_disk_storage_t::compress_n_oldest_blocks(unsigned int n) {
    rassert(log_block_bufs.size() == number_of_blocks);
    cache.assert_thread();

    if (number_of_blocks == 0)
        return;

    n = std::min(number_of_blocks, n);

    // Compress the n oldest blocks
    for (block_id_t i = 1; i <= n; ++i) {
        block_id_t current_block = active_log_block + i;
        if (current_block >= first_block + number_of_blocks)
            current_block -= number_of_blocks;

        if (!block_is_empty[current_block - first_block]) {
            compress_block(current_block);
        }
    }

    // If we affected the active block, we have to reset next_patch_offset
    if (n == number_of_blocks)
        set_active_log_block(active_log_block);
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
    cache.assert_thread();

    std::vector<buf_patch_t*> live_patches;
    live_patches.reserve(cache.get_block_size().value() / 30);

    // Scan over the block and save patches that we want to preserve
    mc_buf_t *log_buf = log_block_bufs[log_block_id - first_block];
    void *buf_data = log_buf->get_data_major_write();
    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
    bool log_block_changed = false;
    while (current_offset + buf_patch_t::get_min_serialized_size() < cache.get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
        if (!patch) {
            break;
        }
        else {
            current_offset += patch->get_serialized_size();

            // We want to preserve this patch iff it is >= the oldest patch that we have in the in-core storage
            const std::vector<buf_patch_t*>* patches = cache.patch_memory_storage.get_patches(patch->get_block_id());
            rassert(!patches || patches->size() > 0);
            if (patches && !(*patch < *patches->front()))
                live_patches.push_back(patch);
            else {
                delete patch;
                log_block_changed = true;
            }
        }
    }

    if (log_block_changed) {
        // Wipe the log block
        init_log_block(log_block_id);

        // Write back live patches
        rassert(log_buf);
        buf_data = log_buf->get_data_major_write();

        guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
        current_offset = sizeof(LOG_BLOCK_MAGIC);
        for (std::vector<buf_patch_t*>::iterator patch = live_patches.begin(); patch != live_patches.end(); ++patch) {
            (*patch)->serialize((char*)buf_data + current_offset);
            current_offset += (*patch)->get_serialized_size();
            delete *patch;
        }
    } else {
        for (std::vector<buf_patch_t*>::iterator patch = live_patches.begin(); patch != live_patches.end(); ++patch) {
            delete *patch;
        }
    }
}

void patch_disk_storage_t::preload_block(const block_id_t log_block_id) {
    // Acquire the block but release it immediately...
    coro_t::move_to_thread(cache.home_thread);
    acquire_block_no_locking(log_block_id)->release();
    coro_t::move_to_thread(cache.serializer->home_thread);
}

void patch_disk_storage_t::clear_block(const block_id_t log_block_id, coro_t* notify_coro) {
    cache.assert_thread();

    // Scan over the block
    mc_buf_t *log_buf = log_block_bufs[log_block_id - first_block];;
    const void *buf_data = log_buf->get_data_read();
    guarantee(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
    uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);
    while (current_offset + buf_patch_t::get_min_serialized_size() < cache.get_block_size().value()) {
        buf_patch_t *patch = buf_patch_t::load_patch((char*)buf_data + current_offset);
        if (!patch) {
            break;
        }
        else {
            current_offset += patch->get_serialized_size();

            // For each patch, acquire the affected block and call ensure_flush()
            // We have to do this only if there is any potentially applicable patch in the in-core storage...
            // (Note: we rely on the fact that deleted blocks never show up in the in-core diff storage)
            if (cache.patch_memory_storage.get_patches(patch->get_block_id())) {
                // We never have to lock the buffer, as we neither really read nor write any data
                // We just have to make sure that the buffer cache loads the block into memory
                // and then make writeback write it back in the next flush
                mc_buf_t *data_buf = acquire_block_no_locking(patch->get_block_id());
                // Check in-core storage again, now that the block has been acquired (old patches might have been evicted from it by doing so)
                if (cache.patch_memory_storage.get_patches(patch->get_block_id())) {
                    data_buf->ensure_flush();
                }

                data_buf->release();
            }

            delete patch;
        }
    }

    // Wipe the log block
    init_log_block(log_block_id);
    block_is_empty[log_block_id - first_block] = true;

    --waiting_for_clear;
    if (waiting_for_clear == 0)
        notify_coro->notify();
}

void patch_disk_storage_t::set_active_log_block(const block_id_t log_block_id) {
    rassert (log_block_id >= first_block && log_block_id < first_block + number_of_blocks);
    active_log_block = log_block_id;

    if (!block_is_empty[log_block_id - first_block]) {
        // Scan through the block to determine next_patch_offset
        mc_buf_t *log_buf = log_block_bufs[active_log_block - first_block];
        const void *buf_data = log_buf->get_data_read();
        rassert(strncmp((char*)buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC)) == 0);
        uint16_t current_offset = sizeof(LOG_BLOCK_MAGIC);

        while (current_offset + buf_patch_t::get_min_serialized_size() < cache.get_block_size().value()) {
            uint16_t length = *(uint16_t*)((char*)buf_data + current_offset);
            if (length == 0) {
                break;
            }
            else {
                current_offset += length;
            }
        }

        next_patch_offset = current_offset;
    }
    else {
        next_patch_offset = sizeof(LOG_BLOCK_MAGIC);
    }
}

void patch_disk_storage_t::init_log_block(const block_id_t log_block_id) {
    mc_buf_t *log_buf = log_block_bufs[log_block_id - first_block];;
    void *buf_data = log_buf->get_data_major_write();

    memcpy(buf_data, LOG_BLOCK_MAGIC, sizeof(LOG_BLOCK_MAGIC));
    bzero((char*)buf_data + sizeof(LOG_BLOCK_MAGIC), cache.serializer->get_block_size().value() - sizeof(LOG_BLOCK_MAGIC));
}

// Just the same as in buffer_cache/co_functions.cc (TODO: Refactor?)
struct co_block_available_callback_2_t : public mc_block_available_callback_t {
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

mc_buf_t* patch_disk_storage_t::acquire_block_no_locking(const block_id_t block_id) {
    cache.assert_thread();

    mc_inner_buf_t *inner_buf = cache.page_map.find(block_id);
    if (!inner_buf) {
        /* The buf isn't in the cache and must be loaded from disk */
        inner_buf = new mc_inner_buf_t(&cache, block_id, true);
    }
    
    // We still have to acquire the lock once to wait for the buf to get ready
    mc_buf_t *buf = new mc_buf_t(inner_buf, rwi_read);

    if (buf->ready) {
        // Release the lock we've got
        buf->inner_buf->lock.unlock();
        buf->non_locking_access = true;
        buf->mode = rwi_write;
        return buf;
    } else {
        co_block_available_callback_2_t cb;
        buf->callback = &cb;
        buf = cb.join();
        // Release the lock we've got
        buf->inner_buf->lock.unlock();
        buf->non_locking_access = true;
        buf->mode = rwi_write;
        return buf;
    }
}
