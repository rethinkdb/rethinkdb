#include "diskinfo.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>

#include "logger.hpp"
#include "config/cmd_args.hpp"

const char *partitions_path = "/proc/partitions";

partition_info_t::partition_info_t(int nmajor, int nminor, int nblocks, std::string name)
    : nmajor(nmajor), nminor(nminor), nblocks(nblocks), name(name) { }

partition_info_t::partition_info_t() {}

dev_t get_device(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st)) {
        logDBG("%s\n", strerror(errno));
    }
    return st.st_dev;
}

void tokenize_line(const std::string& line, std::vector<std::string> &tokens) {
    size_t start, end = 0;
    while (end < line.size()) {
        start = end;
        while (start < line.size() && line[start] == ' ') {
            start++;
        }
        end = start;
        while (end < line.size() && line[end] != ' ') {
            end++;
        }
        if (end - start > 0) {
            tokens.push_back(std::string(line, start, end - start));
        }
    }
}

void get_partition_map(std::vector<partition_info_t> &partitions) {
    /* /proc partitions looks like this:
     * major minor  #blocks  name
     *
     *   8        0 234234234 sda
     *   8        1 123948586 sda1
     *   ...
     *
     * The major and minor device numbers are hexadecimal numbers.
     * To parse we skip the first two lines and then chop up each line into
     * partition_info_t structs.
     *
     */
    std::ifstream partitions_file ("/proc/partitions");
    std::string line;
    std::vector<std::string> tokens;
    char *endptr;
    if (partitions_file.is_open()) {
        getline(partitions_file, line);
        getline(partitions_file, line);
        while (partitions_file.good()) {
            getline(partitions_file, line);
            tokens.clear();
            tokenize_line(line, tokens);
            if (tokens.size() != 4) {
                continue;
            }

            partition_info_t *partition = new partition_info_t;
            partition->nmajor = strtol(tokens[0].c_str(), &endptr, 10);
            partition->nminor = strtol(tokens[1].c_str(), &endptr, 10);
            partition->nblocks = strtol(tokens[2].c_str(), &endptr, 10);
            partition->name = tokens[3];
            partition->name.insert(0, std::string("/dev/"));

            //logDBG("maj%d min%d blk%d nm%s\n", partition->nmajor, partition->nminor, partition->nblocks, partition->name.c_str());


            partitions.push_back(*partition);

        }
    }
}

void log_disk_info(std::vector<log_serializer_private_dynamic_config_t> &serializers) {
    std::vector<partition_info_t> partitions;
    std::set<std::string> devices;
    get_partition_map(partitions);

    int maj, min = 0;
    const char *path;
    for (unsigned int i = 0; i < serializers.size(); i++) {
        path = serializers[i].db_filename.c_str();
        dev_t device = get_device(path);
        maj = major(device);
        min = minor(device);
        if (maj == 0) {
            /* We're looking at a device, not a file */
            devices.insert(path);
            continue;
        }

        for (unsigned int j = 0; j < partitions.size(); j++) {
            if (partitions[j].nmajor == maj && partitions[j].nminor == min) {
                devices.insert(partitions[j].name);
            }
        }
    }

    std::string cmd = std::string("hdparm -I");
    for (std::set<std::string>::iterator it = devices.begin(); it != devices.end(); it++) {
        logDBG("%s\n", (*it).c_str());
        cmd.append(std::string(" "));
        cmd.append(*it);
    }


    logDBG("%s\n", cmd.c_str());



    FILE *stream = popen(cmd.c_str(), "r";



}
