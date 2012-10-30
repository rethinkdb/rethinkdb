// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/page_repl_random.hpp"

#include "buffer_cache/mirrored/mirrored.hpp"
#include "logger.hpp"
#include "perfmon/perfmon.hpp"

evictable_t::evictable_t(mc_cache_t *_cache, bool loaded)
    : eviction_priority(DEFAULT_EVICTION_PRIORITY), cache(_cache), page_repl_index(static_cast<unsigned int>(-1))
{
    cache->assert_thread();
    if (loaded) {
        insert_into_page_repl();
    }
}

evictable_t::~evictable_t() {
    cache->assert_thread();

    // It's the subclass destructor's responsibility to run
    //
    //     if (in_page_repl()) { remove_from_page_repl(); }
    rassert(!in_page_repl());
}

bool evictable_t::in_page_repl() {
    return page_repl_index != (unsigned) -1;
}

void evictable_t::insert_into_page_repl() {
    cache->assert_thread();
    page_repl_index = cache->page_repl.array.size();
    cache->page_repl.array.set(page_repl_index, this);
}

void evictable_t::remove_from_page_repl() {
    cache->assert_thread();
    unsigned int last_index = cache->page_repl.array.size() - 1;

    if (page_repl_index == last_index) {
        cache->page_repl.array.set(page_repl_index, NULL);
    } else {
        evictable_t *replacement = cache->page_repl.array.get(last_index);
        replacement->page_repl_index = page_repl_index;
        cache->page_repl.array.set(page_repl_index, replacement);
        cache->page_repl.array.set(last_index, NULL);
    }
    page_repl_index = static_cast<unsigned int>(-1);
}

page_repl_random_t::page_repl_random_t(unsigned int _unload_threshold, cache_t *_cache)
    : unload_threshold(_unload_threshold),
      cache(_cache)
    {}

bool page_repl_random_t::is_full(unsigned int space_needed) {
    cache->assert_thread();
    return array.size() + space_needed > unload_threshold;
}

//perfmon_counter_t pm_n_blocks_evicted("blocks_evicted");

// make_space tries to make sure that the number of blocks currently in memory is at least
// 'space_needed' less than the user-specified memory limit.
void page_repl_random_t::make_space(unsigned int space_needed) {
    cache->assert_thread();
    unsigned int target;
    // TODO(rntz): why, if more space is needed than unload_threshold, do we set the target number
    // of pages in cache to unload_threshold rather than 0? (note: git blames this on tim)
    if (space_needed > unload_threshold) {
        target = unload_threshold;
    } else {
        target = unload_threshold - space_needed;
    }

    while (array.size() > target) {
        // Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
        // dirty or in use.
        evictable_t *block_to_unload = NULL;
        for (int tries = PAGE_REPL_NUM_TRIES; tries > 0; tries --) {
            /* Choose a block in memory at random. */
            // NOTE: this method of random selection is slightly biased towards lower indices, I
            // think. @rntz
            unsigned int n = random() % array.size();
            evictable_t *block = array.get(n);

            // TODO we don't have code that sets buf_snapshot_t eviction priorities.

            if (!block->safe_to_unload()) {
                /* nothing to do here, jetpack away to the next iteration of this loop */
            } else if (block_to_unload == NULL) {
                /* The block is safe to unload, and our only candidate so far, so he's in */
                block_to_unload = block;
            } else if (block_to_unload->eviction_priority < block->eviction_priority) {
                /* This block is a better candidate than one before, he's in */
                block_to_unload = block;
            } else {
                /* Failed to find a better candidate, continue on our way. */
            }
        }

        if (!block_to_unload) {
	    // The following log message blows the corostack because it has propensity to overlog.
	    // Commenting it out for 1.2. TODO: we might want to address it later in a different
            // way (i.e. spawn_maybe?)
	    /*
            if (array.size() > target + (target / 100) + 10)
                logWRN("cache %p exceeding memory target. %d blocks in memory, %d dirty, target is %d.",
                       cache, array.size(), cache->writeback.num_dirty_blocks(), target);
	    */
            break;
        }

        // Remove it from the page repl and call its callback. Need to remove it from the repl first
        // because its callback could delete it.
        block_to_unload->remove_from_page_repl();
        block_to_unload->unload();
        ++cache->stats->pm_n_blocks_evicted;
    }
}

evictable_t *page_repl_random_t::get_first_buf() {
    cache->assert_thread();
    if (array.size() == 0) return NULL;
    return array.get(0);
}
