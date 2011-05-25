
#ifndef __MIGRATE_MIGRATE_HPP_
#define __MIGRATE_MIGRATE_HPP_

#include "utils.hpp"

#define TEMP_MIGRATION_FILE "temp-file"

namespace migrate {

struct config_t {
    std::vector<std::string> input_filenames;
    std::vector<std::string> output_filenames;
    std::string exec_name;
    std::string intermediate_file;
    bool force;
    config_t()
        : intermediate_file(TEMP_MIGRATION_FILE), force(false)
    { }
};

void usage(const char *name);

} //namespace migrate


int run_migrate(int argc, char **argv);

#endif
