
#ifndef __SERIALIZER_LOG_EXTENT_MANAGER_HPP__
#define __SERIALIZER_LOG_EXTENT_MANAGER_HPP__

#include <assert.h>
#include <sys/types.h>

/* This is a stub extent manager. It never recycles extents. */

class extent_manager_t {

public:
    extent_manager_t(size_t extent_size)
        : extent_size(extent_size), last_extent(-1), reserved_extent(-1)
        {
        assert(extent_size % DEVICE_BLOCK_SIZE == 0);
    }
    ~extent_manager_t() {
        /* Make sure that if we were ever started, we were shut down afterwards. */
        assert(last_extent == -1);
    }
    
    // Reserving of extents is used by the metablock manager so that it can make sure that
    // predetermined locations in the file are available for the metablock.
    void reserve_extent(off64_t extent) {
        // Reserving extents only permitted before DB is initialized.
        assert(last_extent == -1);
        assert(extent % extent_size == 0);
        // Stub extent manager only allows one reservation
        assert(reserved_extent == -1);
        reserved_extent = extent;
    }
    
public:
    struct metablock_mixin_t {
        off64_t last_extent;
    };
    
public:
    void start(fd_t fd) {
        assert(last_extent == -1);
        last_extent = 0;
        dbfd = fd;
    }
    
    void start(fd_t fd, metablock_mixin_t *last_metablock) {
        assert(last_extent == -1);
        last_extent = last_metablock->last_extent;
        dbfd = fd;
    }

public:
    off64_t gen_extent() {
        
        assert(last_extent != -1);
        
        // Don't give out reserved extent
        if (last_extent == reserved_extent) last_extent += extent_size;
        
        off64_t extent = last_extent;
        last_extent += extent_size;
        
        int res = ftruncate(dbfd, last_extent);
        check("Could not expand file", res == -1);
        
        /* TODO: What if we give out an extent and the client writes something to it, but the DB
        crashes before we record a metablock saying that the extent was given out? When we start
        back up, we might try to write over the extent again, forcing the SSD controller to copy
        it, which might lead to fragmentation. A possible solution is to make gen_extent()
        asynchronous and wait until a record of the extent being given out is safely on disk before
        we actually let anything write to the extent. Another possible solution is to wipe every
        extent that we don't know think contains valid data when we start up. */
        
        return extent;
    }
    void release_extent(off64_t extent) {
        // No-op at the moment because we don't recycle extents
    }

public:
    void prepare_metablock(metablock_mixin_t *metablock) {
        metablock->last_extent = last_extent;
    }

public:
    void shutdown() {
        assert(last_extent != -1);
        last_extent = -1;
    }

public:
    size_t extent_size;

private:
    fd_t dbfd;
    off64_t last_extent;
    off64_t reserved_extent;
};

#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
