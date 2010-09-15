

#ifndef __SERIALIZER_LOG_LBA_LIST_HPP__
#define __SERIALIZER_LOG_LBA_LIST_HPP__

#include "arch/resource.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"
#include "in_memory_index.hpp"
#include "disk_structure.hpp"

class lba_start_fsm_t;
class lba_changer_t;

class lba_list_t
{
    friend class lba_start_fsm_t;
    friend class lba_changer_t;
    
public:
    typedef lba_metablock_mixin_t metablock_mixin_t;
    
public:
    lba_list_t(extent_manager_t *em);
    ~lba_list_t();

public:
    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. The first form returns
    immediately; the second form might not. */
    
    void start(fd_t dbfd);
    
    struct ready_callback_t {
        virtual void on_lba_ready() = 0;
    };
    bool start(fd_t dbfd, metablock_mixin_t *last_metablock, ready_callback_t *cb);
    
public:
    /* gen_block_id() will return a block ID which is "in limbo". It is not considered to be
    "in use" until it is actually written to disk, but gen_block_id() will not return the same
    ID again until either it is written to disk and then deleted or the database is restarted. */
    
    block_id_t gen_block_id();

    off64_t get_block_offset(block_id_t block);
    
public:
    typedef lba_entry_t entry_t;
    
    struct sync_callback_t {
        virtual void on_lba_sync() = 0;
    };
    
    bool write(entry_t *entries, int nentries, metablock_mixin_t *mb_out, sync_callback_t *cb);

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
    
    in_memory_index_t *in_memory_index;
    lba_disk_structure_t *disk_structure;
};

#endif /* __SERIALIZER_LOG_LBA_LIST_HPP__ */
