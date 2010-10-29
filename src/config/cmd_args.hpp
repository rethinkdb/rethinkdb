
#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include "config/args.hpp"
#include <sys/types.h>

#define NEVER_FLUSH -1

/* Configuration for the serializer that can change from run to run */

struct log_serializer_dynamic_config_t {

    /* When the proportion of garbage blocks hits gc_high_ratio, then the serializer will collect
    garbage until it reaches gc_low_ratio. */
    float gc_low_ratio, gc_high_ratio;
    
    /* How many data block extents the serializer will be writing to at once */
    unsigned num_active_data_extents;

    /* If file_size is nonzero and the serializer is not running on a block device, then it will
    pretend to be running on a block device by immediately resizing the file to file_size and then
    zoning it like a block device. */
    size_t file_size;
    
    /* How big to make each zone if the database is on a block device or if file_size is given */
    size_t file_zone_size;
};

/* Configuration for the serializer that is set when the database is created */

struct log_serializer_static_config_t {
    
    size_t block_size;
    size_t extent_size;
};

/* Configuration for the cache (it can all change from run to run) */

struct mirrored_cache_config_t {
    
    // Max amount of memory that will be used for the cache, in bytes.
    long long max_size;
    
    // If wait_for_flush is true, then write operations will not return until after the data is
    // safely sitting on the disk.
    bool wait_for_flush;
    
    // flush_timer_ms is how long (in milliseconds) the cache will allow modified data to sit in
    // memory before flushing it to disk. If it is NEVER_FLUSH, then data will be allowed to sit in
    // memory indefinitely.
    int flush_timer_ms;
    
    // flush_threshold_percent is a number between 0 and 100. If the number of dirty blocks in a
    // buffer cache is more than (flush_threshold_percent / 100) of the maximum number of blocks
    // allowed in memory, then dirty blocks will be flushed to disk.
    int flush_threshold_percent;
};

/* Configuration for the btree that is set when the database is created and serialized in the
serializer */

struct btree_config_t {
    
    int n_slices;
};

/* Configuration for the store (btree, cache, and serializers) that can be changed from run
to run */

struct btree_key_value_store_dynamic_config_t {
    
    log_serializer_dynamic_config_t serializer;
    mirrored_cache_config_t cache;
};

/* Configuration for the store (btree, cache, and serializers) that is set at database
creation time */

struct btree_key_value_store_static_config_t {
    
    log_serializer_static_config_t serializer;
    btree_config_t btree;
};

/* All the configuration together */

struct cmd_config_t {
    int port;
    int n_workers;
    
    char log_file_name[MAX_LOG_FILE_NAME];
    // Log messages below this level aren't printed
    //log_level min_log_level;
    
    // Configuration information for the btree
    int n_files;
    const char *files[MAX_SERIALIZERS];
    btree_key_value_store_dynamic_config_t store_dynamic_config;
    bool create_store;
    btree_key_value_store_static_config_t store_static_config;
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config);

// TODO: remove this.
void init_config(cmd_config_t *config);

#endif // __CMD_ARGS_HPP__

