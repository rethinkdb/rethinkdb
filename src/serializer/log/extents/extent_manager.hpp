
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
#include "containers/segmented_vector.hpp"

#define EXTENT_UNRESERVED (off64_t(-2))
#define EXTENT_IN_USE (off64_t(-3))
#define EXTENT_FREE_LIST_END (off64_t(-4))
#define EXTENT_IN_LIMBO (off64_t(-5))

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

public:
    class transaction_t :
        alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, transaction_t>
    {
        friend class extent_manager_t;
        transaction_t() { }
        free_queue_t free_queue;
    };

public:
    extent_manager_t(size_t extent_size)
        : extent_size(extent_size), state(state_reserving_extents), n_extents_in_use(0)
    {
        assert(extent_size % DEVICE_BLOCK_SIZE == 0);
    }
    
    ~extent_manager_t() {
        assert(state == state_reserving_extents || state == state_shut_down);
    }

public:
    /* When we load a database, we use reserve_extent() to inform the extent manager
    which extents were already in use */
    
    void reserve_extent(off64_t extent) {

#ifndef NDEBUG
        flockfile(stderr);
        debugf("Reserve extent %.8x:\n", extent);
        print_backtrace(stderr, false);
        fprintf(stderr, "\n");
        funlockfile(stderr);
#endif
        
        assert(state == state_reserving_extents);
        assert(extent % extent_size == 0);
        
        if (extent / extent_size >= extents.get_size()) {
            extents.set_size(extent / extent_size + 1, EXTENT_UNRESERVED);
        }
        
        assert(extent_info(extent) == EXTENT_UNRESERVED);
        extent_info(extent) = EXTENT_IN_USE;
        
        n_extents_in_use++;
    }

public:
    void start(direct_file_t *file) {
        
        assert(state == state_reserving_extents);
        
        dbfile = file;
        
        current_transaction = NULL;
        
        /* Reconstruct free list */
        
        free_list_head = EXTENT_FREE_LIST_END;
        
        for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
            if (extent_info(extent) == EXTENT_UNRESERVED) {
                extent_info(extent) = free_list_head;
                free_list_head = extent;
            }
        }
        
        state = state_running;

#ifndef NDEBUG
        flockfile(stderr);
        debugf("The following extents are in use as of startup time:\n");
        int count = 0;
        for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
            if (extent_info(extent) == EXTENT_IN_USE) {
                fprintf(stderr, "%.8lx ", extent);
                count++;
            }
        }
        fprintf(stderr, "\n");
        debugf("Total: %d\n", count);
        assert(count == n_extents_in_use);
        funlockfile(stderr);
#endif
    }

    void start(direct_file_t *file, metablock_mixin_t *last_metablock) {
        
        start(file);
        
        assert(n_extents_in_use == last_metablock->debug_extents_in_use);
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
        
        if (free_list_head == EXTENT_FREE_LIST_END) {
            extent = extents.get_size() * extent_size;
            extents.set_size(extents.get_size() + 1);
            dbfile->set_size_at_least(extent + extent_size);
        } else {
            extent = free_list_head;
            free_list_head = extent_info(free_list_head);
        }
        
        extent_info(extent) = EXTENT_IN_USE;

#ifndef NDEBUG
        flockfile(stderr);
        debugf("Gen extent %.8x:\n", extent);
        print_backtrace(stderr, false);
        fprintf(stderr, "\n");
        funlockfile(stderr);
#endif
        
        return extent;
    }

    void release_extent(off64_t extent) {

#ifndef NDEBUG
        flockfile(stderr);
        debugf("Release extent %.8x:\n", extent);
        print_backtrace(stderr, false);
        fprintf(stderr, "\n");
        funlockfile(stderr);
#endif
        
        assert(state == state_running);
        assert(current_transaction);
        
        n_extents_in_use--;
        
        assert(extent_info(extent) == EXTENT_IN_USE);
        extent_info(extent) = EXTENT_IN_LIMBO;

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
            assert(extent_info(extent) == EXTENT_IN_LIMBO);
            extent_info(extent) = free_list_head;
            free_list_head = extent;
        }
        
        delete t;
    }

public:
    void prepare_metablock(metablock_mixin_t *metablock) {
        assert(state == state_running);
        metablock->debug_extents_in_use = n_extents_in_use;
    }

    void shutdown() {
        assert(state == state_running);
        assert(!current_transaction);
        state = state_shut_down;
        
#ifndef NDEBUG
        flockfile(stderr);
        debugf("The following extents are in use as of shutdown time:\n");
        int count = 0;
        for (off64_t extent = 0; extent < (unsigned)(extents.get_size() * extent_size); extent += extent_size) {
            if (extent_info(extent) == EXTENT_IN_USE) {
                fprintf(stderr, "%.8lx ", extent);
                count++;
            }
        }
        fprintf(stderr, "\n");
        debugf("Total: %d\n", count);
        assert(count == n_extents_in_use);
        funlockfile(stderr);
#endif
    }

public:
    size_t extent_size;

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
    
    /* Combination free-list and extent map. Contains one entry per extent.
    During the state_reserving_extents phase, contains EXTENT_UNRESERVED or
    EXTENT_IN_USE at each entry. When we transition to the state_running phase,
    we link all of the EXTENT_UNRESERVED entries together into an extent free
    list, such that each free entry contains the offset of the next free entry,
    and the last one contains EXTENT_FREE_LIST_END. */
    
    segmented_vector_t<off64_t, MAX_DATA_EXTENTS> extents;
    
    off64_t &extent_info(off64_t extent) {
        assert(extent % extent_size == 0);
        assert(extent / extent_size < extents.get_size());
        return extents[extent / extent_size];
    }
    
    off64_t free_list_head;
    
    int n_extents_in_use;
    
    transaction_t *current_transaction;
};
#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
