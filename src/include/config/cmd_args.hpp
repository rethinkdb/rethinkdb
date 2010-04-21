
#ifndef __CMD_ARGS_HPP__
#define __CMD_ARGS_HPP__

#include "config/args.hpp"

struct cmd_config_t {
    int max_cores;
    char db_file_name[MAX_DB_FILE_NAME];
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config);

#endif // __CMD_ARGS_HPP__

