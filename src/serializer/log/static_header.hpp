
#ifndef __SERIALIZER_STATIC_HEADER_HPP__
#define __SERIALIZER_STATIC_HEADER_HPP__

#include "config/args.hpp"
#include <string.h>

#define BTREE_BLOCK_SIZE_MARKER "BTree Blocksize:"
#define EXTENT_SIZE_MARKER      "Extent Size:"

struct static_header {
    public:
        char software_name[sizeof(SOFTWARE_NAME_STRING)];
        char version[sizeof(VERSION_STRING)];
        char btree_block_size_marker[sizeof(BTREE_BLOCK_SIZE_MARKER)];
        uint64_t btree_block_size;
        char extent_size_marker[sizeof(EXTENT_SIZE_MARKER)];
        uint64_t extent_size;

        static_header(uint64_t _btree_block_size, uint64_t _extent_size) {
            mempcpy(software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));
            mempcpy(version, VERSION_STRING, sizeof(VERSION_STRING));
            memcpy(btree_block_size_marker, BTREE_BLOCK_SIZE_MARKER, sizeof(BTREE_BLOCK_SIZE_MARKER));
            btree_block_size = _btree_block_size;
            memcpy(extent_size_marker, EXTENT_SIZE_MARKER, sizeof(EXTENT_SIZE_MARKER));
            extent_size = _extent_size;
        }
}__attribute__((__packed__));

#endif
