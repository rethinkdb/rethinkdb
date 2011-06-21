#ifndef __IMPORT_IMPORT_HPP__
#define __IMPORT_IMPORT_HPP__

#include <string>
#include "server/cmd_args.hpp"

namespace import {
struct cmd_config_t {
    cmd_config_t(); // Automatically initializes to default values

    int n_workers;
    
    char log_file_name[MAX_LOG_FILE_NAME];
    // Log messages below this level aren't printed
    //log_level min_log_level;
    
    // Configuration information for the btree
    btree_key_value_store_dynamic_config_t store_dynamic_config;
    btree_key_value_store_static_config_t store_static_config;
    bool create_store, force_create;

    bool verbose;

    //file containing memcache commands
    std::string memcached_file;
};

} //namespace import
#endif
