
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__

#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"

struct in_memory_index_t
{
    // blocks.get_size() == timestamps.get_size().  We use parallel
    // arrays to avoid wasting memory from alignment.
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;
    segmented_vector_t<repl_timestamp, MAX_BLOCK_ID> timestamps;

public:
    in_memory_index_t() { }

    // TODO: Rename this function.  It's one greater than the max
    // block id.
    ser_block_id_t max_block_id() {
        return ser_block_id_t::make(blocks.get_size());
    }

    struct info_t {
        flagged_off64_t offset;
        repl_timestamp recency;
    };

    info_t get_block_info(ser_block_id_t id) {
        if(id >= blocks.get_size()) {
            info_t ret = { flagged_off64_t::unused(), repl_timestamp::invalid };
            return ret;
        } else {
            info_t ret = { blocks[id.value], timestamps[id.value] };
            return ret;
        }
    }

    void set_block_info(ser_block_id_t id, repl_timestamp recency,
                        flagged_off64_t offset) {

        if (id.value >= blocks.get_size()) {
            blocks.set_size(id.value + 1, flagged_off64_t::unused());
            timestamps.set_size(id.value + 1, repl_timestamp::invalid);
        }

        blocks[id.value] = offset;
        timestamps[id.value] = recency;
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

