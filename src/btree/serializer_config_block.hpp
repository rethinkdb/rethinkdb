#ifndef __BTREE_SERIALIZER_CONFIG_BLOCK_HPP__
#define __BTREE_SERIALIZER_CONFIG_BLOCK_HPP__

#include <stdint.h>
#include "buffer_cache/types.hpp"
#include "server/cmd_args.hpp"

#define CONFIG_BLOCK_ID (config_block_id_t::make(0))
/* This is the format that block ID 0 (CONFIG_BLOCK_ID) on each
   serializer takes. */

typedef uint32_t database_magic_t;

struct serializer_config_block_t {
    block_magic_t magic;
    
    /* What time the database was created. To help catch the case where files from two
    databases are mixed. */
    database_magic_t database_magic;
    
    /* How many serializers the database is using (in case user creates the database with
    some number of serializers and then specifies less than that many on a subsequent
    run) */
    int32_t n_files;
    
    /* Which serializer this is, in case user specifies serializers in a different order from
    run to run */
    int32_t this_serializer;
    
    /* Static btree configuration information, like number of slices. Should be the same on
    each serializer. */
    btree_config_t btree_config;

    static const block_magic_t expected_magic;
};






#endif  // __BTREE_SERIALIZER_CONFIG_BLOCK_HPP__
