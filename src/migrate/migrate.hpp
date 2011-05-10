
#ifndef __MIGRATE_MIGRATE_HPP_
#define __MIGRATE_MIGRATE_HPP_

#include "utils.hpp"

namespace migrate {

struct config_t {
    std::vector<std::string> input_filenames;
    std::string exec_name;
};

} //namespace migrate


int run_migrate(int argc, char **argv);

#endif
