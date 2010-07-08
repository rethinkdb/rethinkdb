
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
    
    // flush_interval_ms is the time that the writeback component waits between flushing data to the
    // disk. It can be NEVER_FLUSH. In theory it can also be 0, to indicate that data should be
    // flushed whenever a write occurs, but as of 2010-06-29 this is broken.
    int flush_interval_ms;
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config);

#endif // __CMD_ARGS_HPP__

