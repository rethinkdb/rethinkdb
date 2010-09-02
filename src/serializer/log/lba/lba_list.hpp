

#ifndef __SERIALIZER_LOG_LBA_LIST_HPP__
#define __SERIALIZER_LOG_LBA_LIST_HPP__

#include "arch/resource.hpp"
#include "arch/io.hpp"
#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"
#include "../extents/extent_manager.hpp"
#include "containers/priority_queue.hpp"
#include <functional>
#include <bitset>

// Used internally by lba_list_t
struct lba_extent_buf_t;
struct lba_superblock_buf_t;
struct lba_write_t;
struct lba_start_fsm_t;

#define LBA_MAGIC_SIZE 8
#define LBA_SUPER_MAGIC_SIZE 8

class lba_list_t
{
public:
    struct metablock_mixin_t;
    
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
    void shutdown();
    
public:
    /* gen_block_id() will return a block ID which is "in limbo". It is not considered to be
    "in use" until it is actually written to disk, but gen_block_id() will not return the same
    ID again until either it is written to disk and then deleted or the database is restarted. */
    block_id_t gen_block_id();

    off64_t get_block_offset(block_id_t block);
    
    /* To store offsets in the file:
    
    1. Call set_block_offset() or delete_block() one or more times
    2. Call sync()
    3. Before calling set_block_offset() again, call prepare_metablock()
    4. Only after sync() returns true or calls its callback, write the prepared metablock
    
    Note that sync() will return true immediately if there is nothing new to write to the file since
    the last sync.
    */

    void set_block_offset(block_id_t block, off64_t offset);
    void delete_block(block_id_t block);

public:
    void prepare_metablock(metablock_mixin_t *metablock);
    struct sync_callback_t : public intrusive_list_node_t<sync_callback_t> {
        virtual void on_lba_sync() = 0;
    };
    bool sync(sync_callback_t *cb);


public:
    struct metablock_mixin_t {
        /* Reference to the last lba extent (that's currently being
         * written to). Once the extent is filled, the reference is
         * moved to the lba superblock, and the next block gets a
         * reference to the clean extent. */
        off64_t last_lba_extent_offset;
        int last_lba_extent_entries_count;

        /* Reference to the LBA superblock and its size */
        off64_t lba_superblock_offset;
        int lba_superblock_entries_count;
    };

private:
    friend class lba_buf_t;
    friend class lba_extent_buf_t;
    friend class lba_superblock_buf_t;
    friend class lba_write_t;
    friend class lba_start_fsm_t;
    
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
    
    lba_extent_buf_t *current_extent;
    lba_superblock_buf_t *superblock_extent;
    
    off64_t lba_superblock_offset;
    int lba_superblock_entry_count;

    // These are only meaningful if current_extent is NULL
    off64_t last_extent_offset;
    int last_extent_entry_count;
    
    lba_write_t *last_write;
    block_id_t next_free_id;

private:
    /* This is the on-disk format of an LBA extent */
    struct lba_entry_t {
        block_id_t block_id;
        off64_t offset;   // Is either an offset into the file or DELETE_BLOCK
    };
    struct lba_extent_t {
        // Header needs to be padded to a multiple of sizeof(lba_entry_t)
        char magic[LBA_MAGIC_SIZE];
        char padding[sizeof(lba_entry_t) - sizeof(magic) % sizeof(lba_entry_t)];
        lba_entry_t entries[0];
    };

    /* This is the on disk format of the LBA superblock */
    struct lba_superblock_entry_t {
        off64_t offset;
        int lba_entries_count;
    };
    
    struct lba_superblock_t {
        // Header needs to be padded to a multiple of sizeof(lba_superblock_entry_t)
        char magic[LBA_SUPER_MAGIC_SIZE];
        char padding[sizeof(lba_superblock_entry_t) - sizeof(magic) % sizeof(lba_superblock_entry_t)];

        /* The superblock contains references to all the extents
         * except the last. The reference to the last extent is
         * maintained in the metablock. This is done in order to be
         * able to store the number of entries in the last extent as
         * it's being filled up without rewriting the superblock. */
        lba_superblock_entry_t entries[0];

        static int entry_count_to_file_size(int nentries) {
            return sizeof(lba_superblock_entry_t) * nentries + offsetof(lba_superblock_t, entries[0]);
        }
    };

private:
    void make_entry_in_extent(block_id_t block, off64_t offset);
    void finalize_current_extent();

private:
    //class gc_array;
    //class gc_pq;
    //typedef std::bitset<(EXTENT_SIZE - sizeof(lba_header_t)) / sizeof(lba_entry_t)> gc_array;
    //typedef priority_queue_t<gc_array, std::less<gc_array> > gc_pq;
};

#endif /* __SERIALIZER_LOG_LBA_LIST_HPP__ */
