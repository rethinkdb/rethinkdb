
#ifndef __SERIALIZER_LOG_LBA_DISK_EXTENT_H__
#define __SERIALIZER_LOG_LBA_DISK_EXTENT_H__

#include "containers/intrusive_list.hpp"
#include "arch/arch.hpp"
#include "extent.hpp"
#include "../extents/extent_manager.hpp"
#include "disk_format.hpp"
#include "in_memory_index.hpp"

class lba_disk_extent_t :
    public intrusive_list_node_t<lba_disk_extent_t>
{

private:
    extent_manager_t *em;

public:
    extent_t *data;
    off64_t offset;
    int count;

public:
    lba_disk_extent_t(extent_manager_t *em, direct_file_t *file)
        : em(em), data(new extent_t(em, file)), offset(data->offset), count(0)
    {
        // Make sure that the size of the header is a multiple of the size of one entry, so that the
        // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
        assert(offsetof(lba_extent_t, entries[0]) % sizeof(lba_entry_t) == 0);
        assert(offsetof(lba_extent_t, entries[0]) == sizeof(lba_extent_t::header_t));
        
        lba_extent_t::header_t header;
        bzero(&header, sizeof(header));
        memcpy(header.magic, lba_magic, LBA_MAGIC_SIZE);
        data->append(&header, sizeof(header));
    }
    
    lba_disk_extent_t(extent_manager_t *em, direct_file_t *file, off64_t offset, int count)
        : em(em), data(new extent_t(em, file, offset, offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t)*count)), offset(offset), count(count)
    {
    }

public:
    bool full() {
        return data->amount_filled == em->extent_size;
    }
    
    void add_entry(lba_entry_t entry) {
        
        // Make sure that entries will align with DEVICE_BLOCK_SIZE
        assert(DEVICE_BLOCK_SIZE % sizeof(lba_entry_t) == 0);
        
        // Make sure that there is room
        assert(data->amount_filled + sizeof(lba_entry_t) <= em->extent_size);
        
        data->append(&entry, sizeof(lba_entry_t));
        count++;
    }
    
    void sync(extent_t::sync_callback_t *cb) {
        
        while (data->amount_filled % DEVICE_BLOCK_SIZE != 0) {
            add_entry(lba_entry_t::make_padding_entry());
        }
        
        data->sync(cb);
    }

public:
    /* To read from an LBA on disk, first call read_step_1(), passing it the address of a
    new read_info_t structure. When it calls the callback you provide, then call
    read_step_2() with the same read_info_t as before and with a pointer to the
    in_memory_index_t to be filled with data. */
    
    struct read_info_t {
        void *buffer;
        int count;
    };
    
    void read_step_1(read_info_t *info_out, extent_t::read_callback_t *cb) {
        
        info_out->buffer = malloc_aligned(em->extent_size, DEVICE_BLOCK_SIZE);
        info_out->count = count;
        data->read(0, sizeof(lba_extent_t) + sizeof(lba_entry_t) * count, info_out->buffer, cb);
    }
    
    void read_step_2(read_info_t *info, in_memory_index_t *index) {
        
        lba_extent_t *extent = (lba_extent_t *)info->buffer;
        assert(memcmp(extent->header.magic, lba_magic, LBA_MAGIC_SIZE) == 0);
        
        for (int i = 0; i < info->count; i++) {
            lba_entry_t *e = &extent->entries[i];
            if (!lba_entry_t::is_padding(e)) {
                index->set_block_offset(e->block_id, e->offset);
            }
        }
        
        free(info->buffer);
    }

public:
    /* destroy() deletes the structure in memory and also tells the extent manager that the extent
    can be safely reused */
    void destroy() {
        data->destroy();
        delete this;
    }
    
    /* shutdown() only deletes the structure in memory */
    void shutdown() {
        data->shutdown();
        delete this;
    }

private:
    /* Use destroy() or shutdown() instead */
    ~lba_disk_extent_t() {}
};

#endif /* __SERIALIZER_LOG_LBA_DISK_EXTENT_H__ */
