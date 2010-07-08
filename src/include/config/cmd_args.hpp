
#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include "config/args.hpp"

#define NEVER_FLUSH -1

struct cmd_config_t {
    int port;
    int max_cores;
    char db_file_name[MAX_DB_FILE_NAME];
    size_t max_cache_size;
    
    // If wait_for_flush is true, then write operations will not return until after the data is
    // safely sitting on the disk.
    bool wait_for_flush;
    
    // safety_timer_ms is how long the cache will allow modified data to sit in memory before
    // flushing it to disk. If it is NEVER_FLUSH, then data will be allowed to sit in memory
    // indefinitely.
    int safety_timer_ms;
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config);

#endif // __CMD_ARGS_HPP__

