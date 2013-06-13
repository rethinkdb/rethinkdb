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

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/disk.hpp"
#include "arch/os_signal.hpp"
#include "arch/runtime/starter.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/main/names.hpp"
#include "clustering/administration/main/import.hpp"
#include "clustering/administration/main/json_import.hpp"
#include "clustering/administration/main/options.hpp"
#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/serve.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/persist.hpp"
#include "logger.hpp"
#include "extproc/spawner.hpp"
#include "mock/dummy_protocol.hpp"
#include "utils.hpp"
#include "help.hpp"

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
                                               rungroup->c_str(), errno_string(errno).c_str()).c_str());
        }
        if (setgid(group_id) != 0) {
            throw std::runtime_error(strprintf("Could not set group to '%s': %s",
                                               rungroup->c_str(), errno_string(errno).c_str()).c_str());
        }
    }

    if (runuser) {
        int user_id;
        gid_t user_group_id;
        if (!get_user_ids(runuser->c_str(), &user_id, &user_group_id)) {
            throw std::runtime_error(strprintf("User '%s' not found: %s",
                                               runuser->c_str(), errno_string(errno).c_str()).c_str());
        }
        if (!rungroup) {
            // No group specified, use the user's group
            if (setgid(user_group_id) != 0) {
                throw std::runtime_error(strprintf("Could not use the group of user '%s': %s",
                                                   runuser->c_str(), errno_string(errno).c_str()).c_str());
            }
        }
        if (setuid(user_id) != 0) {
            throw std::runtime_error(strprintf("Could not set user account to '%s': %s",
                                               runuser->c_str(), errno_string(errno).c_str()).c_str());
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


class host_and_port_t {
public:
    host_and_port_t(const std::string& h, int p) : host(h), port(p) { }
    std::string host;
    int port;
};

class serve_info_t {
public:
    serve_info_t(extproc::spawner_info_t *_spawner_info,
                 const std::vector<host_and_port_t> &_joins,
                 service_address_ports_t _ports,
                 std::string _web_assets,
                 boost::optional<std::string> _config_file):
        spawner_info(_spawner_info),
        joins(&_joins),
        ports(_ports),
        web_assets(_web_assets),
        config_file(_config_file) { }

    extproc::spawner_info_t *spawner_info;
    const std::vector<host_and_port_t> *joins;
    service_address_ports_t ports;
    std::string web_assets;
    boost::optional<std::string> config_file;
};

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

bool check_existence(const base_path_t& base_path) {
    return 0 == access(base_path.path().c_str(), F_OK);
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
    boost::optional<std::string> web_static_directory = get_optional_option(opts, "--web-static-directory");
    return get_web_path(web_static_directory, argv);
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
    const std::vector<std::string> vector_filter = bind_options;
    std::set<ip_address_t> set_filter;
    bool all = false;

    // Scan through specified bind options
    for (size_t i = 0; i < vector_filter.size(); ++i) {
        if (vector_filter[i] == "all") {
            all = true;
        } else {
            // Verify that all specified addresses are valid ip addresses
            struct in_addr addr;
            if (inet_pton(AF_INET, vector_filter[i].c_str(), &addr) == 1) {
                if (addr.s_addr == INADDR_ANY) {
                    all = true;
                } else {
                    set_filter.insert(ip_address_t(addr));
                }
            } else {
                throw address_lookup_exc_t(strprintf("bind ip address '%s' could not be parsed", vector_filter[i].c_str()));
            }
        }
    }

    std::set<ip_address_t> result = ip_address_t::get_local_addresses(set_filter, all);

    // Make sure that all specified addresses were found
    for (std::set<ip_address_t>::iterator i = set_filter.begin(); i != set_filter.end(); ++i) {
        if (result.find(*i) == result.end()) {
            throw address_lookup_exc_t(strprintf("could not find bind ip address '%s'", i->as_dotted_decimal().c_str()));
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

service_address_ports_t get_service_address_ports(const std::map<std::string, options::values_t> &opts) {
    const int port_offset = get_single_int(opts, "--port-offset");
    return service_address_ports_t(get_local_addresses(all_options(opts, "--bind")),
                                   offseted_port(get_single_int(opts, "--cluster-port"), port_offset),
#ifndef NDEBUG
                                   get_single_int(opts, "--client-port"),
#else
                                   port_defaults::client_port,
#endif
                                   exists_option(opts, "--no-http-admin"),
                                   offseted_port(get_single_int(opts, "--http-port"), port_offset),
                                   offseted_port(get_single_int(opts, "--driver-port"), port_offset),
                                   port_offset);
}


void run_rethinkdb_create(const base_path_t &base_path, const name_string_t &machine_name,
                          bool *const result_out) {
    machine_id_t our_machine_id = generate_uuid();

    cluster_semilattice_metadata_t cluster_metadata;
    auth_semilattice_metadata_t auth_metadata;

    machine_semilattice_metadata_t machine_semilattice_metadata;
    machine_semilattice_metadata.name = machine_semilattice_metadata.name.make_new_version(machine_name, our_machine_id);
    machine_semilattice_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
    cluster_metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(machine_semilattice_metadata)));

    io_backender_t io_backender;

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

peer_address_set_t look_up_peers_addresses(const std::vector<host_and_port_t> &names) {
    peer_address_set_t peers;
    for (size_t i = 0; i < names.size(); ++i) {
        peers.insert(peer_address_t(ip_address_t::from_hostname(names[i].host),
                                    names[i].port));
    }
    return peers;
}

void run_rethinkdb_admin(const std::vector<host_and_port_t> &joins, int client_port, const std::vector<std::string>& command_args, bool exit_on_failure, bool *const result_out) {
    os_signal_cond_t sigint_cond;
    *result_out = true;
    std::string host_port;

    if (!joins.empty()) {
        host_port = strprintf("%s:%d", joins[0].host.c_str(), joins[0].port);
    }

    try {
        if (command_args.empty())
            admin_command_parser_t(host_port, look_up_peers_addresses(joins), client_port, &sigint_cond).run_console(exit_on_failure);
        else if (command_args[0] == admin_command_parser_t::complete_command)
            admin_command_parser_t(host_port, look_up_peers_addresses(joins), client_port, &sigint_cond).run_completion(command_args);
        else
            admin_command_parser_t(host_port, look_up_peers_addresses(joins), client_port, &sigint_cond).parse_and_run_command(command_args);
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

void run_rethinkdb_import(extproc::spawner_info_t *spawner_info,
                          std::vector<host_and_port_t> joins,
                          const std::set<ip_address_t> &local_addresses,
                          int client_port,
                          json_import_target_t target,
                          std::string separators,
                          std::string input_filepath,
                          bool *result_out) {
    os_signal_cond_t sigint_cond;
    guarantee(!joins.empty());

    csv_to_json_importer_t importer(separators, input_filepath);

    // TODO: Make the peer port be configurable?
    try {
        *result_out = run_json_import(spawner_info,
                                      look_up_peers_addresses(joins),
                                      local_addresses,
                                      0,
                                      client_port,
                                      target,
                                      &importer,
                                      &sigint_cond);
    } catch (const host_lookup_exc_t &ex) {
        logERR("%s\n", ex.what());
        *result_out = false;
    } catch (const interrupted_exc_t &ex) {
        // This is only ok if we were interrupted by SIGINT, anything else should have been caught elsewhere
        if (sigint_cond.is_pulsed()) {
            logERR("Interrupted\n");
        } else {
            throw;
        }
    }
}

std::string uname_msr() {
    char buf[1024];
    static const std::string unknown = "unknown operating system\n";
    FILE *out = popen("uname -msr", "r");
    if (!out) return unknown;
    if (!fgets(buf, sizeof(buf), out)) return unknown;
    pclose(out);
    return buf;
}

void run_rethinkdb_serve(const base_path_t &base_path,
                         const serve_info_t& serve_info,
                         const int max_concurrent_io_requests,
                         const machine_id_t *our_machine_id,
                         const cluster_semilattice_metadata_t *cluster_metadata,
                         bool *const result_out) {
    logINF("Running %s...\n", RETHINKDB_VERSION_STR);
    logINF("Running on %s", uname_msr().c_str());
    os_signal_cond_t sigint_cond;

    if (!check_existence(base_path)) {
        logERR("ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", base_path.path().c_str(), base_path.path().c_str());
        *result_out = false;
        return;
    }

    logINF("Loading data from directory %s\n", base_path.path().c_str());

    io_backender_t io_backender(max_concurrent_io_requests);

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

        *result_out = serve(serve_info.spawner_info,
                            &io_backender,
                            base_path,
                            cluster_metadata_file.get(),
                            auth_metadata_file.get(),
                            look_up_peers_addresses(*serve_info.joins),
                            serve_info.ports,
                            serve_info.web_assets,
                            &sigint_cond,
                            serve_info.config_file);

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
                             const int max_concurrent_io_requests,
                             const bool new_directory,
                             const serve_info_t &serve_info,
                             bool *const result_out) {
    if (!new_directory) {
        run_rethinkdb_serve(base_path, serve_info, max_concurrent_io_requests,
                            NULL, NULL,
                            result_out);
    } else {
        logINF("Creating directory %s\n", base_path.path().c_str());

        machine_id_t our_machine_id = generate_uuid();

        cluster_semilattice_metadata_t cluster_metadata;

        machine_semilattice_metadata_t our_machine_metadata;
        our_machine_metadata.name = vclock_t<name_string_t>(machine_name, our_machine_id);
        our_machine_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
        cluster_metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(our_machine_metadata)));

        if (serve_info.joins->empty()) {
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

        run_rethinkdb_serve(base_path, serve_info,
                            max_concurrent_io_requests,
                            &our_machine_id, &cluster_metadata, result_out);
    }
}

void run_rethinkdb_proxy(const serve_info_t &serve_info, bool *const result_out) {
    os_signal_cond_t sigint_cond;
    guarantee(!serve_info.joins->empty());

    try {
        *result_out = serve_proxy(serve_info.spawner_info,
                                  look_up_peers_addresses(*serve_info.joins),
                                  serve_info.ports,
                                  serve_info.web_assets,
                                  &sigint_cond,
                                  serve_info.config_file);
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
    return help;
}

options::help_section_t get_config_file_options(std::vector<options::option_t> *options_out) {
    options::help_section_t help("Configuration file options");
    options_out->push_back(options::option_t(options::names_t("--config-file"),
                                             options::OPTIONAL));
    help.add("--config-file", "take options from a configuration file");
    return help;
}

host_and_port_t parse_host_and_port(const std::string &source, const std::string &option_name,
                                    const std::string &value) {
    size_t colon_loc = value.find_first_of(':');
    if (colon_loc != std::string::npos) {
        std::string host = value.substr(0, colon_loc);
        int port = atoi(value.substr(colon_loc + 1).c_str());
        if (host.size() != 0 && port != 0) {
            return host_and_port_t(host, port);
        }
    }

    throw options::value_error_t(source, option_name,
                                 strprintf("Option '%s' has invalid host and port number '%s'",
                                           option_name.c_str(), value.c_str()));
}

std::vector<host_and_port_t> parse_join_options(const std::map<std::string, options::values_t> &opts) {
    std::string source;
    const std::vector<std::string> join_strings = all_options(opts, "--join", &source);
    std::vector<host_and_port_t> joins;
    for (auto it = join_strings.begin(); it != join_strings.end(); ++it) {
        joins.push_back(parse_host_and_port(source, "--join", *it));
    }
    return joins;
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

#ifndef NDEBUG
    options_out->push_back(options::option_t(options::names_t("--client-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::client_port)));
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

#ifndef NDEBUG
    options_out->push_back(options::option_t(options::names_t("--client-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::client_port)));
    help.add("--client-port port", "port to use when connecting to other nodes (for development)");
#endif  // NDEBUG

    options_out->push_back(options::option_t(options::names_t("--join", "-j"),
                                             options::OPTIONAL_REPEAT));
    help.add("-j [ --join ] host:port", "host and port of a rethinkdb node to connect to");

    options_out->push_back(options::option_t(options::names_t("--exit-failure", "-x"),
                                             options::OPTIONAL_NO_PARAMETER));
    help.add("-x [ --exit-failure ]", "exit with an error code immediately if a command fails");

    help_out->push_back(help);
}

void get_rethinkdb_import_options(std::vector<options::help_section_t> *help_out,
                                  std::vector<options::option_t> *options_out) {
    options::help_section_t help("Allowed options");

#ifndef NDEBUG
    options_out->push_back(options::option_t(options::names_t("--client-port"),
                                             options::OPTIONAL,
                                             strprintf("%d", port_defaults::client_port)));
    help.add("--client-port port", "port to use when connecting to other nodes (for development)");
#endif  // NDEBUG

    options_out->push_back(options::option_t(options::names_t("--join", "-j"),
                                             options::OPTIONAL_REPEAT));
    help.add("-j [ --join ] host:port", "host and port of a rethinkdb node to connect to");

    options_out->push_back(options::option_t(options::names_t("--table"),
                                             options::MANDATORY));
    help.add("--table db_name.table_name", "the database and table into which to import");

    options_out->push_back(options::option_t(options::names_t("--datacenter"),
                                             options::OPTIONAL));
    help.add("--datacenter name", "the datacenter into which to create a table");

    options_out->push_back(options::option_t(options::names_t("--primary-key"),
                                             options::OPTIONAL,
                                             "id"));
    help.add("--primary-key key", "the primary key to create a new table with, or expected primary key");

    options_out->push_back(options::option_t(options::names_t("--separators", "-s"),
                                             options::OPTIONAL,
                                             "\t,"));
    help.add("-s [ --separators ]", "list of characters to be used as whitespace -- uses tabs and commas by default");

    options_out->push_back(options::option_t(options::names_t("--input-file"),
                                             options::MANDATORY));
    help.add("--input-file path", "the csv input file");

    help_out->push_back(help);
    help_out->push_back(get_config_file_options(options_out));
    help_out->push_back(get_help_options(options_out));
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

    return options::parse_config_file(file, config_filepath,
                                      options);
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

        const int num_workers = get_cpu_count();

        // TODO: Why do we call check_existence when we just try calling mkdir anyway?  This is stupid.
        if (check_existence(base_path)) {
            fprintf(stderr, "The path '%s' already exists.  Delete it and try again.\n", base_path.path().c_str());
            return EXIT_FAILURE;
        }

        const int res = mkdir(base_path.path().c_str(), 0755);
        if (res != 0) {
            fprintf(stderr, "Could not create directory: %s\n", errno_string(errno).c_str());
            return EXIT_FAILURE;
        }

        recreate_temporary_directory(base_path);

        initialize_logfile(opts, base_path);

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_create, base_path, machine_name, &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
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
            throw std::runtime_error(strprintf("Failed to fork daemon: %s\n", errno_string(errno).c_str()).c_str());
        }

        if (pid > 0) {
            return false;
        }

        umask(0);

        pid_t sid = setsid();
        if (sid == 0) {
            throw std::runtime_error(strprintf("Failed to create daemon session: %s\n", errno_string(errno).c_str()).c_str());
        }

        if (chdir("/") < 0) {
            throw std::runtime_error(strprintf("Failed to change directory: %s\n", errno_string(errno).c_str()).c_str());
        }

        if (freopen("/dev/null", "r", stdin) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stdin for daemon: %s\n", errno_string(errno).c_str()).c_str());
        }
        if (freopen("/dev/null", "w", stdout) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stdin for daemon: %s\n", errno_string(errno).c_str()).c_str());
        }
        if (freopen("/dev/null", "w", stderr) == NULL) {
            throw std::runtime_error(strprintf("Failed to redirect stderr for daemon: %s\n", errno_string(errno).c_str()).c_str());
        }
    }
    return true;
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

        const std::vector<host_and_port_t> joins = parse_join_options(opts);

        service_address_ports_t address_ports = get_service_address_ports(opts);

        const std::string web_path = get_web_path(opts, argv);

        int num_workers;
        if (!parse_cores_option(opts, &num_workers)) {
            return EXIT_FAILURE;
        }

        int max_concurrent_io_requests;
        if (!parse_io_threads_option(opts, &max_concurrent_io_requests)) {
            return EXIT_FAILURE;
        }

        if (!check_existence(base_path)) {
            fprintf(stderr, "ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", base_path.path().c_str(), base_path.path().c_str());
            return EXIT_FAILURE;
        }

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

        extproc::spawner_info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                get_optional_option(opts, "--config-file"));

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_serve, base_path,
                                       serve_info,
                                       max_concurrent_io_requests,
                                       static_cast<machine_id_t*>(NULL),
                                       static_cast<cluster_semilattice_metadata_t*>(NULL),
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

        const std::vector<host_and_port_t> joins = parse_join_options(opts);

#ifndef NDEBUG
        const int client_port = get_single_int(opts, "--client-port");
#else
        const int client_port = port_defaults::client_port;
#endif
        const bool exit_on_failure = exists_option(opts, "--exit-failure");

        const int num_workers = get_cpu_count();

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_admin, joins, client_port, command_args, exit_on_failure, &result),
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

        const std::vector<host_and_port_t> joins = parse_join_options(opts);

        if (joins.empty()) {
            fprintf(stderr, "No --join option(s) given. A proxy needs to connect to something!\n"
                    "Run 'rethinkdb help proxy' for more information.\n");
            return EXIT_FAILURE;
        }

        set_user_group(opts);

        // Default to putting the log file in the current working directory
        base_path_t base_path(".");
        initialize_logfile(opts, base_path);

        service_address_ports_t address_ports = get_service_address_ports(opts);

        const std::string web_path = get_web_path(opts, argv);
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

        extproc::spawner_info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                get_optional_option(opts, "--config-file"));

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_proxy, serve_info, &result),
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

int main_rethinkdb_import(int argc, char *argv[]) {
    std::vector<options::option_t> options;
    std::vector<options::help_section_t> help;
    get_rethinkdb_import_options(&help, &options);

    try {
        std::map<std::string, options::values_t> opts = parse_commands_deep(argc - 2, argv + 2, options);

        if (handle_help_or_version_option(opts, &help_rethinkdb_import)) {
            return EXIT_SUCCESS;
        }

        options::verify_option_counts(options, opts);

        const std::vector<host_and_port_t> joins = parse_join_options(opts);

        if (joins.empty()) {
            fprintf(stderr, "No --join option(s) given. An import process needs to connect to something!\n"
                    "Run 'rethinkdb help import' for more information.\n");
            return EXIT_FAILURE;
        }


#ifndef NDEBUG
        int client_port = get_single_int(opts, "--client-port");
#else
        int client_port = port_defaults::client_port;
#endif
        std::string db_table = get_single_option(opts, "--table");
        std::string db_name_str;
        std::string table_name_str;
        if (!split_db_table(db_table, &db_name_str, &table_name_str)) {
            fprintf(stderr, "--table option should have format database_name.table_name\n");
            return EXIT_FAILURE;
        }

        name_string_t db_name;
        if (!db_name.assign_value(db_name_str)) {
            fprintf(stderr, "ERROR: database name invalid. (%s)  e.g. --table database_name.table_name\n", name_string_t::valid_char_msg);
        }

        name_string_t table_name;
        if (!table_name.assign_value(table_name_str)) {
            fprintf(stderr, "ERROR: table name invalid.  (%s)  e.g. database_name.table_name\n", name_string_t::valid_char_msg);
            return EXIT_FAILURE;
        }

        boost::optional<std::string> datacenter_name_arg = get_optional_option(opts, "--datacenter");
        boost::optional<name_string_t> datacenter_name;
        if (datacenter_name_arg) {
            name_string_t tmp;
            if (!tmp.assign_value(*datacenter_name_arg)) {
                fprintf(stderr, "ERROR: datacenter name invalid.  (%s)\n", name_string_t::valid_char_msg);
                return EXIT_FAILURE;
            }
            *datacenter_name = tmp;
        }

        std::string primary_key = get_single_option(opts, "--primary-key");

        std::string separators = get_single_option(opts, "--separators");
        if (separators.empty()) {
            return EXIT_FAILURE;
        }
        std::string input_filepath = get_single_option(opts, "--input-file");
        // TODO: Is there really any point to specially handling an empty string path?
        if (input_filepath.empty()) {
            fprintf(stderr, "Please supply a non-empty --input-file option.\n");
            return EXIT_FAILURE;
        }

        extproc::spawner_info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        json_import_target_t target;
        target.db_name = db_name;
        target.datacenter_name = datacenter_name;
        target.table_name = table_name;
        target.primary_key = primary_key;

        // Don't bind to any local addresses -- don't listen for any incoming connections.
        const std::set<ip_address_t> local_addresses;

        const int num_workers = get_cpu_count();
        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_import,
                                       &spawner_info,
                                       joins,
                                       local_addresses,
                                       client_port,
                                       target,
                                       separators,
                                       input_filepath,
                                       &result),
                           num_workers);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const options::named_error_t &ex) {
        output_named_error(ex, help);
        fprintf(stderr, "Run 'rethinkdb help import' for help on the command\n");
    } catch (const options::option_error_t &ex) {
        output_sourced_error(ex);
        fprintf(stderr, "Run 'rethinkdb help import' for help on the command\n");
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
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

        const std::vector<host_and_port_t> joins = parse_join_options(opts);

        const service_address_ports_t address_ports = get_service_address_ports(opts);

        const std::string web_path = get_web_path(opts, argv);

        int num_workers;
        if (!parse_cores_option(opts, &num_workers)) {
            return EXIT_FAILURE;
        }

        int max_concurrent_io_requests;
        if (!parse_io_threads_option(opts, &max_concurrent_io_requests)) {
            return EXIT_FAILURE;
        }

        bool new_directory = false;
        // Attempt to create the directory early so that the log file can use it.
        if (!check_existence(base_path)) {
            new_directory = true;
            int mkdir_res = mkdir(base_path.path().c_str(), 0755);
            if (mkdir_res != 0) {
                fprintf(stderr, "Could not create directory: %s\n", errno_string(errno).c_str());
                return EXIT_FAILURE;
            }
        }

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

        extproc::spawner_info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                get_optional_option(opts, "--config-file"));

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_porcelain,
                                       base_path,
                                       machine_name,
                                       max_concurrent_io_requests,
                                       new_directory,
                                       serve_info,
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

    help_pager_t help;
    help.pagef("Running 'rethinkdb' will create a new data directory or use an existing one,\n");
    help.pagef("  and serve as a RethinkDB cluster node.\n");
    help.pagef("%s", format_help(help_sections).c_str());
    help.pagef("\n");
    help.pagef("There are a number of subcommands for more specific tasks:\n");
    help.pagef("    'rethinkdb create': prepare files on disk for a new server instance\n");
    help.pagef("    'rethinkdb serve': use an existing data directory to host data and serve queries\n");
    help.pagef("    'rethinkdb proxy': serve queries from an existing cluster but don't host data\n");
    help.pagef("    'rethinkdb admin': access and modify an existing cluster's metadata\n");
    help.pagef("    'rethinkdb import': import data from from a file into an existing cluster\n");
    help.pagef("\n");
    help.pagef("For more information, run 'rethinkdb help [subcommand]'.\n");
}

void help_rethinkdb_create() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_create_options(&help_sections, &options);
    }

    help_pager_t help;
    help.pagef("'rethinkdb create' is used to prepare a directory to act"
                " as the storage location for a RethinkDB cluster node.\n");
    help.pagef("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_serve() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_serve_options(&help_sections, &options);
    }

    help_pager_t help;
    help.pagef("'rethinkdb serve' is the actual process for a RethinkDB cluster node.\n");
    help.pagef("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_proxy() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_proxy_options(&help_sections, &options);
    }

    help_pager_t help;
    help.pagef("'rethinkdb proxy' serves as a proxy to an existing RethinkDB cluster.\n");
    help.pagef("%s", format_help(help_sections).c_str());
}

void help_rethinkdb_import() {
    std::vector<options::help_section_t> help_sections;
    {
        std::vector<options::option_t> options;
        get_rethinkdb_import_options(&help_sections, &options);
    }

    help_pager_t help;
    help.pagef("'rethinkdb import' imports content from a CSV file.\n");
    help.pagef("%s", format_help(help_sections).c_str());
}
