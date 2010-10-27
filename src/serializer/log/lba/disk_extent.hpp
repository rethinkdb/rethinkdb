
#ifndef __SERIALIZER_LOG_LBA_DISK_EXTENT_H__
#define __SERIALIZER_LOG_LBA_DISK_EXTENT_H__

#include "containers/intrusive_list.hpp"
#include "arch/arch.hpp"
#include "extent.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"

class lba_disk_extent_t :
    public intrusive_list_node_t<lba_disk_extent_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_disk_extent_t>
{

public:
    struct load_callback_t {
        virtual void on_load_lba_extent() = 0;
    };
    struct sync_callback_t {
        virtual void on_sync_lba_extent() = 0;
    };

private:
    extent_manager_t *em;

public:
    extent_t<lba_extent_t> *data;
    off64_t offset;
    int count;

public:
    static void create(extent_manager_t *em, direct_file_t *file, lba_disk_extent_t **out) {
        lba_disk_extent_t *x = new lba_disk_extent_t();
        x->em = em;
        
        extent_t<lba_extent_t>::create(em, file, &x->data);
        x->offset = x->data->offset;
        
        // Make sure that the size of the header is a multiple of the size of one entry, so that the
        // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
        assert(offsetof(lba_extent_t, entries[0]) % sizeof(lba_entry_t) == 0);
        
        memcpy(x->data->data()->magic, lba_magic, LBA_MAGIC_SIZE);
        
        x->count = 0;
        *out = x;
    }
    
    struct loader_t :
        public extent_t<lba_extent_t>::load_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, loader_t>
    {
        lba_disk_extent_t::load_callback_t *callback;
        loader_t(lba_disk_extent_t::load_callback_t *callback) : callback(callback) {}
        void on_load_extent() {
            if (callback) callback->on_load_lba_extent();
            delete this;
        }
    };
    
    static bool load(extent_manager_t *em, direct_file_t *file, off64_t offset, int count, lba_disk_extent_t **out, load_callback_t *cb) {
        lba_disk_extent_t *x = new lba_disk_extent_t();
        x->em = em;
        x->count = count;
        x->offset = offset;
        
        loader_t *l = new loader_t(cb);
        bool done = extent_t<lba_extent_t>::load(em, file, x->offset, x->amount_filled(), &x->data, l);
        if (done) delete l;
        
        *out = x;
        return done;
    }

private:
    size_t amount_filled() {
        return offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t)*count;
    }

public:
    bool full() {
        return amount_filled() == em->extent_size;
    }
    
    // TODO: just pass this an lba_entry_t, eh?
    void add_entry(ser_block_id_t id, flagged_off64_t offset) {
        
        // Make sure that entries will align with DEVICE_BLOCK_SIZE
        assert(DEVICE_BLOCK_SIZE % sizeof(lba_entry_t) == 0);
        
        // Make sure that there is room
        assert(amount_filled() + sizeof(lba_entry_t) <= em->extent_size);
        
        data->data()->entries[count].block_id = id;
        data->data()->entries[count].offset = offset;
        count++;
    }
    
    struct syncer_t :
        public extent_t<lba_extent_t>::sync_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, syncer_t>
    {
        lba_disk_extent_t::sync_callback_t *callback;
        syncer_t(lba_disk_extent_t::sync_callback_t *callback) : callback(callback) {}
        void on_sync_extent() {
            if (callback) callback->on_sync_lba_extent();
            delete this;
        }
    };
    
    bool sync(sync_callback_t *cb) {
        
        while (amount_filled() % DEVICE_BLOCK_SIZE != 0) {
            data->data()->entries[count] = lba_entry_t::make_padding_entry();
            count++;
        }
        
        syncer_t *syncer = new syncer_t(cb);
        bool done = data->sync(amount_filled(), syncer);
        if (done) delete syncer;
        return done;
    }

public:
    void destroy() {
        data->destroy();
        delete this;
    }
    
    void shutdown() {
        data->shutdown();
        delete this;
    }

private:
    /* Use create(), load(), destroy(), and shutdown() instead */
    lba_disk_extent_t() {}
    ~lba_disk_extent_t() {}
};

#endif /* __SERIALIZER_LOG_LBA_DISK_EXTENT_H__ */
