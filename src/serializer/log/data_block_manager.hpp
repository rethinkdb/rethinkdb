
#ifndef __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__
#define __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__

#include "arch/io.hpp"
#include "extents/extent_manager.hpp"

// TODO: When we start up, start a new extent rather than continuing on the old extent. The
// remainder of the old extent is taboo because if we shut down badly, we might have written data
// to it but not recorded that we had written data, and if we wrote over that data again then the
// SSD controller would have to move it and there would be a risk of fragmentation.

class data_block_manager_t {
    
public:
    data_block_manager_t(extent_manager_t *em, size_t block_size)
        : state(state_unstarted), extent_manager(em), block_size(block_size) {}
    ~data_block_manager_t() {
        assert(state == state_unstarted || state == state_shut_down);
    }
    
public:
    struct metablock_mixin_t {
        off64_t last_data_extent;
        unsigned int blocks_in_last_data_extent;
    };

    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. */
public:
    void start(fd_t dbfd);
    void start(fd_t dbfd, metablock_mixin_t *last_metablock);

public:
    bool read(off64_t off_in, void *buf_out, iocallback_t *cb);

public:
    /* The offset that the data block manager chose will be left in off_out as soon as write()
    returns. The callback will be called when the data is actually on disk and it is safe to reuse
    the buffer. */
    bool write(void *buf_in, off64_t *off_out, iocallback_t *cb);

public:
    void prepare_metablock(metablock_mixin_t *metablock);

public:
    void shutdown();

private:
    enum state_t {
        state_unstarted,
        state_ready,
        state_shut_down
    } state;
    
    extent_manager_t *extent_manager;
    
    fd_t dbfd;
    size_t block_size;
    off64_t last_data_extent;
    unsigned int blocks_in_last_data_extent;
    
    off64_t gimme_a_new_offset();
};

#endif /* __SERIALIZER_LOG_DATA_BLOCK_MANAGER_HPP__ */
