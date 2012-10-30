// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MIGRATE_MIGRATE_HPP_
#define MIGRATE_MIGRATE_HPP_

#include <vector>
#include <string>

#include "utils.hpp"

#define TEMP_MIGRATION_FILE "migration-db-dump"

namespace migrate {

struct config_t {
    std::vector<std::string> input_filenames;
    std::vector<std::string> output_filenames;
    std::string exec_name;
    std::string intermediate_file;
    int force;
    config_t()
        : intermediate_file(TEMP_MIGRATION_FILE), force(false)
    { }
};

void usage(const char *name);

} //namespace migrate


int run_migrate(int argc, char **argv);

#endif  // MIGRATE_MIGRATE_HPP_
