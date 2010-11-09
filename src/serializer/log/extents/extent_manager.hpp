
#ifndef __SERIALIZER_LOG_EXTENT_MANAGER_HPP__
#define __SERIALIZER_LOG_EXTENT_MANAGER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <set>
#include <deque>
#include <unistd.h>
#include <list>
#include "utils.hpp"
#include "arch/arch.hpp"
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "containers/segmented_vector.hpp"
#include "logger.hpp"

#define NULL_OFFSET off64_t(-1)

#define EXTENT_UNRESERVED (off64_t(-2))
#define EXTENT_IN_USE (off64_t(-3))
#define EXTENT_FREE_LIST_END (off64_t(-4))

class extent_zone_t {
    
    off64_t start, end;
    size_t extent_size;
    
    unsigned int offset_to_id(off64_t extent) {
        assert(extent < end);
        assert(extent >= start);
        assert(extent % extent_size == 0);
        return (extent - start) / extent_size;
    }
    
    /* Combination free-list and extent map. Contains one entry per extent.
    During the state_reserving_extents phase, contains EXTENT_UNRESERVED or
    EXTENT_IN_USE at each entry. When we transition to the state_running phase,
    we link all of the EXTENT_UNRESERVED entries in each zone together into an
    extent free list, such that each free entry contains the offset of the next
    free entry, and the last one contains EXTENT_FREE_LIST_END. There is one
    free list per zone. */
    
    segmented_vector_t<off64_t, MAX_DATA_EXTENTS> extents;
    
    off64_t free_list_head;
    
public:
    extent_zone_t(off64_t start, off64_t end, size_t extent_size)
        : start(start), end(end), extent_size(extent_size)
    {
        assert(start % extent_size == 0);
        assert(end % extent_size == 0);
    }
    
    void reserve_extent(off64_t extent) {
        
        unsigned int id = offset_to_id(extent);
        
        if (id >= extents.get_size()) extents.set_size(id + 1, EXTENT_UNRESERVED);
        
        assert(extents[id] == EXTENT_UNRESERVED);
        extents[id] = EXTENT_IN_USE;
    }
    
    void reconstruct_free_list() {
        
        free_list_head = EXTENT_FREE_LIST_END;
        
        for (off64_t extent = start; extent < start + (off64_t)(extents.get_size() * extent_size); extent += extent_size) {
            if (extents[offset_to_id(extent)] == EXTENT_UNRESERVED) {
                extents[offset_to_id(extent)] = free_list_head;
                free_list_head = extent;
            }
        }
    }
    
    off64_t gen_extent() {
        
        off64_t extent;
        
        if (free_list_head == EXTENT_FREE_LIST_END) {
        
            extent = start + extents.get_size() * extent_size;
            if (extent == end) return NULL_OFFSET;
            
            extents.set_size(extents.get_size() + 1);
            
        } else {
        
            extent = free_list_head;
            free_list_head = extents[offset_to_id(free_list_head)];
        }
        
        extents[offset_to_id(extent)] = EXTENT_IN_USE;
        
        return extent;
    }
    
    void release_extent(off64_t extent) {
        
        assert(extents[offset_to_id(extent)] == EXTENT_IN_USE);
        extents[offset_to_id(extent)] = free_list_head;
        free_list_head = extent;
    }
};

class extent_manager_t {

public:
    struct metablock_mixin_t {
    
        /* When we shut down, we store the number of extents in use in the metablock.
        When we start back up and reconstruct the free list, we assert that it is the same.
        I would wrap this in #ifndef NDEBUG except that I don't want the format to change
        between debug and release mode. */
        int debug_extents_in_use;
    };

private:
    typedef std::set<off64_t, std::less<off64_t>, gnew_alloc<off64_t> > reserved_extent_set_t;
    typedef std::deque<off64_t, gnew_alloc<off64_t> > free_queue_t;
    
    log_serializer_static_config_t *static_config;
    log_serializer_dynamic_config_t *dynamic_config;

public:
    size_t extent_size;   /* Same as static_config->extent_size */
    
public:
    class transaction_t :
        alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, transaction_t>
    {
        friend class extent_manager_t;
        transaction_t() { }
        free_queue_t free_queue;
    };

public:
    extent_manager_t(direct_file_t *file, log_serializer_static_config_t *static_config, log_serializer_dynamic_config_t *dynamic_config)
        : static_config(static_config), dynamic_config(dynamic_config), extent_size(static_config->extent_size), dbfile(file), state(state_reserving_extents), n_extents_in_use(0)
    {
        assert(extent_size % DEVICE_BLOCK_SIZE == 0);
        
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
            num_zones = 0;
            while (end != floor_aligned((off64_t)file->get_size(), extent_size)) {
                off64_t start = end;
                end = std::min(start + zone_size, floor_aligned(file->get_size(), extent_size));
                zones[num_zones++] = gnew<extent_zone_t>(start, end, extent_size);
            }
            
        } else {
            /* On an ordinary file on disk, make one "zone" that is large enough to encompass
            any file. */
            zones[0] = gnew<extent_zone_t>(0, TERABYTE * 1024, extent_size);
            num_zones = 1;
        }
        
        next_zone = 0;
    }
    
    ~extent_manager_t() {
        assert(state == state_reserving_extents || state == state_shut_down);
        
        for (unsigned int i = 0; i < num_zones; i++) {
            gdelete(zones[i]);
        }
    }

private:
    unsigned int num_zones;
    extent_zone_t *zones[MAX_FILE_ZONES];
    int next_zone;    /* Which zone to give the next extent from */
    
    extent_zone_t *zone_for_offset(off64_t offset) {
        if (dbfile->is_block_device() || dynamic_config->file_size > 0) {
            size_t zone_size = ceil_aligned(dynamic_config->file_zone_size, extent_size);
            return zones[offset / zone_size];
        } else {
            /* There is only one zone on a non-block device */
            return zones[0];
        }
    }
    
public:
    /* When we load a database, we use reserve_extent() to inform the extent manager
    which extents were already in use */
    
    void reserve_extent(off64_t extent) {
        
#ifdef DEBUG_EXTENTS
        debugf("EM %p: Reserve extent %.8lx\n", this, extent);
        print_backtrace(stderr, false);
#endif
        assert(state == state_reserving_extents);
        n_extents_in_use++;
        zone_for_offset(extent)->reserve_extent(extent);
    }

public:
    void start_new() {
        
        assert(state == state_reserving_extents);
        current_transaction = NULL;
        for (unsigned int i = 0; i < num_zones; i++) {
            zones[i]->reconstruct_free_list();
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

    void start_existing(metablock_mixin_t *last_metablock) {
        
        start_new();
        assert(n_extents_in_use == last_metablock->debug_extents_in_use);
    }

    void prepare_metablock(metablock_mixin_t *metablock) {
#ifdef DEBUG_EXTENTS
        debugf("EM %p: Prepare metablock. Extents in use:\n", this);
        for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
            if (extent_info(extent) == EXTENT_IN_USE) {
                fprintf(stderr, "%.8lx ", extent);
            }
        }
        fprintf(stderr, "\n");
#endif
        assert(state == state_running);
        metablock->debug_extents_in_use = n_extents_in_use;
    }

    void shutdown() {
        
#ifdef DEBUG_EXTENTS
        debugf("EM %p: Shutdown. Extents in use:\n", this);
        for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
            if (extent_info(extent) == EXTENT_IN_USE) {
                fprintf(stderr, "%.8lx ", extent);
            }
        }
        fprintf(stderr, "\n");
#endif
        
        assert(state == state_running);
        assert(!current_transaction);
        state = state_shut_down;
    }

public:
    /* The extent manager uses transactions to make sure that extents are not freed
    before it is safe to free them. An extent manager transaction is created for every
    log serializer write transaction. Any extents that are freed in the course of
    perfoming the write transaction are recorded in a "holding area" on the extent
    manager transaction. They are only allowed to be reused after the transaction
    is committed; the log serializer only commits the transaction after the metablock
    has been written. This guarantees that we will not overwrite extents that the
    most recent metablock points to. */
    
    transaction_t *begin_transaction() {
        
        assert(!current_transaction);
        transaction_t *t = current_transaction = new transaction_t();
        return t;
    }
    
    off64_t gen_extent() {
        
        assert(state == state_running);
        assert(current_transaction);
        n_extents_in_use++;
        
        off64_t extent;
        int first_zone = next_zone;
        for (;;) {   /* Loop looking for a zone with a free extent */
            
            extent = zones[next_zone]->gen_extent();
            next_zone = (next_zone+1) % num_zones;
            
            if (extent != NULL_OFFSET) break;
            
            if (next_zone == first_zone) {
                /* We tried every zone and there were no free extents */
                fail("RethinkDB ran out of disk space.");
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

    void release_extent(off64_t extent) {

#ifdef DEBUG_EXTENTS
        debugf("EM %p: Release extent %.8lx\n", this, extent);
        print_backtrace(stderr, false);
#endif
        assert(state == state_running);
        assert(current_transaction);
        n_extents_in_use--;
        current_transaction->free_queue.push_back(extent);
    }
    
    void end_transaction(transaction_t *t) {
        
        assert(current_transaction == t);
        assert(t);
        current_transaction = NULL;
    }
    
    void commit_transaction(transaction_t *t) {
        
        for (free_queue_t::iterator it = t->free_queue.begin(); it != t->free_queue.end(); it++) {
            off64_t extent = *it;
            zone_for_offset(extent)->release_extent(extent);
        }
        delete t;
    }

private:
    direct_file_t *dbfile;
    
    /* During serializer startup, each component informs the extent manager
    which extents in the file it was using at shutdown. This is the
    "state_reserving_extents" phase. Then extent_manager_t::start() is called
    and it is in "state_running", where it releases and generates extents. */
    enum state_t {
        state_reserving_extents,
        state_running,
        state_shut_down
    } state;
    
    int n_extents_in_use;
    
    transaction_t *current_transaction;
};
#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
