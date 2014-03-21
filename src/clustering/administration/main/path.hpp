#ifndef CLUSTERING_ADMINISTRATION_MAIN_PATH_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_PATH_HPP_

#include <string>
#include <vector>

struct path_t {
    std::vector<std::string> nodes;
    bool is_absolute;
};

path_t parse_as_path(const std::string &);
std::string render_as_path(const path_t &);

#endif  // CLUSTERING_ADMINISTRATION_MAIN_PATH_HPP_
