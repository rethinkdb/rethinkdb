
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__

#include "containers/segmented_vector.hpp"
#include "disk_structure.hpp"
#include "config/code.hpp"

struct in_memory_index_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, in_memory_index_t>
{
    
    enum block_state_t {
        // block_state_t is limited to two bits in block_info_t
        block_not_found = 0,  // we initialize the state as not found
        block_unused    = 1,  // we mark a block as unused if we see it's deleted or don't find it at all
        block_in_limbo  = 2,  // block is in limbo if someone asked for it but didn't set its data block yet
        block_used      = 3   // block is used when it is used :)
    };
    
    struct block_info_t {
    public:
        block_info_t()
            : state(block_not_found)
            {}
        
        block_state_t get_state() {
            return state;
        }
        void set_state(block_state_t _state) {
            state = _state;
        }

        off64_t get_offset() {
            assert(get_state() == block_used);
            return offset;
        }
        void set_offset(off64_t _offset) {
            assert(get_state() == block_used);
            offset = _offset;
        }

        block_id_t get_next_free_id() {
            assert(get_state() == block_unused);
            // We need to do this because NULL_BLOCK_ID is -1,
            // block_id_t is uint, and _id and next_free_id have
            // different number of bits
            struct temp_t {
                block_id_t id : 62;
            };
            temp_t temp;
            temp.id = 0;
            temp.id--;
            if(next_free_id == temp.id)
                return NULL_BLOCK_ID;
            else
                return next_free_id;
        }
        void set_next_free_id(block_id_t _id) {
            assert(get_state() == block_unused);
            // We need to do this because NULL_BLOCK_ID is -1,
            // block_id_t is uint, and _id and next_free_id have
            // different number of bits
            if(_id == NULL_BLOCK_ID) {
                next_free_id = 0;
                next_free_id--;
            }
            else
                next_free_id = _id;
        }

    private:
        block_state_t state          : 2;
        union {
            off64_t offset           : 62;  // If state == block_used, the location of the block in the file
            block_id_t next_free_id  : 62;  // If state == block_unused, contains the id of the next free block
        };
    };
    
    segmented_vector_t<block_info_t, MAX_BLOCK_ID> blocks;
    block_id_t next_free_id;

public:
    in_memory_index_t() {
        /* This constructor is used when we are creating a new database. The only entry is the
        superblock. The reason why we put the superblock ID into limbo manually is because this is
        the best way to guarantee that block ID 0 is the superblock. */
        blocks.set_size(1);
        blocks[SUPERBLOCK_ID].set_state(block_in_limbo);
        next_free_id = NULL_BLOCK_ID;
    }
    
    in_memory_index_t(lba_disk_structure_t *s) {
        
        /* Call each lba_extent_t in reverse order */
        
        if (s->last_extent) fill_from_extent(s->last_extent);
        
        if (s->superblock) {
            lba_disk_extent_t *e = s->superblock->extents.tail();
            while (e) {
                fill_from_extent(e);
                e = s->superblock->extents.prev(e);
            }
        }
        
        // Now that we're finished loading the lba blocks, go
        // through the ids and create a freelist of unused ids so
        // that we can recycle and reuse them (this is necessary
        // because the system depends on the id space being
        // dense). We're doing another pass here, and it sucks,
        // but will do for first release. TODO: fix the lba system
        // to avoid O(n) algorithms on startup.
        next_free_id = NULL_BLOCK_ID;
        for (block_id_t id = 0; id < blocks.get_size(); id ++) {
            // Remap blocks that weren't found to unused
            if (blocks[id].get_state() == block_not_found)
                blocks[id].set_state(block_unused);
            // Add unused blocks to free list
            if (blocks[id].get_state() == block_unused) {
                // Add the unused block to the freelist
                blocks[id].set_next_free_id(next_free_id);
                next_free_id = id;
            }
        }
    }
    
    void fill_from_extent(lba_disk_extent_t *x) {
    
        for (int i = x->count - 1; i >= 0; i --) {
            
            lba_entry_t *entry = &x->data->data()->entries[i];
        
            // Skip it if it's padding
            if (entry->block_id == PADDING_BLOCK_ID && entry->offset == PADDING_OFFSET)
                continue;
            assert(entry->block_id != PADDING_BLOCK_ID);
        
            // Sanity check. If this assertion fails, it probably means that the file was
            // corrupted, or was created with a different btree block size.
            assert(entry->offset == DELETE_BLOCK || entry->offset % DEVICE_BLOCK_SIZE == 0);

            // If it's an entry for a block we haven't seen before, then record its information
            if(entry->block_id >= blocks.get_size())
                blocks.set_size(entry->block_id + 1);
        
            block_info_t *binfo = &blocks[entry->block_id];
            if (binfo->get_state() == block_not_found) {
                if (entry->offset == DELETE_BLOCK) {
                    binfo->set_state(block_unused);
                } else {
                    binfo->set_state(block_used);
                    binfo->set_offset(entry->offset);
                }
            }
        }
    }
    
    block_id_t gen_block_id(void) {
        if(next_free_id == NULL_BLOCK_ID) {
            // There is no recyclable block id
            block_id_t id = blocks.get_size();
            blocks.set_size(blocks.get_size() + 1);
            blocks[id].set_state(block_in_limbo);
            return id;
        } else {
            // Grab a recyclable id from the freelist
            block_id_t id = next_free_id;
            assert(blocks[id].get_state() == block_unused);
            next_free_id = blocks[id].get_next_free_id();
            blocks[id].set_state(block_in_limbo);
            return id;
        }
    }
    
    off64_t get_block_offset(block_id_t id) {
        assert(blocks[id].get_state() == block_used);
        return blocks[id].get_offset();
    }
    
    void set_block_offset(block_id_t id, off64_t offset) {
    
        // Blocks must be returned by gen_block_id() before they are legal to use
        assert(blocks[id].get_state() != block_unused);
        
        blocks[id].set_state(block_used);
        blocks[id].set_offset(offset);
    }
    
    void delete_block(block_id_t id) {
    
        assert(blocks[id].get_state() != block_unused);
        blocks[id].set_state(block_unused);
        
        // Add the unused block to the freelist
        blocks[id].set_next_free_id(next_free_id);
        next_free_id = id;
    }
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

