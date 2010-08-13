
#ifndef __SERIALIZER_LOG_LBA_LIST_HPP__
#define __SERIALIZER_LOG_LBA_LIST_HPP__

#include "arch/resource.hpp"
#include "arch/io.hpp"
#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"
#include "../extents/extent_manager.hpp"

// Used internally by lba_list_t
struct lba_extent_buf_t;
struct lba_write_t;
struct lba_start_fsm_t;

#define LBA_MAGIC_SIZE 8

class lba_list_t
{
    friend class lba_extent_buf_t;
    friend class lba_write_t;
    friend class lba_start_fsm_t;
    
public:
    lba_list_t(extent_manager_t *em);
    ~lba_list_t();

private:
    // This is the on-disk format of an LBA extent
    struct lba_entry_t {
        block_id_t block_id;
        off64_t offset;   // Is either an offset into the file or DELETE_BLOCK
    };
    struct lba_header_t {
        char magic[LBA_MAGIC_SIZE];
        off64_t next_extent_in_chain;
        int entries_in_next_extent_in_chain;
    };
    struct lba_extent_t {
        
        // Header needs to be padded to a multiple of sizeof(lba_entry_t)
        lba_header_t header;
        char padding[sizeof(lba_entry_t) - sizeof(lba_header_t) % sizeof(lba_entry_t)];
        
        lba_entry_t entries[0];
    };

public:
    struct metablock_mixin_t {
        off64_t lba_chain_head;
        int entries_in_lba_chain_head;
        block_id_t highest_block_id_plus_one;
    };

    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. The first form returns
    immediately; the second form might not. */
public:
    void start(fd_t dbfd);
    struct ready_callback_t {
        virtual void on_lba_ready() = 0;
    };
    bool start(fd_t dbfd, metablock_mixin_t *last_metablock, ready_callback_t *cb);
    
    /* gen_block_id() will return a block ID which is "in limbo". It is not considered to be
    "in use" until it is actually written to disk, but gen_block_id() will not return the same
    ID again until either it is written to disk and then deleted or the database is restarted. */
public:
    block_id_t gen_block_id();

public:
    off64_t get_block_offset(block_id_t block);
    
    /* To store offsets in the file:
    
    1. Call set_block_offset() or delete_block() one or more times
    2. Call sync()
    3. Before calling set_block_offset() again, call prepare_metablock()
    4. Only after sync() returns true or calls its callback, write the prepared metablock
    
    Note that sync() will return true immediately if there is nothing new to write to the file since
    the last sync.
    */
public:
    void set_block_offset(block_id_t block, off64_t offset);
    void delete_block(block_id_t block);
    struct sync_callback_t : public intrusive_list_node_t<sync_callback_t> {
        virtual void on_lba_sync() = 0;
    };
    bool sync(sync_callback_t *cb);
private:
    void make_entry_in_extent(block_id_t block, off64_t offset);

public:
    void prepare_metablock(metablock_mixin_t *metablock);

public:
    void shutdown();

private:
    extent_manager_t *extent_manager;
    
    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shut_down
    } state;
    
    fd_t dbfd;
    
    enum block_state_t {
        block_unused,
        block_in_limbo,
        block_used
    };
    
    struct block_info_t {
        bool found;   // During startup, this is false on the blocks that we still need entries for
        block_state_t state;
        off64_t offset;   // If state == block_used, the location of the block in the file
    };
    segmented_vector_t<block_info_t, MAX_BLOCK_ID> blocks;
    
    lba_extent_buf_t *current_extent;
    
    // These are only meaningful if current_extent is NULL
    off64_t offset_of_last_extent;
    int entries_in_last_extent;
    
    lba_write_t *last_write;
};

#endif /* __SERIALIZER_LOG_LBA_LIST_HPP__ */
