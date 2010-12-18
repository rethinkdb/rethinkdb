#include "diskinfo.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>

#include "logger.hpp"
#include "config/cmd_args.hpp"

const char *partitions_path = "/proc/partitions";
const char *hdparm_path = "/sbin/hdparm";

struct partition_info_t {
    partition_info_t();
    partition_info_t(int nmajor, int nminor, int nblocks, std::string name);
    int nmajor;
    int nminor;
    int nblocks;
    std::string name;
};

void get_partition_map(std::vector<partition_info_t> &partitions);


partition_info_t::partition_info_t(int nmajor, int nminor, int nblocks, std::string name)
    : nmajor(nmajor), nminor(nminor), nblocks(nblocks), name(name) { }

partition_info_t::partition_info_t() {}

void tokenize_line(const std::string& line, std::vector<std::string> &tokens) {
    /* Given a line and an empty vector to fill in, chops the line up by space
     * characters and puts the words in the tokens vector.
     */
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

void get_partition_map(std::vector<partition_info_t *> &partitions) {
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
    std::ifstream partitions_file (partitions_path);
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
            partition->nmajor = strtol(tokens[0].c_str(), &endptr, 16);
            partition->nminor = strtol(tokens[1].c_str(), &endptr, 16);
            partition->nblocks = strtol(tokens[2].c_str(), &endptr, 10);
            partition->name = tokens[3];
            partition->name.insert(0, std::string("/dev/"));

            partitions.push_back(partition);
        }
    }
}

void log_disk_info(std::vector<log_serializer_private_dynamic_config_t> &serializers) {
    std::vector<partition_info_t *> partitions;
    std::set<std::string> devices;

    logINF("Disk info for database disks:\n");

    struct stat st;

    if (stat(hdparm_path, &st)) {
        logWRN("System lacks hdparm; giving up\n");
        return;
    }

    get_partition_map(partitions);

    int maj, min = 0;
    const char *path;
    const char *dev_path;

    uid_t uid = geteuid();
    int device_idx;

    mlog_start(INF);
    for (unsigned int i = 0; i < serializers.size(); i++) {
        path = serializers[i].db_filename.c_str();
        if (stat(path, &st)) {
            mlogf("Cannot stat \"%s\": %s\n", path, strerror(errno));
            continue;
        }
        maj = major(st.st_dev);
        min = minor(st.st_dev);
        if (maj == 0) {
            /* We're looking at a device, not a file */
            if (strncmp("/dev/ram", path, 8)) {
                devices.insert(path);
            }
            else {
                /* Don't give ramdisks to hdparm because it won't produce
                 * anything useful. */
                mlogf("\n%s:\n    RAM disk\n", path);
            }
            continue;
        }

        device_idx = -1;
        for (unsigned int j = 0; j < partitions.size(); j++) {
            if (partitions[j]->nmajor == maj && partitions[j]->nminor == min) {
                device_idx = j;
                break;
            }
        }

        if (device_idx != -1) {
            dev_path = partitions[device_idx]->name.c_str();
            if (stat(dev_path, &st)) {
                mlogf("Cannot stat \"%s\": %s\n", dev_path, strerror(errno));
                continue;
            }
            if (S_ISDIR(st.st_mode) || S_ISFIFO(st.st_mode)) {
                mlogf("Invalid database file: \"%s\"\n", dev_path);
                continue;
            }
            if (-1 == access(dev_path, R_OK | W_OK) && 0 != uid) {
                mlogf("Must be root or have write permissions on \"%s\" to get disk info.\n", dev_path);
                continue;
            }

            devices.insert(partitions[device_idx]->name);
        }

    }
    for (unsigned int i = 0; i < partitions.size(); i++) {
        delete partitions[i];
    }
    partitions.clear();


    if (devices.size() < 1) {
        /* Don't run hdparm unless we have devices to run it on. */
        mlog_end();
        return;
    }

    std::string cmd;
    for (std::set<std::string>::iterator it = devices.begin(); it != devices.end(); it++) {
        cmd.append(std::string(hdparm_path));
        cmd.append(std::string(" -iI "));
        cmd.append(*it);
        /* Standard error from hdparm isn't useful. */
        cmd.append(std::string(" 2>/dev/null; "));
    }

    printf("%s\n", cmd.c_str());


    char *buf = (char *) calloc(1024, sizeof(char));
    FILE *stream;
    size_t total_bytes = 0;
    size_t nbytes = 0;

    if (NULL == buf) {
        crash("out of memory\n");
    }
    if (NULL == (stream = popen(cmd.c_str(), "r"))) {
        crash("popen failed\n");
    }

    while (1023 == (nbytes = fread(buf, 1, 1023, stream)))
    {
        total_bytes += nbytes;
        mlogf("%s", buf);
    }
    if (-1 == pclose(stream)) {
        mlog_end();
        crash("pclose failed\n");
    }
    total_bytes += nbytes;
    buf[nbytes] = '\0';
    if (total_bytes < 20 * devices.size()) {
        /* No output. We didn't have enough permissions to run hdparm.*/
        mlogf("None of the supplied disks support disk info queries.\n");
    }
    else {
        mlogf("%s\n", buf);
    }
    mlog_end();

    free(buf);
}
