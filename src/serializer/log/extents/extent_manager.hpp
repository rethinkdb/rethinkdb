
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

class extent_manager_t {
public:
    struct metablock_mixin_t {
        off64_t last_extent;
    };

private:
    typedef std::set<off64_t, std::less<off64_t>, gnew_alloc<off64_t> > reserved_extent_set_t;
    typedef std::deque<off64_t, gnew_alloc<off64_t> > free_queue_t;
    typedef std::pair<metablock_mixin_t*, free_queue_t*> holding_pair_t;
    typedef std::list<holding_pair_t, gnew_alloc<holding_pair_t> > holding_area_t;

public:
    extent_manager_t(size_t extent_size)
        :extent_size(extent_size), last_extent(-1), freed_before_mb_write(NULL)
    {
        assert(extent_size % DEVICE_BLOCK_SIZE == 0);
        freed_before_mb_write = gnew<free_queue_t>();
    }
    ~extent_manager_t() {
        /* Make sure that if we were ever started, we were shut down afterwards. */
        assert(last_extent == -1);

        // Delete all the remaining holding area queues
        for(holding_area_t::iterator i = holding_area.begin(); i != holding_area.end(); i++) {
            gdelete(i->second);
        }
    }
    
    // Reserving of extents is used by the metablock manager so that it can make sure that
    // predetermined locations in the file are available for the metablock.
    void reserve_extent(off64_t extent) {
        // Reserving extents only permitted before DB is initialized.
        assert(last_extent == -1);
        assert(extent % extent_size == 0);

        reserved_extent.insert(extent);
    }
    
public:
    void start(direct_file_t *file) {

        assert(last_extent == -1);
        last_extent = 0;
        dbfile = file;
    }

    void start(direct_file_t *file, metablock_mixin_t *last_metablock) {

        assert(last_extent == -1);
        last_extent = last_metablock->last_extent;
        dbfile = file;
    }

public:
    off64_t gen_extent() {
        
        assert(last_extent != -1);

        off64_t extent;

        if (!free_queue.empty()) {
            extent = free_queue.front();
            free_queue.pop_front();
        } else {

            // Don't give out reserved extent
            while(is_reserved(last_extent))
                last_extent += extent_size;

            extent = last_extent;
            last_extent += extent_size;
            
            dbfile->set_size_at_least(last_extent);
            
            /* TODO: What if we give out an extent and the client writes something to it, but the DB
               crashes before we record a metablock saying that the extent was given out? When we start
               back up, we might try to write over the extent again, forcing the SSD controller to copy
               it, which might lead to fragmentation. A possible solution is to make gen_extent()
               asynchronous and wait until a record of the extent being given out is safely on disk before
               we actually let anything write to the extent. Another possible solution is to wipe every
               extent that we don't know think contains valid data when we start up. */
        }
        
        return extent;
    }

    /* return an offset greater than anything we've yet given out */
    off64_t max_extent() {
        return last_extent;
    }

    void release_extent(off64_t extent) {
        // no releasing the reserved extents
        assert(!is_reserved(extent));
        assert(extent < last_extent);

#ifndef NDEBUG
        // In debug mode, make sure we ain't trying to release
        // something that was released already (that would be very
        // bad because we could pull the same extent of the free
        // queue twice).
        for (unsigned int i = 0; i < free_queue.size(); i++) {
            assert(free_queue[i] != extent);
        }
#endif

        assert(freed_before_mb_write);
        freed_before_mb_write->push_back(extent);
    }

public:
    void prepare_metablock(metablock_mixin_t *metablock) {
        metablock->last_extent = last_extent;

        // Freeze current released extent holding area and associate
        // it with this particular metablock.
        holding_area.push_back(holding_pair_t(metablock, freed_before_mb_write));
        freed_before_mb_write = gnew<free_queue_t>();
    }

    void on_metablock_comitted(metablock_mixin_t *metablock) {
        // Move released extents from the frozen holding area
        // associated with this metablock to the list of free extents,
        // as they're now safe to overwrite.
        for(holding_area_t::iterator i = holding_area.begin(); i != holding_area.end(); i++) {
            if(i->first == metablock) {
                // The metablock has been committed, it's now free to
                // overwrite extents stored in its holding area
                free_queue.insert(free_queue.end(), i->second->begin(), i->second->end());
                gdelete(i->second);
                holding_area.erase(i);
                return;
            }
        }
        fail("We committed a metablock that the extent manager doesn't know about");
    }

    void shutdown() {
        assert(last_extent != -1);
        last_extent = -1;
    }

    bool is_reserved(off64_t extent) {
        return reserved_extent.find(extent) != reserved_extent.end();
    }

public:
    size_t extent_size;

private:
    direct_file_t *dbfile;
    off64_t last_extent;
    reserved_extent_set_t reserved_extent;

    // Queue of free extents that are safe to overwrite (since the
    // metablock has been written after their release)
    free_queue_t free_queue;

    // Extents that have been released before the metablock
    // serialization began (i.e. before prepare_metablock is called),
    // so they're not yet safe to overwrite
    free_queue_t *freed_before_mb_write;

    // Holding area of freed extents for metablocks for which
    // serialization has been initiated, but hasn't completed yet. It
    // is a list of metablock pointer/free queue pairs.
    holding_area_t holding_area;
};
#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
