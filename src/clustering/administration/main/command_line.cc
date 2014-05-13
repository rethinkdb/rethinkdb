// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/main/command_line.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>

// Needed for determining rethinkdb binary path below
#if defined(__MACH__)
#include <mach-o/dyld.h>
#elif defined(__FreeBSD_version)
#include <sys/sysctl.h>
#endif

// Needed for determining available RAM for default cache size
#if defined(__MACH__)
#include <mach/host_info.h>
#include <mach/mach_host.h>
#endif

#include <functional>
#include <limits>

#include <re2/re2.h>

#include "arch/io/disk.hpp"
#include "arch/os_signal.hpp"
#include "arch/runtime/starter.hpp"
#include "extproc/extproc_spawner.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/main/names.hpp"
#include "clustering/administration/main/options.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/serve.hpp"
#include "clustering/administration/main/directory_lock.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/path.hpp"
#include "clustering/administration/persist.hpp"
#include "logger.hpp"

#define RETHINKDB_EXPORT_SCRIPT "rethinkdb-export"
#define RETHINKDB_IMPORT_SCRIPT "rethinkdb-import"
#define RETHINKDB_DUMP_SCRIPT "rethinkdb-dump"
#define RETHINKDB_RESTORE_SCRIPT "rethinkdb-restore"

MUST_USE bool numwrite(const char *path, int number) {
    // Try to figure out what this function does.
    FILE *fp1 = fopen(path, "w");
    if (fp1 == NULL) {
        return false;
    }
    fprintf(fp1, "%d", number);
    fclose(fp1);
    return true;
}

static std::string pid_file;

void remove_pid_file() {
    if (!pid_file.empty()) {
        remove(pid_file.c_str());
        pid_file.clear();
    }
}

int check_pid_file(const std::string &pid_filepath) {
    guarantee(pid_filepath.size() > 0);

    if (access(pid_filepath.c_str(), F_OK) == 0) {
        logERR("The pid-file specified already exists. This might mean that an instance is already running.");
        return EXIT_FAILURE;
    }

    // Make a copy of the filename since `dirname` may modify it
    char pid_dir[PATH_MAX + 1];
    strncpy(pid_dir, pid_filepath.c_str(), PATH_MAX + 1);
    if (access(dirname(pid_dir), W_OK) == -1) {
        logERR("Cannot access the pid-file directory.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int write_pid_file(const std::string &pid_filepath) {
    guarantee(pid_filepath.size() > 0);

    // Be very careful about modifying this. It is important that the code that removes the
    // pid-file only run if the checks here pass. Right now, this is guaranteed by the return on
    // failure here.
    if (!pid_file.empty()) {
        logERR("Attempting to write pid-file twice.");
        return EXIT_FAILURE;
    }

    if (check_pid_file(pid_filepath) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    if (!numwrite(pid_filepath.c_str(), getpid())) {
        logERR("Writing to the specified pid-file failed.");
        return EXIT_FAILURE;
    }
    pid_file = pid_filepath;
    atexit(remove_pid_file);

    return EXIT_SUCCESS;
}

// Extracts an option that appears either zero or once.  Multiple appearances are not allowed (or
// expected).
boost::optional<std::string> get_optional_option(const std::map<std::string, options::values_t> &opts,
                                                 const std::string &name,
                                                 std::string *source_out) {
    auto it = opts.find(name);
    if (it == opts.end()) {
        *source_out = "nowhere";
        return boost::optional<std::string>();
    }

    if (it->second.values.empty()) {
        *source_out = it->second.source;
        return boost::optional<std::string>();
    }

    if (it->second.values.size() == 1) {
        *source_out = it->second.source;
        return it->second.values[0];
    }

    throw std::logic_error("Option '%s' appears multiple times (when it should only appear once.)");
}

boost::optional<std::string> get_optional_option(const std::map<std::string, options::values_t> &opts,
                                                 const std::string &name) {
    std::string source;
    return get_optional_option(opts, name, &source);
}

// Returns false if the group was not found.  This function replaces a call to
// getgrnam(3).  That's right, getgrnam_r's interface is such that you have to
// go through these shenanigans.
bool get_group_id(const char *name, gid_t *group_id_out) {
    // On Linux we can use sysconf to learn what the bufsize should be but on OS
    // X we just have to guess.
    size_t bufsize = 4096;
    // I think 128 MB ought to be enough.
    while (bufsize <= (1 << 17)) {
        scoped_array_t<char> buf(bufsize);

        struct group g;
        struct group *result;
        int res;
        do {
            res = getgrnam_r(name, &g, buf.data(), bufsize, &result);
        } while (res == EINTR);

        if (res == 0) {
            if (result == NULL) {
                return false;
            } else {
                *group_id_out = result->gr_gid;
                return true;
            }
        } else if (res == ERANGE) {
            bufsize *= 2;
        } else {
            // Here's what the man page says about error codes of getgrnam_r:
            // 0 or ENOENT or ESRCH or EBADF or EPERM or ...
            //        The given name or uid was not found.
            //
            // So let's just return false here.  I'm sure EMFILE and ENFILE and
            // EIO will turn up again somewhere else.
            return false;
        }
    }

    rassert(false, "get_group_id bufsize overflow");
    return false;
}

// Returns false if the user was not found.  This function replaces a call to
// getpwnam(3).  That's right, getpwnam_r's interface is such that you have to
// go through these shenanigans.
bool get_user_ids(const char *name, int *user_id_out, gid_t *user_group_id_out) {
    // On Linux we can use sysconf to learn what the bufsize should be but on OS
    // X we just have to guess.
    size_t bufsize = 4096;
    // I think 128 MB ought to be enough.
    while (bufsize <= (1 << 17)) {
        scoped_array_t<char> buf(bufsize);

        struct passwd p;
        struct passwd *result;
        int res;
        do {
            res = getpwnam_r(name, &p, buf.data(), bufsize, &result);
        } while (res == EINTR);

        if (res == 0) {
            if (result == NULL) {
                return false;
            } else {
                *user_id_out = result->pw_uid;
                *user_group_id_out = result->pw_gid;
                return true;
            }
        } else if (res == ERANGE) {
            bufsize *= 2;
        } else {
            // Here's what the man page says about error codes of getpwnam_r:
            // 0 or ENOENT or ESRCH or EBADF or EPERM or ...
            //        The given name or uid was not found.
            //
            // So let's just return false here.  I'm sure EMFILE and ENFILE and
            // EIO will turn up again somewhere else.
            //
            // (Yes, this is the same situation as with getgrnam_r, not some
            // copy/paste.)
            return false;
        }
    }

    rassert(false, "get_user_ids bufsize overflow");
    return false;
}


void set_user_group(const std::map<std::string, options::values_t> &opts) {
    boost::optional<std::string> rungroup = get_optional_option(opts, "--rungroup");
    boost::optional<std::string> runuser = get_optional_option(opts, "--runuser");

    if (rungroup) {
        gid_t group_id;
        if (!get_group_id(rungroup->c_str(), &group_id)) {
            throw std::runtime_error(strprintf("Group '%s' not found: %s",
                                               rungroup->c_str(), errno_string(get_errno()).c_str()).c_str());
        }
        if (setgid(group_id) != 0) {
            throw std::runtime_error(strprintf("Could not set group to '%s': %s",
                                               rungroup->c_str(), errno_string(get_errno()).c_str()).c_str());
        }
    }

    if (runuser) {
        int user_id;
        gid_t user_group_id;
        if (!get_user_ids(runuser->c_str(), &user_id, &user_group_id)) {
            throw std::runtime_error(strprintf("User '%s' not found: %s",
                                               runuser->c_str(), errno_string(get_errno()).c_str()).c_str());
        }
        if (!rungroup) {
            // No group specified, use the user's group
            if (setgid(user_group_id) != 0) {
                throw std::runtime_error(strprintf("Could not use the group of user '%s': %s",
                                                   runuser->c_str(), errno_string(get_errno()).c_str()).c_str());
            }
        }
        if (setuid(user_id) != 0) {
            throw std::runtime_error(strprintf("Could not set user account to '%s': %s",
                                               runuser->c_str(), errno_string(get_errno()).c_str()).c_str());
        }
    }
}

int check_pid_file(const std::map<std::string, options::values_t> &opts) {
    boost::optional<std::string> pid_filepath = get_optional_option(opts, "--pid-file");
    if (!pid_filepath || pid_filepath->empty()) {
        return EXIT_SUCCESS;
    }

    return check_pid_file(*pid_filepath);
}

// Maybe writes a pid file, using the --pid-file option, if it's present.
int write_pid_file(const std::map<std::string, options::values_t> &opts) {
    boost::optional<std::string> pid_filepath = get_optional_option(opts, "--pid-file");
    if (!pid_filepath || pid_filepath->empty()) {
        return EXIT_SUCCESS;
    }

    return write_pid_file(*pid_filepath);
}

// Extracts an option that must appear exactly once.  (This is often used for optional arguments
// that have a default value.)
std::string get_single_option(const std::map<std::string, options::values_t> &opts,
                              const std::string &name,
                              std::string *source_out) {
    std::string source;
    boost::optional<std::string> value = get_optional_option(opts, name, &source);

    if (!value) {
        throw std::logic_error(strprintf("Missing option '%s'.  (It does not have a default value.)", name.c_str()));
    }

    *source_out = source;
    return *value;
}

std::string get_single_option(const std::map<std::string, options::values_t> &opts,
                              const std::string &name) {
    std::string source;
    return get_single_option(opts, name, &source);
}

// Used for options that don't take parameters, such as --help or --exit-failure, tells whether the
// option exists.
bool exists_option(const std::map<std::string, options::values_t> &opts, const std::string &name) {
    auto it = opts.find(name);
    return it != opts.end() && !it->second.values.empty();
}

void print_version_message() {
    printf("%s\n", RETHINKDB_VERSION_STR);
}

bool handle_help_or_version_option(const std::map<std::string, options::values_t> &opts,
                                   void (*help_fn)()) {
    if (exists_option(opts, "--help")) {
        help_fn();
        return true;
    } else if (exists_option(opts, "--version")) {
        print_version_message();
        return true;
    }
    return false;
}

serializer_filepath_t get_cluster_metadata_filename(const base_path_t& dirpath) {
    return serializer_filepath_t(dirpath, "metadata");
}

serializer_filepath_t get_auth_metadata_filename(const base_path_t& dirpath) {
    return serializer_filepath_t(dirpath, "auth_metadata");
}

void initialize_logfile(const std::map<std::string, options::values_t> &opts,
                        const base_path_t& dirpath) {
    std::string filename;
    if (exists_option(opts, "--log-file")) {
        filename = get_single_option(opts, "--log-file");
    } else {
        filename = dirpath.path() + "/log_file";
    }
    install_fallback_log_writer(filename);
}

std::string get_web_path(boost::optional<std::string> web_static_directory, char **argv) {
    // We check first for a run-time option, then check the home of the binary,
    // and then we check in the install location if such a location was provided
    // at compile time.
    path_t result;
    if (web_static_directory) {
        result = parse_as_path(*web_static_directory);
    } else {
        result = parse_as_path(argv[0]);
        result.nodes.pop_back();
        result.nodes.push_back(WEB_ASSETS_DIR_NAME);
#ifdef WEBRESDIR
        std::string chkdir(WEBRESDIR);
        if ((access(render_as_path(result).c_str(), F_OK)) && (!access(chkdir.c_str(), F_OK))) {
            result = parse_as_path(chkdir);
        }
#endif  // WEBRESDIR
    }

    // Make sure we return an absolute path
    base_path_t abs_path(render_as_path(result));

    if (!check_existence(abs_path)) {
        throw std::runtime_error(strprintf("ERROR: web assets directory not found '%s'",
                                           abs_path.path().c_str()).c_str());
    }

    abs_path.make_absolute();
    return abs_path.path();
}

std::string get_web_path(const std::map<std::string, options::values_t> &opts, char **argv) {
    if (!exists_option(opts, "--no-http-admin")) {
        boost::optional<std::string> web_static_directory = get_optional_option(opts, "--web-static-directory");
        return get_web_path(web_static_directory, argv);
    }
    return std::string();
}

uint64_t get_avail_mem_size() {
    uint64_t page_size = sysconf(_SC_PAGESIZE);

#if defined(__MACH__)
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if (KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)) {
        fprintf(stderr, "ERROR: could not determine available RAM for the default cache size (errno=%d).\n", get_errno());
        return 1024 * MEGABYTE;
    }
    return vmstat.free_count * page_size;
#else
    uint64_t avail_mem_pages = sysconf(_SC_AVPHYS_PAGES);
    return avail_mem_pages * page_size;
#endif
}

uint64_t get_total_cache_size(const std::map<std::string, options::values_t> &opts) {
    uint64_t cache_limit = std::numeric_limits<intptr_t>::max();
    int64_t available_mem = get_avail_mem_size();

    // Default to half the available memory minus a gigabyte, to leave room for server
    // and query overhead, but never default to less than 100 megabytes
    int64_t signed_res = std::min<int64_t>(available_mem - GIGABYTE, cache_limit) / DEFAULT_MAX_CACHE_RATIO;
    uint64_t res = std::max<int64_t>(signed_res, 100 * MEGABYTE);

    if (exists_option(opts, "--cache-size")) {
        std::string cache_size_opt = get_single_option(opts, "--cache-size");
        if (!strtou64_strict(cache_size_opt, 10, &res)) {
            throw std::runtime_error(strprintf(
                    "ERROR: could not parse cache-size as a number (%s)",
                    cache_size_opt.c_str()));
        }
        res = res * MEGABYTE;

        if (res > cache_limit) {
            throw std::runtime_error(strprintf(
                    "Requested cache size (%" PRIu64 " MB) is higher than the "
                    "expected upper-bound for this platform (%" PRIu64" MB).",
                    res / 1024 / 1024, cache_limit / 1024 / 1024));
        }
    }

    return res;
}

// Note that this defaults to the peer port if no port is specified
//  (at the moment, this is only used for parsing --join directives)
// Possible formats:
//  host only: newton
//  host and port: newton:60435
//  IPv4 addr only: 192.168.0.1
//  IPv4 addr and port: 192.168.0.1:60435
//  IPv6 addr only: ::dead:beef
//  IPv6 addr only: [::dead:beef]
//  IPv6 addr and port: [::dead:beef]:60435
//  IPv4-mapped IPv6 addr only: ::ffff:192.168.0.1
//  IPv4-mapped IPv6 addr only: [::ffff:192.168.0.1]
//  IPv4-mapped IPv6 addr and port: [::ffff:192.168.0.1]:60435
host_and_port_t parse_host_and_port(const std::string &source, const std::string &option_name,
                                    const std::string &value, int default_port) {
    // First disambiguate IPv4 vs IPv6
    size_t colon_count = std::count(value.begin(), value.end(), ':');

    if (colon_count < 2) {
        // IPv4 will have 1 or less colons
        size_t colon_loc = value.find_last_of(':');
        if (colon_loc != std::string::npos) {
            std::string host = value.substr(0, colon_loc);
            int port = atoi(value.substr(colon_loc + 1).c_str());
            if (host.size() != 0 && port != 0 && port <= MAX_PORT) {
                return host_and_port_t(host, port_t(port));
            }
        } else if (value.size() != 0) {
            return host_and_port_t(value, port_t(default_port));
        }
    } else {
        // IPv6 will have 2 or more colons
        size_t last_colon_loc = value.find_last_of(':');
        size_t start_bracket_loc = value.find_first_of('[');
        size_t end_bracket_loc = value.find_last_of(']');

        if (start_bracket_loc > end_bracket_loc) {
            // Error condition fallthrough
        } else if (start_bracket_loc == std::string::npos || end_bracket_loc == std::string::npos) {
            // No brackets, therefore no port, just parse the whole thing as a hostname
            return host_and_port_t(value, port_t(default_port));
        } else if (last_colon_loc < end_bracket_loc) {
            // Brackets, but no port, verify no other characters outside the brackets
            if (value.find_last_not_of(" \t\r\n[", start_bracket_loc) == std::string::npos &&
                value.find_first_not_of(" \t\r\n]", end_bracket_loc) == std::string::npos) {
                std::string host = value.substr(start_bracket_loc + 1, end_bracket_loc - start_bracket_loc - 1);
                return host_and_port_t(host, port_t(default_port));
            }
        } else {
            // Brackets and port
            std::string host = value.substr(start_bracket_loc + 1, end_bracket_loc - start_bracket_loc - 1);
            std::string remainder = value.substr(end_bracket_loc + 1);
            size_t remainder_colon_loc = remainder.find_first_of(':');
            int port = atoi(remainder.substr(remainder_colon_loc + 1).c_str());

            // Verify no characters before the brackets and up to the port colon
            if (port != 0 && port <= MAX_PORT && remainder_colon_loc == 0 &&
                value.find_last_not_of(" \t\r\n[", start_bracket_loc) == std::string::npos) {
                return host_and_port_t(host, port_t(port));
            }
        }
    }


    throw options::value_error_t(source, option_name,
                                 strprintf("Option '%s' has invalid host and port format '%s'",
                                           option_name.c_str(), value.c_str()));
}

class address_lookup_exc_t : public std::exception {
public:
    explicit address_lookup_exc_t(const std::string& data) : info(data) { }
    ~address_lookup_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

std::set<ip_address_t> get_local_addresses(const std::vector<std::string> &bind_options) {
    std::set<ip_address_t> set_filter;
    bool all = false;

    // Scan through specified bind options
    for (size_t i = 0; i < bind_options.size(); ++i) {
        if (bind_options[i] == "all") {
            all = true;
        } else {
            // Verify that all specified addresses are valid ip addresses
            try {
                ip_address_t addr(bind_options[i]);
                if (addr.is_any()) {
                    all = true;
                } else {
                    set_filter.insert(addr);
                }
            } catch (const std::exception &ex) {
                throw address_lookup_exc_t(strprintf("bind ip address '%s' could not be parsed: %s",
                                                     bind_options[i].c_str(),
                                                     ex.what()));
            }
        }
    }

    std::set<ip_address_t> result = get_local_ips(set_filter, all);

    // Make sure that all specified addresses were found
    for (std::set<ip_address_t>::iterator i = set_filter.begin(); i != set_filter.end(); ++i) {
        if (result.find(*i) == result.end()) {
            std::string errmsg = strprintf("could not find bind ip address '%s'", i->to_string().c_str());
            if (i->is_ipv6_link_local()) {
                errmsg += strprintf(", this is an IPv6 link-local address, make sure the scope is correct");
            }
            throw address_lookup_exc_t(errmsg);
        }
    }

    if (result.empty()) {
        throw address_lookup_exc_t("no local addresses found to bind to");
    }

    // If we will use all local addresses, return an empty set, which is how tcp_listener_t does it
    if (all) {
        return std::set<ip_address_t>();
    }

    return result;
}

// Returns the options vector for a given option name.  The option must *exist*!  Typically this is
// for OPTIONAL_REPEAT options with a default value being the empty vector.
const std::vector<std::string> &all_options(const std::map<std::string, options::values_t> &opts,
                                            const std::string &name,
                                            std::string *source_out) {
    auto it = opts.find(name);
    if (it == opts.end()) {
        throw std::logic_error(strprintf("option '%s' not found", name.c_str()));
    }
    *source_out = it->second.source;
    return it->second.values;
}

const std::vector<std::string> &all_options(const std::map<std::string, options::values_t> &opts,
                                            const std::string &name) {
    std::string source;
    return all_options(opts, name, &source);
}

// Gets a single integer option, often an optional integer option with a default value.
int get_single_int(const std::map<std::string, options::values_t> &opts, const std::string &name) {
    const std::string value = get_single_option(opts, name);
    int64_t x;
    if (strtoi64_strict(value, 10, &x)) {
        if (INT_MIN <= x && x <= INT_MAX) {
            return x;
        }
    }
    throw std::runtime_error(strprintf("Option '%s' (with value '%s') not a valid integer",
                                       name.c_str(), value.c_str()));
}

int offseted_port(const int port, const int port_offset) {
    return port == 0 ? 0 : port + port_offset;
}

peer_address_t get_canonical_addresses(const std::map<std::string, options::values_t> &opts,
                                       int default_port) {
    std::string source;
    std::vector<std::string> canonical_options = all_options(opts, "--canonical-address", &source);
    // Verify that all specified addresses are valid ip addresses
    std::set<host_and_port_t> result;
    for (size_t i = 0; i < canonical_options.size(); ++i) {
        host_and_port_t host_port = parse_host_and_port(source, "--canonical-address",
                                                        canonical_options[i], default_port);

        if (host_port.port().value() == 0 && default_port != 0) {
            // The cluster layer would probably swap this out with whatever port we were
            //  actually listening on, but since the user explicitly specified 0, it doesn't make sense
            throw std::logic_error("cannot specify a port of 0 in --canonical-address");
        }
        result.insert(host_port);
    }
    return peer_address_t(result);
}

service_address_ports_t get_service_address_ports(const std::map<std::string, options::values_t> &opts) {
    const int port_offset = get_single_int(opts, "--port-offset");
    const int cluster_port = offseted_port(get_single_int(opts, "--cluster-port"), port_offset);
    return service_address_ports_t(get_local_addresses(all_options(opts, "--bind")),
                                   get_canonical_addresses(opts, cluster_port),
                                   cluster_port,
                                   get_single_int(opts, "--client-port"),
                                   exists_option(opts, "--no-http-admin"),
                                   offseted_port(get_single_int(opts, "--http-port"), port_offset),
                                   offseted_port(get_single_int(opts, "--driver-port"), port_offset),
                                   port_offset);
}


void run_rethinkdb_create(const base_path_t &base_path,
                          const name_string_t &machine_name,
                          const file_direct_io_mode_t direct_io_mode,
                          const int max_concurrent_io_requests,
                          bool *const result_out) {
    machine_id_t our_machine_id = generate_uuid();

    cluster_semilattice_metadata_t cluster_metadata;
    auth_semilattice_metadata_t auth_metadata;

    machine_semilattice_metadata_t machine_semilattice_metadata;
    machine_semilattice_metadata.name = machine_semilattice_metadata.name.make_new_version(machine_name, our_machine_id);
    machine_semilattice_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
    cluster_metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(machine_semilattice_metadata)));

    io_backender_t io_backender(direct_io_mode, max_concurrent_io_requests);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    perfmon_collection_t auth_perfmon_collection;
    perfmon_membership_t auth_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "auth_metadata");

    try {
        metadata_persistence::cluster_persistent_file_t cluster_metadata_file(&io_backender,
                                                                              get_cluster_metadata_filename(base_path),
                                                                              &metadata_perfmon_collection,
                                                                              our_machine_id,
                                                                              cluster_metadata);
        metadata_persistence::auth_persistent_file_t auth_metadata_file(&io_backender,
                                                                        get_auth_metadata_filename(base_path),
                                                                        &auth_perfmon_collection,
                                                                        auth_semilattice_metadata_t());
        logINF("Our machine ID: %s\n", uuid_to_str(our_machine_id).c_str());
        logINF("Created directory '%s' and a metadata file inside it.\n", base_path.path().c_str());
        *result_out = true;
    } catch (const metadata_persistence::file_in_use_exc_t &ex) {
        logINF("Directory '%s' is in use by another rethinkdb process.\n", base_path.path().c_str());
        *result_out = false;
    }
}

void run_rethinkdb_admin(const std::vector<host_and_port_t> &joins,
                         const peer_address_t &canonical_addresses,
                         int client_port,
                         const std::vector<std::string>& command_args,
                         bool exit_on_failure,
                         bool *const result_out) {
    os_signal_cond_t sigint_cond;
    *result_out = true;
    std::string host_port;

    if (!joins.empty()) {
        host_port = strprintf("%s:%d", joins[0].host().c_str(), joins[0].port().value());
    }

    try {
        if (command_args.empty())
            admin_command_parser_t(host_port,
                                   look_up_peers_addresses(joins),
                                   canonical_addresses,
                                   client_port, &sigint_cond).run_console(exit_on_failure);
        else if (command_args[0] == admin_command_parser_t::complete_command)
            admin_command_parser_t(host_port,
                                   look_up_peers_addresses(joins),
                                   canonical_addresses,
                                   client_port, &sigint_cond).run_completion(command_args);
        else
            admin_command_parser_t(host_port,
                                   look_up_peers_addresses(joins),
                                   canonical_addresses,
                                   client_port, &sigint_cond).parse_and_run_command(command_args);
    } catch (const admin_no_connection_exc_t& ex) {
        // Don't use logging, because we might want to printout multiple lines and such, which the log system doesn't like
        fprintf(stderr, "%s\n", ex.what());
        fprintf(stderr, "valid --join option required to handle command, run 'rethinkdb help admin' for more information\n");
        *result_out = false;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        *result_out = false;
    }
}

std::string uname_msr() {
    char buf[1024];
    static const std::string unknown = "unknown operating system\n";
    FILE *out = popen("uname -msr", "r");
    if (!out) return unknown;
    if (!fgets(buf, sizeof(buf), out)) {
        pclose(out);
        return unknown;
    }
    pclose(out);
    return buf;
}

void run_rethinkdb_serve(const base_path_t &base_path,
                         serve_info_t *serve_info,
                         const file_direct_io_mode_t direct_io_mode,
                         const int max_concurrent_io_requests,
                         const uint64_t total_cache_size,
                         const machine_id_t *our_machine_id,
                         const cluster_semilattice_metadata_t *cluster_metadata,
                         directory_lock_t *data_directory_lock,
                         bool *const result_out) {
    logINF("Running %s...\n", RETHINKDB_VERSION_STR);
    logINF("Running on %s", uname_msr().c_str());
    os_signal_cond_t sigint_cond;

    logINF("Using cache size of %" PRIu64 " MB",
           total_cache_size / static_cast<uint64_t>(MEGABYTE));

    // Provide some warnings if the cache size or available memory seem inadequate
    // We can't *really* tell what could go wrong given that we don't know how much data
    // or what kind of queries will be run, so these are just somewhat reasonable values.
    uint64_t available_memory = get_avail_mem_size();
    if (total_cache_size > get_avail_mem_size()) {
        logWRN("Requested cache size is larger than available memory.");
    } else if (total_cache_size + GIGABYTE > get_avail_mem_size()) {
        logWRN("Cache size does not leave much memory for server and query "
               "overhead (available memory: %" PRIu64 " MB).",
               available_memory / static_cast<uint64_t>(MEGABYTE));
    }
    if (total_cache_size <= 100 * MEGABYTE) {
        logWRN("Cache size is very low and may impact performance.");
    }


    logINF("Loading data from directory %s\n", base_path.path().c_str());

    io_backender_t io_backender(direct_io_mode, max_concurrent_io_requests);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    perfmon_collection_t auth_perfmon_collection;
    perfmon_membership_t auth_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "auth_metadata");

    try {
        scoped_ptr_t<metadata_persistence::cluster_persistent_file_t> cluster_metadata_file;
        scoped_ptr_t<metadata_persistence::auth_persistent_file_t> auth_metadata_file;
        if (our_machine_id && cluster_metadata) {
            cluster_metadata_file.init(
                new metadata_persistence::cluster_persistent_file_t(&io_backender,
                                                                    get_cluster_metadata_filename(base_path),
                                                                    &metadata_perfmon_collection,
                                                                    *our_machine_id,
                                                                    *cluster_metadata));
            auth_metadata_file.init(
                new metadata_persistence::auth_persistent_file_t(&io_backender,
                                                                 get_auth_metadata_filename(base_path),
                                                                 &auth_perfmon_collection,
                                                                 auth_semilattice_metadata_t()));
        } else {
            cluster_metadata_file.init(
                new metadata_persistence::cluster_persistent_file_t(&io_backender,
                                                                    get_cluster_metadata_filename(base_path),
                                                                    &metadata_perfmon_collection));
            auth_metadata_file.init(
                new metadata_persistence::auth_persistent_file_t(&io_backender,
                                                                 get_auth_metadata_filename(base_path),
                                                                 &auth_perfmon_collection));
        }

        // Tell the directory lock that the directory is now good to go, as it will
        //  otherwise delete an uninitialized directory
        data_directory_lock->directory_initialized();

        serve_info->look_up_peers();
        *result_out = serve(&io_backender,
                            base_path,
                            cluster_metadata_file.get(),
                            auth_metadata_file.get(),
                            total_cache_size,
                            *serve_info,
                            &sigint_cond);

    } catch (const metadata_persistence::file_in_use_exc_t &ex) {
        logINF("Directory '%s' is in use by another rethinkdb process.\n", base_path.path().c_str());
        *result_out = false;
    } catch (const host_lookup_exc_t &ex) {
        logERR("%s\n", ex.what());
        *result_out = false;
    }
}

void run_rethinkdb_porcelain(const base_path_t &base_path,
                             const name_string_t &machine_name,
                             const file_direct_io_mode_t direct_io_mode,
                             const int max_concurrent_io_requests,
                             const uint64_t total_cache_size,
                             const bool new_directory,
                             serve_info_t *serve_info,
                             directory_lock_t *data_directory_lock,
                             bool *const result_out) {
    if (!new_directory) {
        run_rethinkdb_serve(base_path, serve_info, direct_io_mode,
                            max_concurrent_io_requests, total_cache_size,
                            NULL, NULL, data_directory_lock,
                            result_out);
    } else {
        logINF("Creating directory %s\n", base_path.path().c_str());

        machine_id_t our_machine_id = generate_uuid();

        cluster_semilattice_metadata_t cluster_metadata;

        machine_semilattice_metadata_t our_machine_metadata;
        our_machine_metadata.name = vclock_t<name_string_t>(machine_name, our_machine_id);
        our_machine_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
        cluster_metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(our_machine_metadata)));

        if (serve_info->joins.empty()) {
            logINF("Creating a default database for your convenience. (This is because you ran 'rethinkdb' "
                   "without 'create', 'serve', or '--join', and the directory '%s' did not already exist.)\n",
                   base_path.path().c_str());

            /* Create a test database. */
            database_id_t database_id = generate_uuid();
            database_semilattice_metadata_t database_metadata;
            name_string_t db_name;
            bool db_name_success = db_name.assign_value("test");
            guarantee(db_name_success);
            database_metadata.name = vclock_t<name_string_t>(db_name, our_machine_id);
            cluster_metadata.databases.databases.insert(std::make_pair(
                database_id,
                deletable_t<database_semilattice_metadata_t>(database_metadata)));
        }

        run_rethinkdb_serve(base_path, serve_info, direct_io_mode,
                            max_concurrent_io_requests, total_cache_size,
                            &our_machine_id, &cluster_metadata,
                            data_directory_lock, result_out);
    }
}

void run_rethinkdb_proxy(serve_info_t *serve_info, bool *const result_out) {
    os_signal_cond_t sigint_cond;
    guarantee(!serve_info->joins.empty());

    try {
        serve_info->look_up_peers();
        *result_out = serve_proxy(*serve_info,
                                  &sigint_cond);
    } catch (const host_lookup_exc_t &ex) {
        logERR("%s\n", ex.what());
        *result_out = false;
    }
}

options::help_section_t get_machine_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Machine name options");
    options_out->push_back(options::option_t(options::names_t("--machine-name", "-n"),
                                             options::OPTIONAL,
                                             get_random_machine_name()));
    help.add("-n [ --machine-name ] arg",
             "the name for this machine (as will appear in the metadata).  If not"
             " specified, it will be randomly chosen from a short list of names.");
    return help;
}

options::help_section_t get_log_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Log options");
    options_out->push_back(options::option_t(options::names_t("--log-file"),
                                             options::OPTIONAL));
    help.add("--log-file file", "specify the file to log to, defaults to 'log_file'");
    return help;
}


options::help_section_t get_file_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("File path options");
    options_out->push_back(options::option_t(options::names_t("--directory", "-d"),
                                             options::OPTIONAL,
                                             "rethinkdb_data"));
    help.add("-d [ --directory ] path", "specify directory to store data and metadata");
    options_out->push_back(options::option_t(options::names_t("--io-threads"),
                                             options::OPTIONAL,
                                             strprintf("%d", DEFAULT_MAX_CONCURRENT_IO_REQUESTS)));
    help.add("--io-threads n",
             "how many simultaneous I/O operations can happen at the same time");
    options_out->push_back(options::option_t(options::names_t("--no-direct-io"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("--no-direct-io", "disable direct I/O");
    options_out->push_back(options::option_t(options::names_t("--cache-size"),
                                             options::OPTIONAL));
    help.add("--cache-size mb", "total cache size (in megabytes) for the process");
    return help;
}

options::help_section_t get_config_file_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Configuration file options");
    options_out->push_back(options::option_t(options::names_t("--config-file"),
                                             options::OPTIONAL));
    help.add("--config-file", "take options from a configuration file");
    return help;
}

std::vector<host_and_port_t> parse_join_options(const std::map<std::string, options::values_t> &opts,
                                                int default_port) {
    std::string source;
    const std::vector<std::string> join_strings = all_options(opts, "--join", &source);
    std::vector<host_and_port_t> joins;
    for (auto it = join_strings.begin(); it != join_strings.end(); ++it) {
        joins.push_back(parse_host_and_port(source, "--join", *it, default_port));
    }
    return joins;
}

std::string get_reql_http_proxy_option(const std::map<std::string, options::values_t> &opts) {
    std::string source;
    boost::optional<std::string> proxy = get_optional_option(opts, "--reql-http-proxy", &source);
    if (!proxy.is_initialized()) {
        return std::string();
    }

    // We verify the correct format here, as we won't be configuring libcurl until later,
    // in the extprocs.  At the moment, we do not support specifying IPv6 addresses.
    //
    // protocol = proxy protocol recognized by libcurl: (http, socks4, socks4a, socks5, socks5h)
    // host = hostname or ip address
    // port = integer in range [0-65535]
    // [protocol://]host[:port]
    //
    // The chunks in the regex used to parse and verify the format are:
    //   protocol - (?:([A-z][A-z0-9+-.]*)(?:://))? - captures the protocol, adhering to
    //     RFC 3986, discarding the '://' from the end
    //   host - ([A-z0-9.-]+) - captures the hostname or ip address, consisting of letters,
    //     numbers, dashes, and dots
    //   port - (?::(\d+))? - captures the numeric port, discarding the preceding ':'
    RE2 re2_parser("(?:([A-z][A-z0-9+-.]*)(?:://))?([A-z0-9_.-]+)(?::(\\d+))?");
    std::string protocol, host, port_str;
    if (!RE2::FullMatch(proxy.get(), re2_parser, &protocol, &host, &port_str)) {
        throw std::runtime_error(strprintf("--reql-http-proxy format unrecognized, "
                                           "expected [protocol://]host[:port]: %s",
                                           proxy.get().c_str()));
    }

    if (!protocol.empty() &&
        protocol != "http" &&
        protocol != "socks4" &&
        protocol != "socks4a" &&
        protocol != "socks5" &&
        protocol != "socks5h") {
        throw std::runtime_error(strprintf("--reql-http-proxy protocol unrecognized (%s), "
                                           "must be one of http, socks4, socks4a, socks5, "
                                           "and socks5h", protocol.c_str()));
    }

    if (!port_str.empty()) {
        int port = atoi(port_str.c_str());
        if (port_str.length() > 5 || port <= 0 || port > MAX_PORT) {
            throw std::runtime_error(strprintf("--reql-http-proxy port (%s) is not in "
                                               "the valid range (0-65535)",
                                               port_str.c_str()));
        }
    }
    return proxy.get();
}

options::help_section_t get_web_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Web options");
    options_out->push_back(options::option_t(options::names_t("--web-static-directory"),
                                             options::OPTIONAL));
    help.add("--web-static-directory directory", "the directory containing web resources for the http interface");
    options_out->push_back(options::option_t(options::names_t("--http-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::http_port)));
    help.add("--http-port port", "port for web administration console");
    options_out->push_back(options::option_t(options::names_t("--no-http-admin"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("--no-http-admin", "disable web administration console");
    return help;
}

options::help_section_t get_network_options(const bool join_required, std::vector<options::option_t> *options_out) {
    options::help_section_t help("Network options");
    options_out->push_back(options::option_t(options::names_t("--bind"),
                                             options::OPTIONAL_REPEAT));
    help.add("--bind {all | addr}", "add the address of a local interface to listen on when accepting connections; loopback addresses are enabled by default");

    options_out->push_back(options::option_t(options::names_t("--cluster-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::peer_port)));
    help.add("--cluster-port port", "port for receiving connections from other nodes");

    options_out->push_back(options::option_t(options::names_t("--client-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::client_port)));
#ifndef NDEBUG
    help.add("--client-port port", "port to use when connecting to other nodes (for development)");
#endif  // NDEBUG

    options_out->push_back(options::option_t(options::names_t("--driver-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::reql_port)));
    help.add("--driver-port port", "port for rethinkdb protocol client drivers");

    options_out->push_back(options::option_t(options::names_t("--port-offset", "-o"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::port_offset)));
    help.add("-o [ --port-offset ] offset", "all ports used locally will have this value added");

    options_out->push_back(options::option_t(options::names_t("--join", "-j"),
                                             join_required ? options::MANDATORY_REPEAT : options::OPTIONAL_REPEAT));
    help.add("-j [ --join ] host:port", "host and port of a rethinkdb node to connect to");

    options_out->push_back(options::option_t(options::names_t("--reql-http-proxy"),
                                             options::OPTIONAL));
    help.add("--reql-http-proxy [protocol://]host[:port]", "HTTP proxy to use for performing `r.http(...)` queries, default port is 1080");

    options_out->push_back(options::option_t(options::names_t("--canonical-address"),
                                             options::OPTIONAL_REPEAT));
    help.add("--canonical-address addr", "address that other rethinkdb instances will use to connect to us, can be specified multiple times");

    return help;
}

options::help_section_t get_cpu_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("CPU options");
    options_out->push_back(options::option_t(options::names_t("--cores", "-c"),
                                             options::OPTIONAL,
                                             strprintf("%d", get_cpu_count())));
    help.add("-c [ --cores ] n", "the number of cores to use");
    return help;
}

MUST_USE bool parse_cores_option(const std::map<std::string, options::values_t> &opts,
                                 int *num_workers_out) {
    int num_workers = get_single_int(opts, "--cores");
    if (num_workers <= 0 || num_workers > MAX_THREADS) {
        fprintf(stderr, "ERROR: number specified for cores to use must be between 1 and %d\n", MAX_THREADS);
        return false;
    }
    *num_workers_out = num_workers;
    return true;
}

options::help_section_t get_service_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Service options");
    options_out->push_back(options::option_t(options::names_t("--pid-file"),
                                             options::OPTIONAL));
    help.add("--pid-file path", "a file in which to write the process id when the process is running");
    options_out->push_back(options::option_t(options::names_t("--daemon"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("--daemon", "daemonize this rethinkdb process");
    return help;
}

options::help_section_t get_setuser_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Set User/Group options");
    options_out->push_back(options::option_t(options::names_t("--runuser"),
                                             options::OPTIONAL));
    help.add("--runuser user", "run as the specified user");
    options_out->push_back(options::option_t(options::names_t("--rungroup"),
                                             options::OPTIONAL));
    help.add("--rungroup group", "run with the specified group");

    return help;
}

options::help_section_t get_help_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Help options");
    options_out->push_back(options::option_t(options::names_t("--help", "-h"),
                                             options::OPTIONAL_NO_PARAMETER));
    options_out->push_back(options::option_t(options::names_t("--version", "-v"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("-h [ --help ]", "print this help");
    help.add("-v [ --version ]", "print the version number of rethinkdb");
    return help;
}

void get_rethinkdb_create_options(std::vector<options::help_section_t> *help_out,
                                  std::vector<options::option_t> *options_out) {
    help_out->push_back(get_file_options(options_out));
    help_out->push_back(get_machine_options(options_out));
    help_out->push_back(get_setuser_options(options_out));
    help_out->push_back(get_help_options(options_out));
    help_out->push_back(get_log_options(options_out));
    help_out->push_back(get_config_file_options(options_out));
}

void get_rethinkdb_serve_options(std::vector<options::help_section_t> *help_out,
                                 std::vector<options::option_t> *options_out) {
    help_out->push_back(get_file_options(options_out));
    help_out->push_back(get_network_options(false, options_out));
    help_out->push_back(get_web_options(options_out));
    help_out->push_back(get_cpu_options(options_out));
    help_out->push_back(get_service_options(options_out));
    help_out->push_back(get_setuser_options(options_out));
    help_out->push_back(get_help_options(options_out));
    help_out->push_back(get_log_options(options_out));
    help_out->push_back(get_config_file_options(options_out));
}

void get_rethinkdb_proxy_options(std::vector<options::help_section_t> *help_out,
                                 std::vector<options::option_t> *options_out) {
    help_out->push_back(get_network_options(true, options_out));
    help_out->push_back(get_web_options(options_out));
    help_out->push_back(get_service_options(options_out));
    help_out->push_back(get_setuser_options(options_out));
    help_out->push_back(get_help_options(options_out));
    help_out->push_back(get_log_options(options_out));
    help_out->push_back(get_config_file_options(options_out));
}

void get_rethinkdb_admin_options(std::vector<options::help_section_t> *help_out,
                                 std::vector<options::option_t> *options_out) {
    options::help_section_t help("Allowed options");

    options_out->push_back(options::option_t(options::names_t("--client-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::client_port)));
#ifndef NDEBUG
    help.add("--client-port port", "port to use when connecting to other nodes (for development)");
#endif  // NDEBUG

    options_out->push_back(options::option_t(options::names_t("--join", "-j"),
                                             options::OPTIONAL_REPEAT,
                                             "localhost"));
    help.add("-j [ --join ] host:port", "host and cluster port of a rethinkdb node to connect to");

    options_out->push_back(options::option_t(options::names_t("--canonical-address"),
                                             options::OPTIONAL_REPEAT));
    help.add("--canonical-address addr", "address that other rethinkdb instances will use to connect to us, can be specified multiple times");

    options_out->push_back(options::option_t(options::names_t("--exit-failure", "-x"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("-x [ --exit-failure ]", "exit with an error code immediately if a command fails");

    help_out->push_back(help);
}

void get_rethinkdb_porcelain_options(std::vector<options::help_section_t> *help_out,
                                     std::vector<options::option_t> *options_out) {
    help_out->push_back(get_file_options(options_out));
    help_out->push_back(get_machine_options(options_out));
    help_out->push_back(get_network_options(false, options_out));
    help_out->push_back(get_web_options(options_out));
    help_out->push_back(get_cpu_options(options_out));
    help_out->push_back(get_service_options(options_out));
    help_out->push_back(get_setuser_options(options_out));
    help_out->push_back(get_help_options(options_out));
    help_out->push_back(get_log_options(options_out));
    help_out->push_back(get_config_file_options(options_out));
}

std::map<std::string, options::values_t> parse_config_file_flat(const std::string &config_filepath,
                                                                const std::vector<options::option_t> &options) {
    std::string file;
    if (!blocking_read_file(config_filepath.c_str(), &file)) {
        throw std::runtime_error(strprintf("Trouble reading config file '%s'", config_filepath.c_str()));
    }

    std::vector<options::option_t> options_superset;
    std::vector<options::help_section_t> helps_superset;

    // There will be some duplicates in here, but it shouldn't be a problem
    get_rethinkdb_create_options(&helps_superset, &options_superset);
    get_rethinkdb_serve_options(&helps_superset, &options_superset);
    get_rethinkdb_proxy_options(&helps_superset, &options_superset);
    get_rethinkdb_porcelain_options(&helps_superset, &options_superset);

    return options::parse_config_file(file, config_filepath,
                                      options, options_superset);
}

std::map<std::string, options::values_t> parse_commands_deep(int argc, char **argv,
                                                             std::vector<options::option_t> options) {
    std::map<std::string, options::values_t> opts = options::parse_command_line(argc, argv, options);
    const boost::optional<std::string> config_file_name = get_optional_option(opts, "--config-file");
    if (config_file_name) {
        opts = options::merge(opts, parse_config_file_flat(*config_file_name, options));
    }
    opts = options::merge(opts, default_values_map(options));
    return opts;
}

void output_sourced_error(const options::option_error_t &ex) {
    fprintf(stderr, "Error in %s: %s\n", ex.source().c_str(), ex.what());
}

void output_named_error(const options::named_error_t &ex, const std::vector<options::help_section_t> &help) {
    output_sourced_error(ex);

    for (auto section = help.begin(); section != help.end(); ++section) {
        for (auto line = section->help_lines.begin(); line != section->help_lines.end(); ++line) {
            if (line->syntax_description.find(ex.option_name()) != std::string::npos) {
                std::vector<options::help_line_t> one_help_line;
                one_help_line.push_back(*line);
                std::vector<options::help_section_t> one_help_section;
                one_help_section.push_back(options::help_section_t("Usage", one_help_line));
                fprintf(stderr, "%s",
                        options::format_help(one_help_section).c_str());
                break;
            }
        }
    }
}

MUST_USE bool parse_io_threads_option(const std::map<std::string, options::values_t> &opts,
                                      int *max_concurrent_io_requests_out) {
    int max_concurrent_io_requests = get_single_int(opts, "--io-threads");
    if (max_concurrent_io_requests <= 0
        || max_concurrent_io_requests > MAXIMUM_MAX_CONCURRENT_IO_REQUESTS) {
        fprintf(stderr, "ERROR: io-threads must be between 1 and %lld\n",
                MAXIMUM_MAX_CONCURRENT_IO_REQUESTS);
        return false;
    }
    *max_concurrent_io_requests_out = max_concurrent_io_requests;
    return true;
}

file_direct_io_mode_t parse_direct_io_mode_option(const std::map<std::string, options::values_t> &opts) {
    return exists_option(opts, "--no-direct-io") ?
        file_direct_io_mode_t::buffered_desired :
        file_direct_io_mode_t::direct_desired;
}

int main_rethinkdb_create(int argc, char *argv[]) {
    std::vector<options::option_t> options;
    std::vector<options::help_section_t> help;
    get_rethinkdb_create_options(&help, &options);

    try {
        std::map<std::string, options::values_t> opts = parse_commands_deep(argc - 2, argv + 2, options);

        if (handle_help_or_version_option(opts, &help_rethinkdb_create)) {
            return EXIT_SUCCESS;
        }

        options::verify_option_counts(options, opts);

        set_user_group(opts);

        base_path_t base_path(get_single_option(opts, "--directory"));

        std::string machine_name_str = get_single_option(opts, "--machine-name");
        name_string_t machine_name;
        if (!machine_name.assign_value(machine_name_str)) {
            fprintf(stderr, "ERROR: machine-name '%s' is invalid.  (%s)", machine_name_str.c_str(), name_string_t::valid_char_msg);
            return EXIT_FAILURE;
        }

        int max_concurrent_io_requests;
        if (!parse_io_threads_option(opts, &max_concurrent_io_requests)) {
            return EXIT_FAILURE;
        }

        const int num_workers = get_cpu_count();

        bool is_new_directory = false;
        directory_lock_t data_directory_lock(base_path, true, &is_new_directory);

        if (!is_new_directory) {
            fprintf(stderr, "The path '%s' already exists.  Delete it and try again.\n", base_path.path().c_str());
            return EXIT_FAILURE;
        }

        recreate_temporary_directory(base_path);

        initialize_logfile(opts, base_path);

        const file_direct_io_mode_t direct_io_mode = parse_direct_io_mode_option(opts);

        bool result;
        run_in_thread_pool(std::bind(&run_rethinkdb_create, base_path,
                                     machine_name,
                                     direct_io_mode,
                                     max_concurrent_io_requests,
                                     &result),
                           num_workers);

        if (result) {
            // Tell the directory lock that the directory is now good to go, as it will
            //  otherwise delete an uninitialized directory
            data_directory_lock.directory_initialized();
            return EXIT_SUCCESS;
        }
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help create' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help create' for help on the command\n");
    } catch (const std::exception &ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
    return EXIT_FAILURE;
}

bool maybe_daemonize(const std::map<std::string, options::values_t> &opts) {
    if (exists_option(opts, "--daemon")) {
        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error(strprintf("Failed to fork daemon: %s\n", errno_string(get_errno()).c_str()).c_str());
        }

        if (pid > 0) {
            return false;
        }

        umask(0);

        pid_t sid = setsid();
        if (sid == 0) {
            throw std::runtime_error(strprintf("Failed to create daemon session: %s\n", errno_string(get_errno()).c_str()).c_str());
        }

        if (chdir("/") < 0) {
            throw std::runtime_error(strprintf("Failed to change directory: %s\n", errno_string(get_errno()).c_str()).c_str());
        }

        if (freopen("/dev/null", "r", stdin) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stdin for daemon: %s\n", errno_string(get_errno()).c_str()).c_str());
        }
        if (freopen("/dev/null", "w", stdout) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stdin for daemon: %s\n", errno_string(get_errno()).c_str()).c_str());
        }
        if (freopen("/dev/null", "w", stderr) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stderr for daemon: %s\n", errno_string(get_errno()).c_str()).c_str());
        }
    }
    return true;
}

int main_rethinkdb_serve(int argc, char *argv[]) {
    std::vector<options::option_t> options;
    std::vector<options::help_section_t> help;
    get_rethinkdb_serve_options(&help, &options);

    try {
        std::map<std::string, options::values_t> opts = parse_commands_deep(argc - 2, argv + 2, options);

        if (handle_help_or_version_option(opts, &help_rethinkdb_serve)) {
            return EXIT_SUCCESS;
        }

        options::verify_option_counts(options, opts);

        set_user_group(opts);

        base_path_t base_path(get_single_option(opts, "--directory"));

        std::vector<host_and_port_t> joins = parse_join_options(opts, port_defaults::peer_port);

        service_address_ports_t address_ports = get_service_address_ports(opts);

        std::string web_path = get_web_path(opts, argv);

        int num_workers;
        if (!parse_cores_option(opts, &num_workers)) {
            return EXIT_FAILURE;
        }

        int max_concurrent_io_requests;
        if (!parse_io_threads_option(opts, &max_concurrent_io_requests)) {
            return EXIT_FAILURE;
        }

        uint64_t total_cache_size = get_total_cache_size(opts);

        // Open and lock the directory, but do not create it
        bool is_new_directory = false;
        directory_lock_t data_directory_lock(base_path, false, &is_new_directory);
        guarantee(!is_new_directory);

        recreate_temporary_directory(base_path);

        base_path.make_absolute();
        initialize_logfile(opts, base_path);

        if (check_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        if (!maybe_daemonize(opts)) {
            // This is the parent process of the daemon, just exit
            return EXIT_SUCCESS;
        }

        if (write_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        extproc_spawner_t extproc_spawner;

        serve_info_t serve_info(std::move(joins),
                                get_reql_http_proxy_option(opts),
                                std::move(web_path),
                                address_ports,
                                get_optional_option(opts, "--config-file"));

        const file_direct_io_mode_t direct_io_mode = parse_direct_io_mode_option(opts);

        bool result;
        run_in_thread_pool(std::bind(&run_rethinkdb_serve,
                                     base_path,
                                     &serve_info,
                                     direct_io_mode,
                                     max_concurrent_io_requests,
                                     total_cache_size,
                                     static_cast<machine_id_t*>(NULL),
                                     static_cast<cluster_semilattice_metadata_t*>(NULL),
                                     &data_directory_lock,
                                     &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help serve' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help serve' for help on the command\n");
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
    return EXIT_FAILURE;
}

int main_rethinkdb_admin(int argc, char *argv[]) {
    std::vector<options::help_section_t> help;
    std::vector<options::option_t> options;
    get_rethinkdb_admin_options(&help, &options);

    try {
        std::vector<std::string> command_args;
        std::map<std::string, options::values_t> opts
            = options::merge(options::parse_command_line_and_collect_unrecognized(argc - 2, argv + 2, options,
                                                                                  &command_args),
                             options::default_values_map(options));

        options::verify_option_counts(options, opts);

        std::vector<host_and_port_t> joins = parse_join_options(opts, port_defaults::peer_port);

        peer_address_t canonical_addresses = get_canonical_addresses(opts, 0);

        const int client_port = get_single_int(opts, "--client-port");

        const bool exit_on_failure = exists_option(opts, "--exit-failure");

        const int num_workers = get_cpu_count();

        bool result;
        run_in_thread_pool(std::bind(&run_rethinkdb_admin, joins, canonical_addresses, client_port, command_args, exit_on_failure, &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help admin' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help admin' for help on the command\n");
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
    return EXIT_FAILURE;
}

int main_rethinkdb_proxy(int argc, char *argv[]) {
    std::vector<options::option_t> options;
    std::vector<options::help_section_t> help;
    get_rethinkdb_proxy_options(&help, &options);

    try {
        std::map<std::string, options::values_t> opts = parse_commands_deep(argc - 2, argv + 2, options);

        if (handle_help_or_version_option(opts, &help_rethinkdb_proxy)) {
            return EXIT_SUCCESS;
        }

        options::verify_option_counts(options, opts);

        std::vector<host_and_port_t> joins = parse_join_options(opts, port_defaults::peer_port);

        service_address_ports_t address_ports = get_service_address_ports(opts);

        if (joins.empty()) {
            fprintf(stderr, "No --join option(s) given. A proxy needs to connect to something!\n"
                    "Run 'rethinkdb help proxy' for more information.\n");
            return EXIT_FAILURE;
        }

        set_user_group(opts);

        // Default to putting the log file in the current working directory
        base_path_t base_path(".");
        initialize_logfile(opts, base_path);

        std::string web_path = get_web_path(opts, argv);
        const int num_workers = get_cpu_count();

        if (check_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        if (!maybe_daemonize(opts)) {
            // This is the parent process of the daemon, just exit
            return EXIT_SUCCESS;
        }

        if (write_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        extproc_spawner_t extproc_spawner;

        serve_info_t serve_info(std::move(joins),
                                get_reql_http_proxy_option(opts),
                                std::move(web_path),
                                address_ports,
                                get_optional_option(opts, "--config-file"));

        bool result;
        run_in_thread_pool(std::bind(&run_rethinkdb_proxy, &serve_info, &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help proxy' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help proxy' for help on the command\n");
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
    return EXIT_FAILURE;
}

// TODO: Add split_db_table unit test.
MUST_USE bool split_db_table(const std::string &db_table, std::string *db_name_out, std::string *table_name_out) {
    size_t first_pos = db_table.find_first_of('.');
    if (first_pos == std::string::npos || db_table.find_last_of('.') != first_pos) {
        return false;
    }

    if (first_pos == 0 || first_pos + 1 == db_table.size()) {
        return false;
    }

    db_name_out->assign(db_table.data(), first_pos);
    table_name_out->assign(db_table.data() + first_pos + 1, db_table.data() + db_table.size());
    guarantee(db_name_out->size() > 0);
    guarantee(table_name_out->size() > 0);
    return true;
}

void run_backup_script(const std::string& script_name, char * const arguments[]) {
    int res = execvp(script_name.c_str(), arguments);
    if (res == -1) {

        fprintf(stderr, "Error when launching '%s': %s\n",
                script_name.c_str(), errno_string(get_errno()).c_str());
        fprintf(stderr, "The %s command depends on the RethinkDB Python driver, which must be installed.\n",
                script_name.c_str());
        fprintf(stderr, "If the Python driver is already installed, make sure that the PATH environment variable\n"
                "includes the location of the backup scripts, and that the current user has permission to\n"
                "access and run the scripts.\n"
                "Instructions for installing the RethinkDB Python driver are available here:\n"
                "http://www.rethinkdb.com/docs/install-drivers/python/\n");
    }
}

int main_rethinkdb_export(int, char *argv[]) {
    run_backup_script(RETHINKDB_EXPORT_SCRIPT, argv + 1);
    return EXIT_FAILURE;
}

int main_rethinkdb_import(int, char *argv[]) {
    run_backup_script(RETHINKDB_IMPORT_SCRIPT, argv + 1);
    return EXIT_FAILURE;
}

int main_rethinkdb_dump(int, char *argv[]) {
    run_backup_script(RETHINKDB_DUMP_SCRIPT, argv + 1);
    return EXIT_FAILURE;
}

int main_rethinkdb_restore(int, char *argv[]) {
    run_backup_script(RETHINKDB_RESTORE_SCRIPT, argv + 1);
    return EXIT_FAILURE;
}

int main_rethinkdb_porcelain(int argc, char *argv[]) {
    std::vector<options::option_t> options;
    std::vector<options::help_section_t> help;
    get_rethinkdb_porcelain_options(&help, &options);

    try {
        std::map<std::string, options::values_t> opts = parse_commands_deep(argc - 1, argv + 1, options);

        if (handle_help_or_version_option(opts, help_rethinkdb_porcelain)) {
            return EXIT_SUCCESS;
        }

        options::verify_option_counts(options, opts);

        set_user_group(opts);

        base_path_t base_path(get_single_option(opts, "--directory"));

        std::string machine_name_str = get_single_option(opts, "--machine-name");
        name_string_t machine_name;
        if (!machine_name.assign_value(machine_name_str)) {
            fprintf(stderr, "ERROR: machine-name invalid.  (%s)\n", name_string_t::valid_char_msg);
            return EXIT_FAILURE;
        }

        std::vector<host_and_port_t> joins = parse_join_options(opts, port_defaults::peer_port);

        const service_address_ports_t address_ports = get_service_address_ports(opts);

        std::string web_path = get_web_path(opts, argv);

        int num_workers;
        if (!parse_cores_option(opts, &num_workers)) {
            return EXIT_FAILURE;
        }

        int max_concurrent_io_requests;
        if (!parse_io_threads_option(opts, &max_concurrent_io_requests)) {
            return EXIT_FAILURE;
        }

        uint64_t total_cache_size = get_total_cache_size(opts);

        // Attempt to create the directory early so that the log file can use it.
        // If we create the file, it will be cleaned up unless directory_initialized()
        // is called on it.  This will be done after the metadata files have been created.
        bool is_new_directory = false;
        directory_lock_t data_directory_lock(base_path, true, &is_new_directory);

        recreate_temporary_directory(base_path);

        base_path.make_absolute();
        initialize_logfile(opts, base_path);

        if (check_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        if (!maybe_daemonize(opts)) {
            // This is the parent process of the daemon, just exit
            // Make sure the parent process doesn't remove the directory,
            //  the child process will handle it from here
            data_directory_lock.directory_initialized();
            return EXIT_SUCCESS;
        }

        if (write_pid_file(opts) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        extproc_spawner_t extproc_spawner;

        serve_info_t serve_info(std::move(joins),
                                get_reql_http_proxy_option(opts),
                                std::move(web_path),
                                address_ports,
                                get_optional_option(opts, "--config-file"));

        const file_direct_io_mode_t direct_io_mode = parse_direct_io_mode_option(opts);

        bool result;
        run_in_thread_pool(std::bind(&run_rethinkdb_porcelain,
                                     base_path,
                                     machine_name,
                                     direct_io_mode,
                                     max_concurrent_io_requests,
                                     total_cache_size,
                                     is_new_directory,
                                     &serve_info,
                                     &data_directory_lock,
                                     &result),
                           num_workers);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help' for help on the command\n");
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }

    return EXIT_FAILURE;
}

void help_rethinkdb_porcelain() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_porcelain_options(&help_sections, &options);
    }

    printf("Running 'rethinkdb' will create a new data directory or use an existing one,\n");
    printf("  and serve as a RethinkDB cluster node.\n");
    printf("%s", format_help(help_sections).c_str());
    printf("\n");
    printf("There are a number of subcommands for more specific tasks:\n");
    printf("    'rethinkdb create': prepare files on disk for a new server instance\n");
    printf("    'rethinkdb serve': use an existing data directory to host data and serve queries\n");
    printf("    'rethinkdb proxy': serve queries from an existing cluster but don't host data\n");
    printf("    'rethinkdb admin': access and modify an existing cluster's metadata\n");
    printf("    'rethinkdb export': export data from an existing cluster into a file or directory\n");
    printf("    'rethinkdb import': import data from from a file or directory into an existing cluster\n");
    printf("    'rethinkdb dump': export and compress data from an existing cluster\n");
    printf("    'rethinkdb restore': import compressed data into an existing cluster\n");
    printf("\n");
    printf("For more information, run 'rethinkdb help [subcommand]'.\n");
}

void help_rethinkdb_create() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_create_options(&help_sections, &options);
    }

    printf("'rethinkdb create' is used to prepare a directory to act"
                " as the storage location for a RethinkDB cluster node.\n");
    printf("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_serve() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_serve_options(&help_sections, &options);
    }

    printf("'rethinkdb serve' is the actual process for a RethinkDB cluster node.\n");
    printf("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_proxy() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_proxy_options(&help_sections, &options);
    }

    printf("'rethinkdb proxy' serves as a proxy to an existing RethinkDB cluster.\n");
    printf("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_export() {
    char help_arg[] = "--help";
    char dummy_arg[] = RETHINKDB_EXPORT_SCRIPT;
    char* args[3] = { dummy_arg, help_arg, NULL };
    run_backup_script(RETHINKDB_EXPORT_SCRIPT, args);
}

void help_rethinkdb_import() {
    char help_arg[] = "--help";
    char dummy_arg[] = RETHINKDB_IMPORT_SCRIPT;
    char* args[3] = { dummy_arg, help_arg, NULL };
    run_backup_script(RETHINKDB_IMPORT_SCRIPT, args);
}

void help_rethinkdb_dump() {
    char help_arg[] = "--help";
    char dummy_arg[] = RETHINKDB_DUMP_SCRIPT;
    char* args[3] = { dummy_arg, help_arg, NULL };
    run_backup_script(RETHINKDB_DUMP_SCRIPT, args);
}

void help_rethinkdb_restore() {
    char help_arg[] = "--help";
    char dummy_arg[] = RETHINKDB_RESTORE_SCRIPT;
    char* args[3] = { dummy_arg, help_arg, NULL };
    run_backup_script(RETHINKDB_RESTORE_SCRIPT, args);
}
