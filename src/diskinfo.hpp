#ifndef __DISKINFO_HPP__
#define __DISKINFO_HPP__
#include <string>
#include <vector>
#include "server/cmd_args.hpp"

/* Logs hdparm information about the disks that the serializers' files reside on. */
void log_disk_info(std::vector<log_serializer_private_dynamic_config_t> const &serializers);

#endif
