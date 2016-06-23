// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/extent_manager.hpp"

#include <queue>

#include "arch/arch.hpp"
#include "logger.hpp"
#include "math.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/log_serializer.hpp"

struct extent_info_t {
public:
    enum state_t {
        state_unreserved,
        state_in_use,
        state_free
    };
private:
    state_t state_;

public:
    void set_state(state_t new_state) {
        guarantee(state_ != state_in_use || extent_use_refcount == 0);
        state_ = new_state;
    }

    state_t state() const { return state_; }

    // Valid and non-zero when state_in_use.  There are two ways to own a part of
    // this refcount.  One is if you believe you're currently "using" the extent (if
    // you're the LBA or data_block_manager_t).  The other is (at the time of
    // writing) for every (extent_transaction_t, block_id) for live extent
    // transactions that have set an i_array entry to zero (in the data block
    // manager) but have not yet been commmitted.  The data_block_manager_t and LBA
    // ownership of the refcount also can get passed into the extent_transaction_t
    // object.
    intptr_t extent_use_refcount;

    extent_info_t() : state_(state_unreserved),
                      extent_use_refcount(0) { }
};

class extent_zone_t {
    const uint64_t extent_size;

    size_t offset_to_id(int64_t extent) const {
        rassert(divides(extent_size, extent));
        return extent / extent_size;
    }

    /* free-list and extent map. Contains one entry per extent.  During the
    state_reserving_extents phase, each extent has state state_unreserved or
    state state_in_use. When we transition to the state_running phase, we link
    all of the state_unreserved entries in each zone together into extent free
    queue.  The free queue can contain entries that point past the end of
    extents, if the file size ever gets shrunk. */

    std::vector<extent_info_t> extents;

    // We want to remove the minimum element from the free_queue first, leaving
    // free extents at the end of the file.
    std::priority_queue<size_t,
                        std::vector<size_t>,
                        std::greater<size_t> > free_queue;

    file_t *const dbfile;

    // The number of free extents in the file.
    size_t held_extents_;

public:
    size_t held_extents() const {
        return held_extents_;
    }

    extent_zone_t(file_t *_dbfile, uint64_t _extent_size)
        : extent_size(_extent_size), dbfile(_dbfile), held_extents_(0) {
        // (Avoid a bunch of reallocations by resize calls (avoiding O(n log n)
        // work on average).)
        extents.reserve(dbfile->get_file_size() / extent_size);
    }

    extent_reference_t reserve_extent(int64_t extent) {
        size_t id = offset_to_id(extent);

        if (id >= extents.size()) {
            extents.resize(id + 1);
        }

        rassert(extents[id].state() == extent_info_t::state_unreserved,
                "current state: %d (%s)",
                extents[id].state(),
                extents[id].state() == extent_info_t::state_in_use ? "in_use"
                  : extents[id].state() == extent_info_t::state_free ? "free"
                  : "unknown");
        extents[id].set_state(extent_info_t::state_in_use);
        return make_extent_reference(extent);
    }

    void reconstruct_free_list() {
        for (size_t extent_id = 0; extent_id < extents.size(); ++extent_id) {
            if (extents[extent_id].state() == extent_info_t::state_unreserved) {
                extents[extent_id].set_state(extent_info_t::state_free);
                free_queue.push(extent_id);
                ++held_extents_;
            }
        }
    }

    extent_reference_t gen_extent() {
        int64_t extent;

        if (free_queue.empty()) {
            rassert(held_extents_ == 0);
            extent = extents.size() * extent_size;
            extents.push_back(extent_info_t());
        } else if (free_queue.top() >= extents.size()) {
            rassert(held_extents_ == 0);
            std::priority_queue<size_t,
                                std::vector<size_t>,
                                std::greater<size_t> > tmp;
            free_queue = tmp;
            extent = extents.size() * extent_size;
            extents.push_back(extent_info_t());
        } else {
            extent = free_queue.top() * extent_size;
            free_queue.pop();
            --held_extents_;
        }

        extent_info_t *info = &extents[offset_to_id(extent)];
        info->set_state(extent_info_t::state_in_use);

        extent_reference_t extent_ref = make_extent_reference(extent);

        dbfile->set_file_size_at_least(extent + extent_size);

        return extent_ref;
    }

    extent_reference_t make_extent_reference(const int64_t extent) {
        size_t id = offset_to_id(extent);
        guarantee(id < extents.size());
        extent_info_t *info = &extents[id];
        guarantee(info->state() == extent_info_t::state_in_use);
        ++info->extent_use_refcount;
        return extent_reference_t(extent);
    }

    void try_shrink_file() {
        // Now potentially shrink the file.
        bool shrink_file = false;
        while (!extents.empty() && extents.back().state() == extent_info_t::state_free) {
            shrink_file = true;
            --held_extents_;
            extents.pop_back();
        }

        if (shrink_file) {
            dbfile->set_file_size(extents.size() * extent_size);

            // Prevent the existence of a relatively large free queue after the file
            // size shrinks.
            if (held_extents_ < free_queue.size() / 2) {
                std::priority_queue<size_t,
                                    std::vector<size_t>,
                                    std::greater<size_t> > tmp;
                for (size_t i = 0; i < held_extents_; ++i) {
                    tmp.push(free_queue.top());
                    free_queue.pop();
                }

                // held_extents_ was and will be the number of entries in the free_queue
                // that _didn't_ point off the end of the file.  We just moved those
                // entries to tmp.  The remaining entries must therefore point off the
                // end of the file.  Check that no remaining entries point within the
                // file.
                guarantee(free_queue.top() >= extents.size(), "Tried to discard valid held extents.");
                free_queue = std::move(tmp);
            }
        }
    }

    void release_extent(extent_reference_t &&extent_ref) {
        int64_t extent = extent_ref.release();
        extent_info_t *info = &extents[offset_to_id(extent)];
        guarantee(info->state() == extent_info_t::state_in_use);
        guarantee(info->extent_use_refcount > 0);
        --info->extent_use_refcount;
        if (info->extent_use_refcount == 0) {
            info->set_state(extent_info_t::state_free);
            free_queue.push(offset_to_id(extent));
            ++held_extents_;
            try_shrink_file();
        }
    }
};

extent_manager_t::extent_manager_t(file_t *file,
                                   const log_serializer_on_disk_static_config_t *static_config,
                                   log_serializer_stats_t *_stats)
    : stats(_stats), extent_size(static_config->extent_size()),
      state(state_reserving_extents) {
    guarantee(divides(DEVICE_BLOCK_SIZE, extent_size));

    zone.init(new extent_zone_t(file, extent_size));
}

extent_manager_t::~extent_manager_t() {
    rassert(state == state_reserving_extents || state == state_shut_down);
}

extent_reference_t extent_manager_t::reserve_extent(int64_t extent) {
    assert_thread();
    rassert(state == state_reserving_extents);
    ++stats->pm_extents_in_use;
    stats->pm_bytes_in_use += extent_size;
    return zone->reserve_extent(extent);
}

void extent_manager_t::prepare_initial_metablock(metablock_mixin_t *mb) {
    mb->padding = 0;
}

void extent_manager_t::start_existing(UNUSED metablock_mixin_t *last_metablock) {
    assert_thread();
    rassert(state == state_reserving_extents);
    current_transaction = nullptr;
    zone->reconstruct_free_list();
    state = state_running;

}

void extent_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    assert_thread();
    rassert(state == state_running);
    metablock->padding = 0;
}

void extent_manager_t::shutdown() {
    assert_thread();
    rassert(state == state_running);
    rassert(!current_transaction);
    state = state_shut_down;
}

void extent_manager_t::begin_transaction(extent_transaction_t *out) {
    assert_thread();
    rassert(!current_transaction);
    current_transaction = out;
    out->init();
}

extent_reference_t extent_manager_t::gen_extent() {
    assert_thread();
    rassert(state == state_running);
    ++stats->pm_extents_in_use;
    stats->pm_bytes_in_use += extent_size;

    return zone->gen_extent();
}

extent_reference_t
extent_manager_t::copy_extent_reference(const extent_reference_t &extent_ref) {
    int64_t offset = extent_ref.offset();
    return zone->make_extent_reference(offset);
}

void extent_manager_t::release_extent_into_transaction(extent_reference_t &&extent_ref, extent_transaction_t *txn) {
    release_extent_preliminaries();
    rassert(current_transaction);
    txn->push_extent(std::move(extent_ref));
}

void extent_manager_t::release_extent(extent_reference_t &&extent_ref) {
    release_extent_preliminaries();
    zone->release_extent(std::move(extent_ref));
}

void extent_manager_t::release_extent_preliminaries() {
    assert_thread();
    rassert(state == state_running);
    --stats->pm_extents_in_use;
    stats->pm_bytes_in_use -= extent_size;
}



void extent_manager_t::end_transaction(extent_transaction_t *t) {
    assert_thread();
    rassert(current_transaction == t);
    current_transaction = nullptr;
    t->mark_end();
}

void extent_manager_t::commit_transaction(extent_transaction_t *t) {
    assert_thread();
    std::vector<extent_reference_t> extents = t->reset();
    for (auto it = extents.begin(); it != extents.end(); ++it) {
        zone->release_extent(std::move(*it));
    }
}

size_t extent_manager_t::held_extents() {
    assert_thread();
    return zone->held_extents();
}
