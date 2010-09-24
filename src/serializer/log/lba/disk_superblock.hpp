
#ifndef __SERIALIZER_LOG_LBA_DISK_SUPERBLOCK__
#define __SERIALIZER_LOG_LBA_DISK_SUPERBLOCK__

#include "containers/intrusive_list.hpp"
#include "arch/arch.hpp"
#include "extent.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"
#include "disk_extent.hpp"

/* TODO: Make sure that we call back sync() callbacks in the same order as sync()
was called */

class lba_disk_superblock_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_disk_superblock_t>
{
    
public:
    struct load_callback_t {
        virtual void on_load_lba_superblock() = 0;
    };
    struct sync_callback_t : public extent_t<void>::sync_callback_t {
        virtual void on_sync_lba_superblock() = 0;
        void on_sync_extent() { on_sync_lba_superblock(); }
    };

public:
    static void create(extent_manager_t *em, direct_file_t *file, lba_disk_superblock_t **out) {
        lba_disk_superblock_t *s = new lba_disk_superblock_t();
        s->em = em;
        s->file = file;
        s->modified = false;
        
        extent_t<void>::create(em, file, &s->current_extent);
        s->amount_in_current_extent = 0;
        s->offset = NULL_OFFSET;
        
        *out = s;
    }

private:
    /* FSM to load an LBA superblock and all of the LBA extents in it */
    
    struct loader_t :
        public extent_t<void>::load_callback_t,
        public lba_disk_extent_t::load_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, loader_t>
    {
        lba_disk_superblock_t *owner;
        loader_t(lba_disk_superblock_t *owner, off64_t offset, int count)
            : owner(owner), offset(offset), count(count) {}
        
        off64_t offset;
        int count;
        int outstanding_cbs;
        lba_disk_superblock_t::load_callback_t *callback;
        
        bool load_superblock(lba_disk_superblock_t::load_callback_t *cb) {
            
            owner->offset = offset;
            
            off64_t extent_offset = offset - offset % owner->em->extent_size;
            owner->amount_in_current_extent = ceil_aligned(
                offset - extent_offset + lba_superblock_t::entry_count_to_file_size(count),
                DEVICE_BLOCK_SIZE); 
            
            callback = NULL;
            if (extent_t<void>::load(owner->em, owner->file, extent_offset, owner->amount_in_current_extent,
                    &owner->current_extent, this)) {
                return loaded_superblock();
            } else {
                callback = cb;
                return false;
            }
        }
        
        void on_load_extent() {
            loaded_superblock();
        }
        
        bool loaded_superblock() {
            
            lba_superblock_t *sb = (lba_superblock_t *)(
                (char*)(owner->current_extent->data()) + (offset - owner->current_extent->offset)
                );
            
            outstanding_cbs = 0;
            for (int i = 0; i < count; i ++) {
                lba_disk_extent_t *x;
                if (!lba_disk_extent_t::load(owner->em, owner->file, sb->entries[i].offset, sb->entries[i].lba_entries_count, &x, this))
                    outstanding_cbs ++;
                owner->extents.push_back(x);
            }
            
            if (outstanding_cbs == 0) {
                return finish();
            } else {
                return false;
            }
        }
        
        void on_load_lba_extent() {
            outstanding_cbs --;
            if (outstanding_cbs == 0) finish();
        }
        
        bool finish() {
            if (callback) callback->on_load_lba_superblock();
            delete this;
            return true;
        }
    };

public:
    static bool load(extent_manager_t *em, direct_file_t *file, off64_t offset, int count, lba_disk_superblock_t **out, load_callback_t *cb) {
        lba_disk_superblock_t *s = new lba_disk_superblock_t();
        s->em = em;
        s->file = file;
        s->modified = false;
        
        loader_t *loader = new loader_t(s, offset, count);
        bool done = loader->load_superblock(cb);
        
        *out = s;
        return done;
    }

private:
    extent_manager_t *em;
    direct_file_t *file;
    
    extent_t<void> *current_extent;
    size_t amount_in_current_extent;
    
public:
    intrusive_list_t<lba_disk_extent_t> extents;
    bool modified;
    off64_t offset;
    
    bool sync(sync_callback_t *cb) {
        
        if (!modified) return true;
        modified = false;
        
        /* Start a new extent if necessary */
        
        if (amount_in_current_extent + lba_superblock_t::entry_count_to_file_size(extents.size()) >
            em->extent_size) {
            
            current_extent->destroy();
            
            extent_t<void>::create(em, file, &current_extent);
            amount_in_current_extent = 0;
        }
        
        /* Write the superblock to the extent */
        
        offset = current_extent->offset + amount_in_current_extent;
        lba_superblock_t *sb = (lba_superblock_t*)((char*)current_extent->data() + amount_in_current_extent);
        memcpy(sb->magic, lba_super_magic, LBA_SUPER_MAGIC_SIZE);
        int i = 0;
        for (lba_disk_extent_t *e = extents.head(); e; e = extents.next(e)) {
            sb->entries[i].offset = e->offset;
            sb->entries[i].lba_entries_count = e->count;
            i++;
        }
        
        /* Write the extent to disk */
        
        amount_in_current_extent = ceil_aligned(
            amount_in_current_extent + lba_superblock_t::entry_count_to_file_size(extents.size()),
            DEVICE_BLOCK_SIZE);
        return current_extent->sync(amount_in_current_extent, cb);
    }

public:
    void destroy() {
        current_extent->destroy();
        while (lba_disk_extent_t *e = extents.head()) {
            extents.remove(e);
            e->destroy();
        }
        delete this;
    }
    
    void shutdown() {
        current_extent->shutdown();
        while (lba_disk_extent_t *e = extents.head()) {
            extents.remove(e);
            e->shutdown();
        }
        delete this;
    }

private:
    /* Use create(), load(), destroy(), and shutdown() instead */
    lba_disk_superblock_t() {}
    ~lba_disk_superblock_t() {}
};

#endif /* __SERIALIZER_LOG_LBA_DISK_SUPERBLOCK__ */
