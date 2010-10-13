

#ifndef __SERIALIZER_LOG_LBA_LIST_HPP__
#define __SERIALIZER_LOG_LBA_LIST_HPP__

#include "serializer/types.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"
#include "in_memory_index.hpp"
#include "disk_structure.hpp"
#include "serializer/log/data_block_manager.hpp"

class lba_start_fsm_t;
class lba_syncer_t;
class gc_fsm_t;

class lba_list_t
{
    friend class lba_start_fsm_t;
    friend class lba_syncer_t;
    friend class gc_fsm_t;
    
public:
    typedef lba_metablock_mixin_t metablock_mixin_t;
    
public:
    lba_list_t(data_block_manager_t *dbm, extent_manager_t *em);
    ~lba_list_t();

public:
    /* When initializing the database from scratch, call start() with just the database FD. When
    restarting an existing database, call start() with the last metablock. The first form returns
    immediately; the second form might not. */
    
    void start(direct_file_t *dbfile);
    
    struct ready_callback_t {
        virtual void on_lba_ready() = 0;
    };
    bool start(direct_file_t *dbfile, metablock_mixin_t *last_metablock, ready_callback_t *cb);
    
public:
    /* Returns DELETE_BLOCK if the block does not exist */
    off64_t get_block_offset(ser_block_id_t block);
    
    /* Returns a block ID such that all blocks that exist are guaranteed to have IDs less than
    that block ID. */
    ser_block_id_t max_block_id();
    
#ifndef NDEBUG
    bool is_extent_referenced(off64_t offset);
    bool is_offset_referenced(off64_t offset);
    int extent_refcount(off64_t offset);
#endif
    
public:
    void set_block_offset(ser_block_id_t block, off64_t offset);
    
    struct sync_callback_t {
        virtual void on_lba_sync() = 0;
    };
    bool sync(sync_callback_t *cb);
    
    void prepare_metablock(metablock_mixin_t *mb_out);

public:
    struct shutdown_callback_t {
        virtual void on_lba_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

private:
    shutdown_callback_t *shutdown_callback;
    gc_fsm_t *gc_fsm;
    bool __shutdown();

private:
    data_block_manager_t *data_block_manager;
    extent_manager_t *extent_manager;
    
    enum state_t {
        state_unstarted,
        state_starting_up,
        state_ready,
        state_shutting_down,
        state_shut_down
    } state;
    
    direct_file_t *dbfile;
    
    in_memory_index_t *in_memory_index;
    
    lba_disk_structure_t *disk_structure;
    
    void gc();

    bool we_want_to_gc();

};

#endif /* __SERIALIZER_LOG_LBA_LIST_HPP__ */
