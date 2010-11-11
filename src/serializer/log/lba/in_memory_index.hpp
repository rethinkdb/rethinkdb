
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__

#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"

struct in_memory_index_t
{
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;

public:
    in_memory_index_t() {
    }
    
    ser_block_id_t max_block_id() {
        return blocks.get_size();
    }
    
    flagged_off64_t get_block_offset(ser_block_id_t id) {
        if(id >= blocks.get_size()) {
            return flagged_off64_t::unused();
        } else {
            return blocks[id];
        }
    }
    
    void set_block_offset(ser_block_id_t id, flagged_off64_t offset) {
        
        /* Grow the array if necessary, and fill in the empty space with flagged_off64_t::unused(). */
        if (id >= blocks.get_size()) {
            blocks.set_size(id + 1, flagged_off64_t::unused());
        }
        
        blocks[id] = offset;
    }
    
    void print() {
#ifndef NDEBUG
        printf("LBA:\n");
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            printf("%d %.8lx\n", i, (unsigned long int)blocks[i].whole_value);
        }
#endif
    }
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

