// Copyright 2010-2012 RethinkDB, all rights reserved.
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

    // Valid and non-zero when state_in_use.  There are two ways to own a part of this refcount.
    // One is if you believe you're currently "using" the extent (if you're the LBA or
    // data_block_manager_t).  The other is (at the time of writing) for every
    // (extent_transaction_t, block_id) for live extent transactions that have set an i_array entry
    // to zero (in the data block manager) but have not yet been commmitted.  The
    // data_block_manager_t and LBA ownership of the refcount also can get passed into the
    // extent_transaction_t object.
    int32_t extent_use_refcount;

    off64_t next_in_free_list;   // Valid if state == state_free

    extent_info_t() : state_(state_unreserved), extent_use_refcount(0), next_in_free_list(-1) {}
};

class extent_zone_t {
    off64_t start, end;
    size_t extent_size;

    unsigned int offset_to_id(off64_t extent) {
        rassert(extent < end);
        rassert(extent >= start);
#ifndef NDEBUG
        bool extent_mod_extent_size_equals_zero = extent % extent_size == 0;
        rassert(extent_mod_extent_size_equals_zero);
#endif
        return (extent - start) / extent_size;
    }

    /* Combination free-list and extent map. Contains one entry per extent.
    During the state_reserving_extents phase, each extent has state state_unreserved
    or state state_in_use. When we transition to the state_running phase,
    we link all of the state_unreserved entries in each zone together into an
    extent free list, such that each free entry's 'next_in_free_list' field is the
    offset of the next free extent. */

    segmented_vector_t<extent_info_t, MAX_DATA_EXTENTS> extents;

    off64_t free_list_head;
private:
    int held_extents_;
public:
    int held_extents() const {
        return held_extents_;
    }

public:
    extent_zone_t(off64_t _start, off64_t _end, size_t _extent_size)
        : start(_start), end(_end), extent_size(_extent_size), held_extents_(0)
    {
#ifndef NDEBUG
        bool start_aligned = start % extent_size == 0;
        rassert(start_aligned);
        bool end_aligned = end % extent_size == 0;
        rassert(end_aligned);
#endif
    }

    void reserve_extent(off64_t extent, extent_reference_t *extent_ref_out) {
        unsigned int id = offset_to_id(extent);

        if (id >= extents.get_size()) {
            extent_info_t default_info;
            extents.set_size(id + 1, default_info);
        }

        rassert(extents[id].state() == extent_info_t::state_unreserved);
        extents[id].set_state(extent_info_t::state_in_use);
        make_extent_reference(extent, extent_ref_out);
    }

    void reconstruct_free_list() {
        free_list_head = NULL_OFFSET;

        for (off64_t extent = start; extent < start + (off64_t)(extents.get_size() * extent_size); extent += extent_size) {
            if (extents[offset_to_id(extent)].state() == extent_info_t::state_unreserved) {
                extents[offset_to_id(extent)].set_state(extent_info_t::state_free);
                extents[offset_to_id(extent)].next_in_free_list = free_list_head;
                free_list_head = extent;
                held_extents_++;
            }
        }
    }

    bool gen_extent(extent_reference_t *extent_ref_out) {
        off64_t extent;

        if (free_list_head == NULL_OFFSET) {
            extent = start + extents.get_size() * extent_size;
            if (extent == end) {
                return false;
            }

            extents.set_size(extents.get_size() + 1);
        } else {
            extent = free_list_head;
            free_list_head = extents[offset_to_id(free_list_head)].next_in_free_list;
            held_extents_--;
        }

        extent_info_t *info = &extents[offset_to_id(extent)];
        info->set_state(extent_info_t::state_in_use);

        make_extent_reference(extent, extent_ref_out);

        return true;
    }

    void make_extent_reference(off64_t extent, extent_reference_t *extent_ref_out) {
        unsigned int id = offset_to_id(extent);
        guarantee(id < extents.get_size());
        extent_info_t *info = &extents[id];
        guarantee(info->state() == extent_info_t::state_in_use);
        ++info->extent_use_refcount;
        extent_ref_out->init(extent);
    }

    void release_extent(extent_reference_t *extent_ref) {
        off64_t extent = extent_ref->release();
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

extent_manager_t::extent_manager_t(file_t *file, const log_serializer_on_disk_static_config_t *_static_config,
                                   const log_serializer_dynamic_config_t *_dynamic_config, log_serializer_stats_t *_stats)
    : stats(_stats), extent_size(_static_config->extent_size()), static_config(_static_config),
      dynamic_config(_dynamic_config), dbfile(file), state(state_reserving_extents) {
    guarantee(divides(DEVICE_BLOCK_SIZE, extent_size));

    // TODO: Why does dynamic_config have the possibility of setting a file size?
    if (file->is_block_device() || dynamic_config->file_size > 0) {
        /* If we are given a fixed file size, we pretend to be on a block device. */
        if (!file->is_block_device()) {
            if (file->get_size() <= dynamic_config->file_size) {
                file->set_size(dynamic_config->file_size);
            } else {
                logWRN("File size specified is smaller than the file actually is. To avoid "
                    "risk of smashing database, ignoring file size specification.");
            }
        }

        /* On a block device, chop the block device up into equal-sized zones, the number of
        which is determined by a configuration parameter. */
        size_t zone_size = ceil_aligned(dynamic_config->file_zone_size, extent_size);
        off64_t end = 0;
        while (end != floor_aligned((off64_t)file->get_size(), extent_size)) {
            off64_t start = end;
            end = std::min(start + zone_size, floor_aligned(file->get_size(), extent_size));
            zones.push_back(new extent_zone_t(start, end, extent_size));
        }
    } else {
        /* On an ordinary file on disk, make one "zone" that is large enough to encompass
        any file. */
        guarantee(zones.size() == 0);
        zones.push_back(new extent_zone_t(0, TERABYTE * 1024, extent_size));
    }

    next_zone = 0;
}

extent_manager_t::~extent_manager_t() {
    rassert(state == state_reserving_extents || state == state_shut_down);
}

extent_zone_t *extent_manager_t::zone_for_offset(off64_t offset) {
    assert_thread();
    if (dbfile->is_block_device() || dynamic_config->file_size > 0) {
        size_t zone_size = ceil_aligned(dynamic_config->file_zone_size, extent_size);
        return &zones[offset / zone_size];
    } else {
        /* There is only one zone on a non-block device */
        return &zones[0];
    }
}


void extent_manager_t::reserve_extent(off64_t extent, extent_reference_t *extent_ref) {
    assert_thread();
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Reserve extent %.8lx\n", this, extent);
    debugf("%s", format_backtrace(false).c_str());
#endif
    rassert(state == state_reserving_extents);
    ++stats->pm_extents_in_use;
    stats->pm_bytes_in_use += extent_size;
    zone_for_offset(extent)->reserve_extent(extent, extent_ref);
}

void extent_manager_t::prepare_initial_metablock(metablock_mixin_t *mb) {
    mb->padding = 0;
}

void extent_manager_t::start_existing(UNUSED metablock_mixin_t *last_metablock) {
    assert_thread();
    rassert(state == state_reserving_extents);
    current_transaction = NULL;
    for (boost::ptr_vector<extent_zone_t>::iterator it = zones.begin(); it != zones.end(); ++it) {
        it->reconstruct_free_list();
    }
    state = state_running;

#ifdef DEBUG_EXTENTS
    debugf("EM %p: Start. Extents in use:\n", this);
    for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
        if (extent_info(extent) == EXTENT_IN_USE) {
            fprintf(stderr, "%.8lx ", extent);
        }
    }
    fprintf(stderr, "\n");
#endif
}

void extent_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    assert_thread();
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Prepare metablock. Extents in use:\n", this);
    for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
        if (extent_info(extent) == EXTENT_IN_USE) {
            fprintf(stderr, "%.8lx ", extent);
        }
    }
    fprintf(stderr, "\n");
#endif
    rassert(state == state_running);
    metablock->padding = 0;
}

void extent_manager_t::shutdown() {
    assert_thread();
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Shutdown. Extents in use:\n", this);
    for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
        if (extent_info(extent) == EXTENT_IN_USE) {
            fprintf(stderr, "%.8lx ", extent);
        }
    }
    fprintf(stderr, "\n");
#endif

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

void extent_manager_t::gen_extent(extent_reference_t *extent_ref_out) {
    assert_thread();
    rassert(state == state_running);
    ++stats->pm_extents_in_use;
    stats->pm_bytes_in_use += extent_size;

    extent_reference_t extent_ref;
    int first_zone = next_zone;
    for (;;) {   /* Loop looking for a zone with a free extent */
        bool success = zones[next_zone].gen_extent(&extent_ref);
        next_zone = (next_zone+1) % zones.size();

        if (success) break;

        if (next_zone == first_zone) {
            /* We tried every zone and there were no free extents */
            crash("RethinkDB ran out of disk space.");
        }
    }

    /* In case we are not on a block device */
    dbfile->set_size_at_least(extent_ref.get() + extent_size);

#ifdef DEBUG_EXTENTS
    debugf("EM %p: Gen extent %.8lx\n", this, extent);
    debugf("%s", format_backtrace(false).c_str());
#endif
    extent_ref_out->init(extent_ref.release());
}

void extent_manager_t::copy_extent_reference(extent_reference_t *extent_ref, extent_reference_t *extent_ref_out) {
    off64_t offset = extent_ref->get();
    zone_for_offset(offset)->make_extent_reference(offset, extent_ref_out);
}

void extent_manager_t::release_extent_into_transaction(extent_reference_t *extent_ref, extent_transaction_t *txn) {
    release_extent_preliminaries();
    txn->push_extent(extent_ref);
}

void extent_manager_t::release_extent(extent_reference_t *extent_ref) {
    release_extent_preliminaries();
    zone_for_offset(extent_ref->get())->release_extent(extent_ref);
}

void extent_manager_t::release_extent_preliminaries() {
    assert_thread();
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Release extent %.8lx\n", this, extent);
    debugf("%s", format_backtrace(false).c_str());
#endif
    rassert(state == state_running);
    rassert(current_transaction);
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
    std::deque<off64_t> extents;
    t->reset(&extents);
    for (std::deque<off64_t>::const_iterator it = extents.begin(); it != extents.end(); ++it) {
        extent_reference_t extent_ref;
        extent_ref.init(*it);
        zone_for_offset(extent_ref.get())->release_extent(&extent_ref);
    }
}

int extent_manager_t::held_extents() {
    assert_thread();
    int total = 0;

    for (boost::ptr_vector<extent_zone_t>::iterator it = zones.begin(); it != zones.end(); ++it) {
        total += it->held_extents();
    }
    return total;
}
