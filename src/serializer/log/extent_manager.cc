// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/log/extent_manager.hpp"

#include "arch/arch.hpp"
#include "logger.hpp"
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
    int32_t extent_use_refcount;

    int64_t next_in_free_list;   // Valid if state == state_free

    extent_info_t() : state_(state_unreserved),
                      extent_use_refcount(0),
                      next_in_free_list(-1) {}
};

class extent_zone_t {
    const size_t extent_size;

    unsigned int offset_to_id(int64_t extent) {
        rassert(divides(extent_size, extent));
        return extent / extent_size;
    }

    /* Combination free-list and extent map. Contains one entry per extent.
    During the state_reserving_extents phase, each extent has state state_unreserved
    or state state_in_use. When we transition to the state_running phase,
    we link all of the state_unreserved entries in each zone together into an
    extent free list, such that each free entry's 'next_in_free_list' field is the
    offset of the next free extent. */

    segmented_vector_t<extent_info_t, MAX_DATA_EXTENTS> extents;

    int64_t free_list_head;
private:
    int held_extents_;
public:
    int held_extents() const {
        return held_extents_;
    }

public:
    extent_zone_t(size_t _extent_size)
        : extent_size(_extent_size), held_extents_(0) { }

    extent_reference_t reserve_extent(int64_t extent) {
        unsigned int id = offset_to_id(extent);

        if (id >= extents.get_size()) {
            extent_info_t default_info;
            extents.set_size(id + 1, default_info);
        }

        rassert(extents[id].state() == extent_info_t::state_unreserved);
        extents[id].set_state(extent_info_t::state_in_use);
        return make_extent_reference(extent);
    }

    void reconstruct_free_list() {
        free_list_head = NULL_OFFSET;

        for (int64_t extent = 0;
             extent < static_cast<int64_t>(extents.get_size() * extent_size);
             extent += extent_size) {
            if (extents[offset_to_id(extent)].state() == extent_info_t::state_unreserved) {
                extents[offset_to_id(extent)].set_state(extent_info_t::state_free);
                extents[offset_to_id(extent)].next_in_free_list = free_list_head;
                free_list_head = extent;
                held_extents_++;
            }
        }
    }

    extent_reference_t gen_extent() {
        int64_t extent;

        if (free_list_head == NULL_OFFSET) {
            extent = extents.get_size() * extent_size;

            extents.set_size(extents.get_size() + 1);
        } else {
            extent = free_list_head;
            free_list_head = extents[offset_to_id(free_list_head)].next_in_free_list;
            held_extents_--;
        }

        extent_info_t *info = &extents[offset_to_id(extent)];
        info->set_state(extent_info_t::state_in_use);

        return make_extent_reference(extent);
    }

    extent_reference_t make_extent_reference(int64_t extent) {
        unsigned int id = offset_to_id(extent);
        guarantee(id < extents.get_size());
        extent_info_t *info = &extents[id];
        guarantee(info->state() == extent_info_t::state_in_use);
        ++info->extent_use_refcount;
        return extent_reference_t(extent);
    }

    void release_extent(extent_reference_t &&extent_ref) {
        int64_t extent = extent_ref.release();
        extent_info_t *info = &extents[offset_to_id(extent)];
        guarantee(info->state() == extent_info_t::state_in_use);
        guarantee(info->extent_use_refcount > 0);
        --info->extent_use_refcount;
        if (info->extent_use_refcount == 0) {
            info->set_state(extent_info_t::state_free);
            info->next_in_free_list = free_list_head;
            free_list_head = extent;
            held_extents_++;
        }
    }
};

extent_manager_t::extent_manager_t(file_t *file,
                                   const log_serializer_on_disk_static_config_t *static_config,
                                   log_serializer_stats_t *_stats)
    : stats(_stats), extent_size(static_config->extent_size()),
      dbfile(file), state(state_reserving_extents) {
    guarantee(divides(DEVICE_BLOCK_SIZE, extent_size));

    zone.init(new extent_zone_t(extent_size));
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
    current_transaction = NULL;
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

    extent_reference_t extent_ref = zone->gen_extent();

    /* In case we are not on a block device */
    dbfile->set_size_at_least(extent_ref.offset() + extent_size);

    return extent_ref;
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
    current_transaction = NULL;
    t->mark_end();
}

void extent_manager_t::commit_transaction(extent_transaction_t *t) {
    assert_thread();
    std::vector<extent_reference_t> extents = t->reset();
    for (auto it = extents.begin(); it != extents.end(); ++it) {
        zone->release_extent(std::move(*it));
    }
}

int extent_manager_t::held_extents() {
    assert_thread();
    return zone->held_extents();
}
