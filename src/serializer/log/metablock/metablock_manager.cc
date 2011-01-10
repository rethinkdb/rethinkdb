#include "serializer/log/metablock/metablock_manager.hpp"

void initialize_metablock_offsets(off64_t extent_size, std::vector<off64_t> *offsets) {
    offsets->clear();

    for (off64_t i = 0; i < MB_NEXTENTS; i++) {
        off64_t extent = i * extent_size * MB_EXTENT_SEPARATION;
    
        for (unsigned j = 0; j < extent_size / DEVICE_BLOCK_SIZE; j++) {
            off64_t offset = extent + j * DEVICE_BLOCK_SIZE;
            
            /* The very first DEVICE_BLOCK_SIZE of the file is used for the static header */
            if (offset == 0) continue;
            
            offsets->push_back(offset);
        }
    }
}
