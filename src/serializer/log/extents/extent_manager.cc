#include "utils.hpp"
#include "extent_manager.hpp"

#define EXTENT_UNRESERVED (off64_t(-2))
#define EXTENT_IN_USE (off64_t(-3))
#define EXTENT_FREE_LIST_END (off64_t(-4))

static perfmon_counter_t pm_extents_in_use("serializer_extents_in_use");
static perfmon_counter_t pm_bytes_in_use("serializer_bytes_in_use", false);

struct extent_info_t {

    enum state_t {
        state_unreserved,
        state_in_use,
        state_free
    } state;

    off64_t next_in_free_list;   // Valid if state == state_free
public:
    extent_info_t() : state(state_unreserved), next_in_free_list(-1) {}
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
    int _held_extents;
public:
    int held_extents() {return _held_extents;}
    
public:
    extent_zone_t(off64_t start, off64_t end, size_t extent_size)
        : start(start), end(end), extent_size(extent_size), _held_extents(0)
    {
#ifndef NDEBUG
        bool start_aligned = start % extent_size == 0;
        rassert(start_aligned);
        bool end_aligned = end % extent_size == 0;
        rassert(end_aligned);
#endif
    }
    
    void reserve_extent(off64_t extent) {
        unsigned int id = offset_to_id(extent);
        
        if (id >= extents.get_size()) {
            extent_info_t default_info;
            default_info.state = extent_info_t::state_unreserved;
            extents.set_size(id + 1, default_info);
        }
        
        rassert(extents[id].state == extent_info_t::state_unreserved);
        extents[id].state = extent_info_t::state_in_use;
    }
    
    void reconstruct_free_list() {
        free_list_head = NULL_OFFSET;
        
        for (off64_t extent = start; extent < start + (off64_t)(extents.get_size() * extent_size); extent += extent_size) {
            if (extents[offset_to_id(extent)].state == extent_info_t::state_unreserved) {
                extents[offset_to_id(extent)].state = extent_info_t::state_free;
                extents[offset_to_id(extent)].next_in_free_list = free_list_head;
                free_list_head = extent;
                _held_extents++;
            }
        }
    }
    
    off64_t gen_extent() {
        off64_t extent;
        
        if (free_list_head == NULL_OFFSET) {
            extent = start + extents.get_size() * extent_size;
            if (extent == end) return NULL_OFFSET;
            
            extents.set_size(extents.get_size() + 1);
        } else {
            extent = free_list_head;
            free_list_head = extents[offset_to_id(free_list_head)].next_in_free_list;
            _held_extents--;
        }
        
        extents[offset_to_id(extent)].state = extent_info_t::state_in_use;
        
        return extent;
    }
    
    void release_extent(off64_t extent) {
        extent_info_t *info = &extents[offset_to_id(extent)];
        rassert(info->state == extent_info_t::state_in_use);
        info->state = extent_info_t::state_free;
        info->next_in_free_list = free_list_head;
        free_list_head = extent;
        _held_extents++;
    } 
};

extent_manager_t::extent_manager_t(direct_file_t *file, log_serializer_static_config_t *static_config, log_serializer_dynamic_config_t *dynamic_config)
    : static_config(static_config), dynamic_config(dynamic_config), extent_size(static_config->extent_size()), dbfile(file), state(state_reserving_extents), n_extents_in_use(0)
{
#ifndef NDEBUG
    bool modcmp = extent_size % DEVICE_BLOCK_SIZE == 0;
    rassert(modcmp);
#endif
    
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

void extent_manager_t::reserve_extent(off64_t extent) {
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Reserve extent %.8lx\n", this, extent);
    print_backtrace(stderr, false);
#endif
    rassert(state == state_reserving_extents);
    n_extents_in_use++;
    pm_extents_in_use++;
    pm_bytes_in_use += extent_size;
    zone_for_offset(extent)->reserve_extent(extent);
}

void extent_manager_t::prepare_initial_metablock(metablock_mixin_t *mb, int extents_in_use) {
    mb->debug_extents_in_use = extents_in_use;
}

void extent_manager_t::start_existing(UNUSED metablock_mixin_t *last_metablock) {

    rassert(state == state_reserving_extents);
    current_transaction = NULL;
    for (boost::ptr_vector<extent_zone_t>::iterator it  = zones.begin(); 
                                                      it != zones.end(); 
                                                      it++) {
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

    rassert(n_extents_in_use == last_metablock->debug_extents_in_use);
}

void extent_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
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
    metablock->debug_extents_in_use = n_extents_in_use;
}

void extent_manager_t::shutdown() {
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

extent_manager_t::transaction_t *extent_manager_t::begin_transaction() {
    rassert(!current_transaction);
    transaction_t *t = current_transaction = new transaction_t();
    return t;
}

off64_t extent_manager_t::gen_extent() {
    rassert(state == state_running);
    rassert(current_transaction);
    n_extents_in_use++;
    pm_extents_in_use++;
    pm_bytes_in_use += extent_size;
    
    off64_t extent;
    int first_zone = next_zone;
    for (;;) {   /* Loop looking for a zone with a free extent */
        
        extent = zones[next_zone].gen_extent();
        next_zone = (next_zone+1) % zones.size();
        
        if (extent != NULL_OFFSET) break;
        
        if (next_zone == first_zone) {
            /* We tried every zone and there were no free extents */
            crash("RethinkDB ran out of disk space.");
        }
    }
    
    /* In case we are not on a block device */
    dbfile->set_size_at_least(extent + extent_size);
    
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Gen extent %.8lx\n", this, extent);
    print_backtrace(stderr, false);
#endif
    return extent;
}

void extent_manager_t::release_extent(off64_t extent) {
#ifdef DEBUG_EXTENTS
    debugf("EM %p: Release extent %.8lx\n", this, extent);
    print_backtrace(stderr, false);
#endif
    rassert(state == state_running);
    rassert(current_transaction);
    n_extents_in_use--;
    pm_extents_in_use--;
    pm_bytes_in_use -= extent_size;
    current_transaction->free_queue.push_back(extent);
}

void extent_manager_t::end_transaction(UNUSED transaction_t *t) {
    rassert(current_transaction == t);
    rassert(t);
    current_transaction = NULL;
}

void extent_manager_t::commit_transaction(transaction_t *t) {
    for (free_queue_t::iterator it = t->free_queue.begin(); it != t->free_queue.end(); it++) {
        off64_t extent = *it;
        zone_for_offset(extent)->release_extent(extent);
    }
    delete t;
}

int extent_manager_t::held_extents() {
    int total = 0;

    for (boost::ptr_vector<extent_zone_t>::iterator it  = zones.begin(); 
                                                    it != zones.end(); 
                                                    it++) {
        total += it->held_extents();
    }
    return total;
}
