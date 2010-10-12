
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__

#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"
#include "disk_structure.hpp"
#include "config/alloc.hpp"
#include "serializer/log/data_block_manager.hpp"

struct in_memory_index_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, in_memory_index_t>
{
    segmented_vector_t<off64_t, MAX_BLOCK_ID> blocks;

public:
    in_memory_index_t() {
        /* This constructor is used when we are creating a new database. Initially no blocks
        exist. */
    }
    
    in_memory_index_t(lba_disk_structure_t *s, data_block_manager_t *dbm, extent_manager_t *em) {
        
        /* Call each lba_extent_t in the same order they were written */
        
        if (s->superblock) {
            for (lba_disk_extent_t *e = s->superblock->extents.head();
                 e;
                 e = s->superblock->extents.next(e)) {
                fill_from_extent(e);
            }
        }
        if (s->last_extent) fill_from_extent(s->last_extent);
        
        // Inform the DBM which blocks are in use
        dbm->start_reconstruct();
        for (ser_block_id_t id = 0; id < blocks.get_size(); id ++) {
        
            // The segmented_vector_t will initialize its cells to 0. For this to
            // be zero probably means we didn't fill it in with anything for some
            // reason.
            assert(blocks[id] != 0);
            
            if (blocks[id] != DELETE_BLOCK) {
                dbm->mark_live(blocks[id]);
            }
        }
        end_reconstruct(dbm, em, s);
    }

private:
    void end_reconstruct(data_block_manager_t *dbm, extent_manager_t *em, lba_disk_structure_t *s) {
        // Go through all extents and release each one if it isn't in
        // use.
        for (unsigned int extent_id = 0;
             (extent_id * em->extent_size) < (unsigned int) em->max_extent();
             extent_id++)
        {
            if (!is_extent_in_use(dbm, em, s, extent_id))
                em->release_extent(extent_id * em->extent_size);
        }
        dbm->end_reconstruct();
    }

    bool is_extent_in_use(data_block_manager_t *dbm, extent_manager_t *em, lba_disk_structure_t *s, unsigned int extent_id) {
        off64_t extent_offset = extent_id * em->extent_size;
        return
            em->is_reserved(extent_offset) ||
            dbm->is_extent_in_use(extent_id) ||
            is_extent_in_use_by_lba(em, s, extent_id);
    }

#ifndef NDEBUG
public:
    bool is_extent_referenced(unsigned int extent_id) {
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            if (blocks[i] / EXTENT_SIZE == extent_id) {
                printf("Referenced with block_id: %d\n", i);
                return true;
            }
        }
        return false;
    }

    int extent_refcount(unsigned int extent_id) {
        int refcount = 0;
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            if (blocks[i] / EXTENT_SIZE == extent_id) {
                printf("refed by block id: %d\n", i);
                refcount++;
            }
        }
        return refcount;
    }

    bool is_offset_referenced(off64_t offset) {
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            if (blocks[i] == offset)
                return true;
        }
        return false;
    }

    bool is_offset_referenced(ser_block_id_t block_id, off64_t offset) {
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            if (blocks[i] == offset && i != block_id)
                return true;
        }
        return false;
    }

    bool have_duplicates() {
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            if (blocks[i] == DELETE_BLOCK) continue;
            for (unsigned int j = i + 1; j < blocks.get_size(); j++) {
                if (blocks[j] == blocks[i])
                    return true;
            }
        }
        return false;
    }
#endif

    bool is_extent_in_use_by_lba(extent_manager_t *em, lba_disk_structure_t *s, unsigned int extent_id) {
        off64_t extent_offset = extent_id * em->extent_size;
        // First check if it's the last extent
        if (s->last_extent && s->last_extent->offset == extent_offset)
            return true;
        // If we have a superblock, gotta check if extent is
        // superblock or one of the other extents stored in the
        // superblock
        if (s->superblock != NULL) {
            // TODO: this is shit. Superblock offset may be in the
            // middle of the extent, and we round it up here to the
            // extent boundary. When we load superblocks in
            // disk_superblock.hpp we do the same thing. We should do
            // it one place (i.e. superblock_t::get_extent_id()).
            off64_t sb_offset = s->superblock->offset;
            sb_offset = sb_offset - sb_offset % em->extent_size;
            if(sb_offset == extent_offset)
                return true;

            // TODO: This is O(N^2) because we check if every extent
            // is one of LBA extents - no good, fix it!
            for (intrusive_list_t<lba_disk_extent_t>::iterator it = s->superblock->extents.begin();
                 it != s->superblock->extents.end();
                 it++)
            {
                assert((*it).offset % em->extent_size == 0);
                if((*it).offset == extent_offset)
                    return true;
            }
        }
        return false;
    }

public:
    void fill_from_extent(lba_disk_extent_t *x) {
    
        for (int i = 0; i < x->count; i ++) {
            
            lba_entry_t *entry = &x->data->data()->entries[i];
        
            // Skip it if it's padding
            if (entry->block_id == PADDING_BLOCK_ID && entry->offset == PADDING_OFFSET)
                continue;
            assert(entry->block_id != PADDING_BLOCK_ID);
        
            // Sanity check. If this assertion fails, it probably means that the file was
            // corrupted, or was created with a different device block size.
            assert(entry->offset == DELETE_BLOCK || entry->offset % DEVICE_BLOCK_SIZE == 0);

            // Grow the array if necessary
            if (entry->block_id >= blocks.get_size()) {
                ser_block_id_t old_size = blocks.get_size();
                blocks.set_size(entry->block_id + 1);
                for (ser_block_id_t i = old_size; i < entry->block_id; i++) {
                    blocks[i] = DELETE_BLOCK;
                }
            }
            
            // Write the entry
            blocks[entry->block_id] = entry->offset;
        }
    }
    
    ser_block_id_t max_block_id() {
        return blocks.get_size();
    }
    
    off64_t get_block_offset(ser_block_id_t id) {
        if(id >= blocks.get_size()) {
            return DELETE_BLOCK;
        } else {
            return blocks[id];
        }
    }
    
    void set_block_offset(ser_block_id_t id, off64_t offset) {
        
        /* Grow the array if necessary, and fill in the empty space with DELETE_BLOCK */
        if (id >= blocks.get_size()) {
            int old_size = blocks.get_size();
            blocks.set_size(id + 1);
            for (unsigned i = old_size; i < blocks.get_size(); i++) {
                blocks[i] = DELETE_BLOCK;
            }
        }
        
        blocks[id] = offset;
    }
    
    void print() {
#ifndef NDEBUG
        printf("LBA:\n");
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            printf("%d %.8lx\n", i, (unsigned long int)blocks[i]);
        }
#endif
    }
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

