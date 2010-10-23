
#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include "config/args.hpp"
#include <sys/types.h>

#define NEVER_FLUSH -1

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

struct log_serializer_static_config_t {
    size_t block_size;
    size_t extent_size;
};

struct cmd_config_t {
    int port;
    int n_workers;
    int n_slices;
    int n_serializers;
    char db_file_name[MAX_DB_FILE_NAME];
    long long max_cache_size;
    
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

    char log_file_name[MAX_LOG_FILE_NAME];
    // Log messages below this level aren't printed
    //log_level min_log_level;

    // Configuration information for the serializer
    log_serializer_dynamic_config_t ser_dynamic_config;
    log_serializer_static_config_t ser_static_config;
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config);

#endif // __CMD_ARGS_HPP__

