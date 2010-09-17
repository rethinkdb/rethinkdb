
#ifndef __SERIALIZER_LOG_LBA_DISK_STRUCTURE__
#define __SERIALIZER_LOG_LBA_DISK_STRUCTURE__

#include "arch/io.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"
#include "disk_extent.hpp"
#include "disk_superblock.hpp"

class lba_load_fsm_t;
class lba_writer_t;

class lba_disk_structure_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_disk_structure_t>
{
    friend class lba_load_fsm_t;
    friend class lba_writer_t;
    
public:
    struct load_callback_t {
        virtual void on_load_lba() = 0;
    };
    struct sync_callback_t {
        virtual void on_sync_lba() = 0;
    };

public:
    static void create(extent_manager_t *em, fd_t fd, lba_disk_structure_t **out);
    static bool load(extent_manager_t *em, fd_t fd, lba_metablock_mixin_t *metablock,
        lba_disk_structure_t **out, load_callback_t *cb);
    
    void add_entry(block_id_t block_id, off64_t offset);
    bool sync(sync_callback_t *cb);
    void prepare_metablock(lba_metablock_mixin_t *mb_out);
    
    void destroy();   // Delete both in memory and on disk
    void shutdown();   // Delete just in memory

private:
    extent_manager_t *em;
    fd_t fd;

public:
    lba_disk_superblock_t *superblock;
    lba_disk_extent_t *last_extent;

private:
    /* Use create(), load(), destroy(), and shutdown() instead */
    lba_disk_structure_t() {}
    ~lba_disk_structure_t() {}
};

#endif /* __SERIALIZER_LOG_LBA_DISK_STRUCTURE__ */
