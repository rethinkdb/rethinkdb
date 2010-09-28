
#ifndef __SERIALIZER_LOG_EXTENT_MANAGER_HPP__
#define __SERIALIZER_LOG_EXTENT_MANAGER_HPP__

#include "utils.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config/args.hpp"
#include <set>
#include <deque>
#include <functional>

/* This is a stub extent manager. It never recycles extents. */

class extent_manager_t {
private:
    typedef std::set<off64_t, std::less<off64_t>, gnew_alloc<off64_t> > reserved_extent_set_t;
    typedef std::deque<off64_t, gnew_alloc<off64_t> > free_queue_t;
public:
    extent_manager_t(size_t extent_size)
        :extent_size(extent_size), last_extent(-1), last_truncated_extent(-1)
    {
        assert(extent_size % DEVICE_BLOCK_SIZE == 0);
    }
    ~extent_manager_t() {
        /* Make sure that if we were ever started, we were shut down afterwards. */
        assert(last_extent == -1);
    }
    
    // Reserving of extents is used by the metablock manager so that it can make sure that
    // predetermined locations in the file are available for the metablock.
    void reserve_extent(off64_t extent, bool truncate) {
        // Reserving extents only permitted before DB is initialized.
        assert(last_extent == -1);
        assert(extent % extent_size == 0);

        reserved_extent.insert(extent);
        truncate_reservations = truncate;
    }
    
public:
    struct metablock_mixin_t {
        off64_t last_extent;
        off64_t last_truncated_extent;
    };

public:
    void start(fd_t fd) {
        /* grab stats about the file */
        int res = fstat(fd, &file_stat);
        check("Stat request on file failed", res != 0);
        assert(last_extent == -1);
        last_extent = 0;
        last_truncated_extent = 0;
        dbfd = fd;
    }

    void start(fd_t fd, metablock_mixin_t *last_metablock) {
        /* grab stats about the file */
        int res = fstat(fd, &file_stat);
        check("Stat request on file failed", res != 0);
        assert(last_extent == -1);
        last_extent = last_metablock->last_extent;
        last_truncated_extent = last_metablock->last_truncated_extent;
        dbfd = fd;
        if (truncate_reservations && !reserved_extent.empty()) {
            /* sets are sorted so the last element is the maximal one, (thats what rbegin points to) */
            truncate(*reserved_extent.rbegin() + extent_size);
        }
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

            extend_if_necessary();

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

        free_queue.push_back(extent);
    }

public:
    void prepare_metablock(metablock_mixin_t *metablock) {
        metablock->last_extent = last_extent;
        metablock->last_truncated_extent = last_truncated_extent;
    }

    void shutdown() {
        assert(last_extent != -1);
        last_extent = -1;
        last_truncated_extent = -1;
    }

    bool is_reserved(off64_t extent) {
        return reserved_extent.find(extent) != reserved_extent.end();
    }

private:
    /* \brief truncate the file to have room for extents many extents
     */
    void truncate(off64_t last_truncated_extent) {
        if (S_ISBLK(file_stat.st_mode)) {
            ; //we're on a block device so do nothing
        } else if (S_ISREG(file_stat.st_mode)) {
            this->last_truncated_extent = last_truncated_extent;
            int res = ftruncate(dbfd, last_truncated_extent);
            check("Could not expand file", res == -1);
        } else {
            fail("Unknown file type");
        }
    }

    // Extends the db file by calling underlysing FS's truncate iff
    // we're getting past the last truncated extent.
    void extend_if_necessary() {
        if(last_truncated_extent < last_extent) {
            truncate(last_truncated_extent + extent_size * FILE_GROWTH_RATE_IN_EXTENTS);
        }
    }

public:
    size_t extent_size;
    struct stat file_stat; /* stats about the file, we could make this only what we need to save memory (is it worth it?) */

private:
    bool truncate_reservations;
    fd_t dbfd;
    off64_t last_extent;
    off64_t last_truncated_extent;
    reserved_extent_set_t reserved_extent;
    free_queue_t free_queue;
};
#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
