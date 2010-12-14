#ifndef __DISKINFO_HPP__
#define __DISKINFO_HPP__
#include <string>
#include <vector>
#include "config/cmd_args.hpp"

struct partition_info_t {
    partition_info_t();
    partition_info_t(int nmajor, int nminor, int nblocks, std::string name);
    int nmajor;
    int nminor;
    int nblocks;
    std::string name;
};

void get_partition_map(std::vector<partition_info_t> &partitions);

void log_disk_info(std::vector<log_serializer_private_dynamic_config_t> &serializers);

#endif
