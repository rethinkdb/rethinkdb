// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/main/command_line.hpp"

#include <ftw.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>  // NOLINT(readability/streams). Needed for use with program_options.  Sorry.

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "arch/io/disk.hpp"
#include "arch/os_signal.hpp"
#include "arch/runtime/starter.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/main/names.hpp"
#include "clustering/administration/main/import.hpp"
#include "clustering/administration/main/json_import.hpp"
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

namespace po = boost::program_options;

int delete_all_helper(const char *path, UNUSED const struct stat *ptr, UNUSED const int flag, UNUSED FTW *ftw) {
    int res = ::remove(path);
    guarantee_err(res == 0, "remove syscall failed");
    return 0;
}

void delete_all(const char *path) {
    // max_openfd is ignored on OS X (which claims the parameter specifies the maximum traversal
    // depth) and used by Linux to limit the number of file descriptors that are open (by opening
    // and closing directories extra times if it needs to go deeper than that).
    const int max_openfd = 128;
    int res = nftw(path, delete_all_helper, max_openfd, FTW_PHYS | FTW_MOUNT | FTW_DEPTH);
    guarantee_err(res == 0 || errno == ENOENT, "Trouble while traversing and destroying temporary directory %s.", path);
}

std::string temporary_directory_path(const base_path_t& base_path) {
    return base_path.path() + "/tmp";
}

void recreate_temporary_directory(const base_path_t& base_path) {
    const std::string path = temporary_directory_path(base_path);

    delete_all(path.c_str());

    int res;
    do {
        res = mkdir(path.c_str(), 0755);
    } while (res == -1 && errno == EINTR);
    guarantee_err(res == 0, "mkdir of temporary directory %s failed", path.c_str());
}

template <typename T>
static inline boost::optional<T> optional_variable_value(const po::variable_value& var){
    return var.empty() ?
        boost::optional<T>() :
        boost::optional<T>(var.as<T>());
}

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

static const char *pid_file = NULL;

void remove_pid_file() {
    if (pid_file) {
        remove(pid_file);
        pid_file = NULL;
    }
}

// write_pid_file registers remove_pid_file with atexit.
// Always call it after spawner_t::create to ensure correct behaviour.
int write_pid_file(const po::variables_map& vm) {
    if (vm.count("pid-file") && vm["pid-file"].as<std::string>().length()) {
        // Be very careful about modifying this. It is important that the code that removes the
        // pid-file only run if the checks here pass. Right now, this is guaranteed by the return on
        // failure here.
        if (pid_file) {
            fprintf(stderr, "ERROR: Attempting to write pid-file twice.\n");
            return EXIT_FAILURE;
        }
        if (!access(vm["pid-file"].as<std::string>().c_str(), F_OK)) {
            fprintf(stderr, "ERROR: The pid-file specified already exists. This might mean that an instance is already running.\n");
            return EXIT_FAILURE;
        }
        if (!numwrite(vm["pid-file"].as<std::string>().c_str(), getpid())) {
            fprintf(stderr, "ERROR: Writing to the specified pid-file failed.\n");
            return EXIT_FAILURE;
        }
        pid_file = vm["pid-file"].as<std::string>().c_str();
        atexit(remove_pid_file);
    }
    return EXIT_SUCCESS;
}

class host_and_port_t {
public:
    host_and_port_t(const std::string& h, int p) : host(h), port(p) { }
    std::string host;
    int port;
};

class serve_info_t {
public:
    serve_info_t(extproc::spawner_t::info_t *_spawner_info,
                 const std::vector<host_and_port_t> &_joins,
                 service_address_ports_t _ports,
                 std::string _web_assets,
                 boost::optional<std::string> _config_file):
        spawner_info(_spawner_info),
        joins(&_joins),
        ports(_ports),
        web_assets(_web_assets),
        config_file(_config_file) { }

    extproc::spawner_t::info_t *spawner_info;
    const std::vector<host_and_port_t> *joins;
    service_address_ports_t ports;
    std::string web_assets;
    boost::optional<std::string> config_file;
};

serializer_filepath_t metadata_file(const base_path_t& dirpath) {
    return serializer_filepath_t(dirpath, "metadata");
}

std::string get_logfilepath(const base_path_t& dirpath) {
    return dirpath.path() + "/log_file";
}

bool check_existence(const base_path_t& base_path) {
    return 0 == access(base_path.path().c_str(), F_OK);
}

std::string get_web_path(const po::variables_map& vm, char *argv[]) {
    // We check first for a run-time option, then check the home of the binary,
    //  and then we check in the install location if such a location was provided at compile time.
    path_t result;
    if (vm.count("web-static-directory")) {
        result = parse_as_path(vm["web-static-directory"].as<std::string>());
    } else {
        result = parse_as_path(argv[0]);
        result.nodes.pop_back();
        result.nodes.push_back(WEB_ASSETS_DIR_NAME);
        #ifdef WEBRESDIR
        std::string chkdir(WEBRESDIR);
        if ((access(render_as_path(result).c_str(), F_OK)) && (!access(chkdir.c_str(), F_OK))) {
            result = parse_as_path(chkdir);
        }
        #endif
    }

    return render_as_path(result);
}

class address_lookup_exc_t : public std::exception {
public:
    explicit address_lookup_exc_t(const std::string& data) : info(data) { }
    ~address_lookup_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

std::set<ip_address_t> get_local_addresses(const po::variables_map& vm) {
    std::vector<std::string> vector_filter;
    if (vm.count("bind") > 0) {
        vector_filter = vm["bind"].as<std::vector<std::string> >();
    }
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
                set_filter.insert(ip_address_t(addr));
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

service_address_ports_t get_service_address_ports(const po::variables_map& vm) {
    // serve
    int cluster_port = vm["cluster-port"].as<int>();
    int http_port = vm["http-port"].as<int>();
    bool http_admin_is_disabled = vm.count("no-http-admin") > 0;
#ifndef NDEBUG
    int cluster_client_port = vm["client-port"].as<int>();
#else
    int cluster_client_port = port_defaults::client_port;
#endif
    // int reql_port = vm["reql-port"].as<int>();
    int reql_port = vm["driver-port"].as<int>();
    int port_offset = vm["port-offset"].as<int>();

    if (cluster_port != 0) {
        cluster_port += port_offset;
    }

    if (http_port != 0) {
        http_port += port_offset;
    }

    if (reql_port != 0) {
        reql_port += port_offset;
    }

    return service_address_ports_t(
        get_local_addresses(vm), cluster_port, cluster_client_port,
        http_admin_is_disabled, http_port, reql_port, port_offset);
}

void run_rethinkdb_create(const base_path_t &base_path, const name_string_t &machine_name, const io_backend_t io_backend, bool *const result_out) {
    machine_id_t our_machine_id = generate_uuid();
    logINF("Our machine ID: %s\n", uuid_to_str(our_machine_id).c_str());

    cluster_semilattice_metadata_t metadata;

    machine_semilattice_metadata_t machine_semilattice_metadata;
    machine_semilattice_metadata.name = machine_semilattice_metadata.name.make_new_version(machine_name, our_machine_id);
    machine_semilattice_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
    metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(machine_semilattice_metadata)));

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(io_backend, &io_backender);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    try {
        metadata_persistence::persistent_file_t store(io_backender.get(), metadata_file(base_path), &metadata_perfmon_collection, our_machine_id, metadata);
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
        fprintf(stderr, "valid --join option required to handle command, run 'rethinkdb admin help' for more information\n");
        *result_out = false;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        *result_out = false;
    }
}

void run_rethinkdb_import(extproc::spawner_t::info_t *spawner_info,
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
                         const io_backend_t io_backend,
                         const serve_info_t& serve_info,
                         const machine_id_t *our_machine_id,
                         const cluster_semilattice_metadata_t *semilattice_metadata,
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

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(io_backend, &io_backender);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    try {
        scoped_ptr_t<metadata_persistence::persistent_file_t> store;
        if (our_machine_id && semilattice_metadata) {
            store.init(new metadata_persistence::persistent_file_t(io_backender.get(),
                                                                   metadata_file(base_path),
                                                                   &metadata_perfmon_collection,
                                                                   *our_machine_id,
                                                                   *semilattice_metadata));
        } else {
            store.init(new metadata_persistence::persistent_file_t(io_backender.get(),
                                                                   metadata_file(base_path),
                                                                   &metadata_perfmon_collection));
        }

        *result_out = serve(serve_info.spawner_info,
                            io_backender.get(),
                            base_path, store.get(),
                            look_up_peers_addresses(*serve_info.joins),
                            serve_info.ports,
                            store->read_machine_id(),
                            store->read_metadata(),
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
                             const io_backend_t io_backend,
                             const bool new_directory,
                             const serve_info_t &serve_info,
                             bool *const result_out) {
    if (!new_directory) {
        run_rethinkdb_serve(base_path, io_backend, serve_info, NULL, NULL, result_out);
    } else {
        logINF("Creating directory %s\n", base_path.path().c_str());

        machine_id_t our_machine_id = generate_uuid();

        cluster_semilattice_metadata_t semilattice_metadata;

        machine_semilattice_metadata_t our_machine_metadata;
        our_machine_metadata.name = vclock_t<name_string_t>(machine_name, our_machine_id);
        our_machine_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
        semilattice_metadata.machines.machines.insert(std::make_pair(our_machine_id, make_deletable(our_machine_metadata)));
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
            semilattice_metadata.databases.databases.insert(std::make_pair(
                database_id,
                deletable_t<database_semilattice_metadata_t>(database_metadata)));
        }

        run_rethinkdb_serve(base_path, io_backend, serve_info,
                            &our_machine_id, &semilattice_metadata, result_out);
    }
}

void run_rethinkdb_proxy(const serve_info_t &serve_info, bool *const result_out) {
    os_signal_cond_t sigint_cond;
    guarantee(!serve_info.joins->empty());

    try {
        *result_out = serve_proxy(serve_info.spawner_info,
                                  look_up_peers_addresses(*serve_info.joins),
                                  serve_info.ports,
                                  generate_uuid(), cluster_semilattice_metadata_t(),
                                  serve_info.web_assets,
                                  &sigint_cond,
                                  serve_info.config_file);
    } catch (const host_lookup_exc_t &ex) {
        logERR("%s\n", ex.what());
        *result_out = false;
    }
}

po::options_description get_machine_options() {
    po::options_description desc("Machine name options");
    desc.add_options()
        ("machine-name,n", po::value<std::string>()->default_value(get_random_machine_name()), "the name for this machine (as will appear in the metadata).");
    return desc;
}

// A separate version of this for help since we don't have fine-tuned control over default options blah blah blah
po::options_description get_machine_options_visible() {
    po::options_description desc("Machine name options");
    desc.add_options()
        ("machine-name,n", po::value<std::string>(), "the name for this machine (as will appear in the metadata).  If not specified, it will be randomly chosen from a short list of names.");
    return desc;
}

po::options_description get_file_options() {
    po::options_description desc("File path options");
    desc.add_options()
        ("directory,d", po::value<std::string>()->default_value("rethinkdb_data"), "specify directory to store data and metadata");
    return desc;
}

po::options_description get_config_file_options() {
    po::options_description desc("Configuration file options");
    desc.add_options()
        ("config-file", po::value<std::string>(), "take options from a configuration file");
    return desc;
}

po::options_description config_file_attach_wrapper(po::options_description desc) {
    desc.add(get_config_file_options());
    return desc;
}

/* This allows `host_and_port_t` to be used as a command-line argument with
`boost::program_options`. */
void validate(boost::any& value_out, const std::vector<std::string>& words,
        host_and_port_t *, int)
{
    po::validators::check_first_occurrence(value_out);
    const std::string& word = po::validators::get_single_string(words);
    size_t colon_loc = word.find_first_of(':');
    if (colon_loc != std::string::npos) {
        std::string host = word.substr(0, colon_loc);
        int port = atoi(word.substr(colon_loc + 1).c_str());
        if (host.size() != 0 && port != 0) {
            value_out = host_and_port_t(host, port);
            return;
        }
    }

#if BOOST_VERSION >= 104200
    throw po::validation_error(po::validation_error::invalid_option_value, word);
#else
    throw po::validation_error("Invalid option value: " + word);
#endif
}

po::options_description get_web_options() {
    po::options_description desc("Web options");
    desc.add_options()
        ("web-static-directory", po::value<std::string>(), "specify directory from which to serve web resources")
        ("http-port", po::value<int>()->default_value(port_defaults::http_port), "port for http admin console")
        ("no-http-admin", "disable http admin console");
    return desc;
}

po::options_description get_web_options_visible() {
    po::options_description desc("Web options");
    desc.add_options()
        ("http-port", po::value<int>()->default_value(port_defaults::http_port), "port for http admin console")
        ("no-http-admin", "disable http admin console");
    return desc;
}

po::options_description get_network_options() {
    po::options_description desc("Network options");
    desc.add_options()
        ("bind", po::value<std::vector<std::string> >()->composing(), "add the address of a local interface to listen on when accepting connections, may be 'all' or an IP address, loopback addresses are enabled by default")
        ("cluster-port", po::value<int>()->default_value(port_defaults::peer_port), "port for receiving connections from other nodes")
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("driver-port", po::value<int>()->default_value(port_defaults::reql_port), "port for rethinkdb protocol for client drivers")
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to")
        ("port-offset,o", po::value<int>()->default_value(port_defaults::port_offset), "all ports used locally will have this value added");
    return desc;
}

po::options_description get_disk_options() {
    po::options_description desc("Disk I/O options");
    desc.add_options()
        ("io-backend", po::value<std::string>()->default_value("pool"), "event backend to use: native or pool.");
    return desc;
}

po::options_description get_cpu_options() {
    po::options_description desc("CPU options");
    desc.add_options()
        ("cores,c", po::value<int>()->default_value(get_cpu_count()), "the number of cores to utilize");
    return desc;
}

po::options_description get_service_options() {
    po::options_description desc("Service options");
    desc.add_options()
        ("pid-file", po::value<std::string>(), "specify a file in which to stash the pid when the process is running");
    return desc;
}

po::options_description get_help_options() {
    po::options_description desc("Help options");
    desc.add_options()
        ("help,h", "show command-line options instead of running rethinkdb");
    return desc;
}

po::options_description get_rethinkdb_create_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options());
    desc.add(get_disk_options());
    desc.add(get_help_options());
    return desc;
}

po::options_description get_rethinkdb_create_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options_visible());
    desc.add(get_help_options());
#ifdef AIOSUPPORT
    desc.add(get_disk_options());
#endif // AIOSUPPORT
    return desc;
}

po::options_description get_rethinkdb_serve_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_network_options());
    desc.add(get_web_options());
    desc.add(get_disk_options());
    desc.add(get_cpu_options());
    desc.add(get_service_options());
    desc.add(get_help_options());
    return desc;
}

po::options_description get_rethinkdb_serve_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_network_options());
    desc.add(get_web_options_visible());
#ifdef AIOSUPPORT
    desc.add(get_disk_options());
#endif // AIOSUPPORT
    desc.add(get_cpu_options());
    desc.add(get_service_options());
    desc.add(get_help_options());
    return desc;
}

po::options_description get_rethinkdb_proxy_options() {
    po::options_description desc("Allowed options");
    desc.add(get_network_options());
    desc.add(get_web_options());
    desc.add(get_service_options());
    desc.add(get_help_options());
    desc.add_options()
        ("log-file", po::value<std::string>()->default_value("log_file"), "specify log file");
    return desc;
}

po::options_description get_rethinkdb_proxy_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_network_options());
    desc.add(get_web_options_visible());
    desc.add(get_service_options());
    desc.add(get_help_options());
    desc.add_options()
        ("log-file", po::value<std::string>()->default_value("log_file"), "specify log file");
    return desc;
}

po::options_description get_rethinkdb_admin_options() {
    po::options_description desc("Allowed options");
    desc.add_options()
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to")
        ("exit-failure,x", po::value<bool>()->zero_tokens(), "exit with an error code immediately if a command fails");
    return desc;
}

po::options_description get_rethinkdb_import_options() {
    po::options_description desc("Allowed options");
    desc.add_options()
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to")
        // Default value of empty string?  Because who knows what the duck it returns with
        // no default value.  Or am I supposed to wade my way back into the
        // program_options documentation again?
        // A default value is not required. One can check vm.count("thing") in order to determine whether the user has supplied the option. --Juggernaut
        ("table", po::value<std::string>()->default_value(""), "the database and table into which to import, of the format 'database.table'")
        ("datacenter", po::value<std::string>()->default_value(""), "the datacenter into which to create a table")
        ("primary-key", po::value<std::string>()->default_value("id"), "the primary key to create a new table with, or expected primary key")
        ("separators,s", po::value<std::string>()->default_value("\t,"), "list of characters to be used as whitespace -- uses \"\\t,\" by default")
        ("input-file", po::value<std::string>()->default_value(""), "the csv input file");
    desc.add(get_help_options());

    return desc;
}

po::options_description get_rethinkdb_porcelain_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options());
    desc.add(get_network_options());
    desc.add(get_web_options());
    desc.add(get_disk_options());
    desc.add(get_cpu_options());
    desc.add(get_service_options());
    desc.add(get_help_options());
    return desc;
}

po::options_description get_rethinkdb_porcelain_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options_visible());
    desc.add(get_network_options());
    desc.add(get_web_options_visible());
#ifdef AIOSUPPORT
    desc.add(get_disk_options());
#endif // AIOSUPPORT
    desc.add(get_cpu_options());
    desc.add(get_service_options());
    desc.add(get_help_options());
    return desc;
}

// Returns true upon success.
MUST_USE bool pull_io_backend_option(const po::variables_map& vm, io_backend_t *out) {
    std::string io_backend = vm["io-backend"].as<std::string>();
    if (io_backend == "pool") {
        *out = aio_pool;
    } else if (io_backend == "native") {
#ifdef AIOSUPPORT
        *out = aio_native;
#else
        return false;
#endif
    } else {
        return false;
    }

    return true;
}

MUST_USE bool parse_commands_flat(int argc, char *argv[], po::variables_map *vm, const po::options_description& options) {
    try {
        int style =
            po::command_line_style::default_style &
            ~po::command_line_style::allow_guessing;
        po::store(po::parse_command_line(argc, argv, options, style), *vm);
    } catch (const po::multiple_occurrences& ex) {
        fprintf(stderr, "flag specified too many times\n");
        return false;
    }
    return true;
}

MUST_USE bool parse_commands(int argc, char *argv[], po::variables_map *vm, const po::options_description& options) {
    if (parse_commands_flat(argc, argv, vm, options)) {
        po::notify(*vm);
    } else {
        return false;
    }
    return true;
}

MUST_USE bool parse_config_file_flat(const std::string & conf_file_name, po::variables_map *vm, const po::options_description& options) {

    if ((conf_file_name.length() == 0) || access(conf_file_name.c_str(), R_OK)) {
        return false;
    }
    std::ifstream conf_file(conf_file_name.c_str(), std::ifstream::in);
    if (conf_file.fail()) {
        return false;
    }
    try {
        po::store(po::parse_config_file(conf_file, options, true), *vm);
    } catch (const po::multiple_occurrences& ex) {
        fprintf(stderr, "flag specified too many times\n");
        conf_file.close();
        return false;
    } catch (...) {
        conf_file.close();
        throw;
    }
    return true;
}

MUST_USE bool parse_commands_deep(int argc, char *argv[], po::variables_map *vm, const po::options_description& options) {
    po::options_description opt2 = config_file_attach_wrapper(options);
    try {
        int style =
            po::command_line_style::default_style &
            ~po::command_line_style::allow_guessing;
        po::store(po::parse_command_line(argc, argv, opt2, style), *vm);
        if (vm->count("config-file") && (*vm)["config-file"].as<std::string>().length()) {
            if (!parse_config_file_flat((*vm)["config-file"].as<std::string>(), vm, options)) {
                return false;
            }
        }
        po::notify(*vm);
    } catch (const po::multiple_occurrences& ex) {
        fprintf(stderr, "flag specified too many times\n");
        return false;
    }
    return true;
}

int main_rethinkdb_create(int argc, char *argv[]) {
    try {
        po::variables_map vm;
        if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_create_options())) {
            return EXIT_FAILURE;
        }

        if (vm.count("help") > 0) {
            help_rethinkdb_create();
            return EXIT_SUCCESS;
        }

        io_backend_t io_backend;
        if (!pull_io_backend_option(vm, &io_backend)) {
            fprintf(stderr, "ERROR: selected io-backend is invalid or unsupported.\n");
            return EXIT_FAILURE;
        }

        const base_path_t base_path(vm["directory"].as<std::string>());
        std::string logfilepath = get_logfilepath(base_path);

        std::string machine_name_str = vm["machine-name"].as<std::string>();
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

        install_fallback_log_writer(logfilepath);

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_create, base_path, machine_name, io_backend, &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const boost::program_options::error &ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }
}

int main_rethinkdb_serve(int argc, char *argv[]) {
    try {
        po::variables_map vm;
        if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_serve_options())) {
            return EXIT_FAILURE;
        }

        if (vm.count("help") > 0) {
            help_rethinkdb_serve();
            return EXIT_SUCCESS;
        }

        const base_path_t base_path(vm["directory"].as<std::string>());
        std::string logfilepath = get_logfilepath(base_path);

        std::vector<host_and_port_t> joins;
        if (vm.count("join") > 0) {
            joins = vm["join"].as<std::vector<host_and_port_t> >();
        }

        service_address_ports_t address_ports;
            address_ports = get_service_address_ports(vm);

        std::string web_path = get_web_path(vm, argv);

        io_backend_t io_backend;
        if (!pull_io_backend_option(vm, &io_backend)) {
            fprintf(stderr, "ERROR: selected io-backend is invalid or unsupported.\n");
            return EXIT_FAILURE;
        }

        extproc::spawner_t::info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        const int num_workers = vm["cores"].as<int>();
        if (num_workers <= 0 || num_workers > MAX_THREADS) {
            fprintf(stderr, "ERROR: number specified for cores to utilize must be between 1 and %d\n", MAX_THREADS);
            return EXIT_FAILURE;
        }

        if (write_pid_file(vm) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        if (!check_existence(base_path)) {
            fprintf(stderr, "ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", base_path.path().c_str(), base_path.path().c_str());
            return EXIT_FAILURE;
        }

        recreate_temporary_directory(base_path);

        install_fallback_log_writer(logfilepath);

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                optional_variable_value<std::string>(vm["config-file"]));

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_serve, base_path,
                                       io_backend,
                                       serve_info,
                                       static_cast<machine_id_t*>(NULL),
                                       static_cast<cluster_semilattice_metadata_t*>(NULL),
                                       &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const boost::program_options::error &ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }
}

int main_rethinkdb_admin(int argc, char *argv[]) {
    bool result = false;
    po::variables_map vm;
    po::options_description options;
    options.add(get_rethinkdb_admin_options());

    try {
        po::command_line_parser parser(argc - 1, &argv[1]);
        parser.options(options);
        parser.allow_unregistered();
        po::parsed_options parsed = parser.run();
        po::store(parsed, vm);
        po::notify(vm);

        std::vector<host_and_port_t> joins;
        if (vm.count("join") > 0) {
            joins = vm["join"].as<std::vector<host_and_port_t> >();
        }
#ifndef NDEBUG
        int client_port = vm["client-port"].as<int>();
#else
        int client_port = port_defaults::client_port;
#endif
        bool exit_on_failure = false;
        if (vm.count("exit-failure") > 0)
            exit_on_failure = true;

        std::vector<std::string> cmd_args = po::collect_unrecognized(parsed.options, po::include_positional);

        // This is an ugly hack, but it seems boost will ignore an empty flag at the end, which is very useful for completions
        std::string last_arg(argv[argc - 1]);
        if (last_arg == "-" || last_arg == "--")
            cmd_args.push_back(last_arg);

        const int num_workers = get_cpu_count();
        run_in_thread_pool(boost::bind(&run_rethinkdb_admin, joins, client_port, cmd_args, exit_on_failure, &result),
                           num_workers);

    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        result = false;
    }

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main_rethinkdb_proxy(int argc, char *argv[]) {
    try {
        po::variables_map vm;
        if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_proxy_options())) {
            return EXIT_FAILURE;
        }

        if (vm.count("help") > 0) {
            help_rethinkdb_proxy();
            return EXIT_SUCCESS;
        }

        if (!vm.count("join")) {
            fprintf(stderr, "No --join option(s) given. A proxy needs to connect to something!\n"
                   "Run 'rethinkdb proxy help' for more information.\n");
            return EXIT_FAILURE;
         }

        std::string logfilepath = vm["log-file"].as<std::string>();
        install_fallback_log_writer(logfilepath);

        std::vector<host_and_port_t> joins = vm["join"].as<std::vector<host_and_port_t> >();

        service_address_ports_t address_ports;
        address_ports = get_service_address_ports(vm);

        std::string web_path = get_web_path(vm, argv);

        extproc::spawner_t::info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        const int num_workers = get_cpu_count();

        if (write_pid_file(vm) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                optional_variable_value<std::string>(vm["config-file"]));

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_proxy, serve_info, &result),
                           num_workers);
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const boost::program_options::error &ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }
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
    // TODO: On errors supply usage information?
    try {
        po::variables_map vm;
        if (!parse_commands(argc - 1, argv + 1, &vm, get_rethinkdb_import_options())) {
            return EXIT_FAILURE;
        }

        if (vm.count("help") > 0) {
            help_rethinkdb_import();
            return EXIT_SUCCESS;
        }

        // TODO: Does this not work with a zero count?
        if (vm.count("join") == 0) {
            fprintf(stderr, "--join HOST:PORT must be specified\n");
            return EXIT_FAILURE;
        }
        std::vector<host_and_port_t> joins;
        joins = vm["join"].as<std::vector<host_and_port_t> >();

        std::set<ip_address_t> local_addresses;
        local_addresses = get_local_addresses(vm);

#ifndef NDEBUG
        int client_port = vm["client-port"].as<int>();
#else
        int client_port = port_defaults::client_port;
#endif
        std::string db_table = vm["table"].as<std::string>();
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

        std::string datacenter_name_arg = vm["datacenter"].as<std::string>();
        boost::optional<name_string_t> datacenter_name;
        if (!datacenter_name_arg.empty()) {
            name_string_t tmp;
            if (!tmp.assign_value(datacenter_name_arg)) {
                fprintf(stderr, "ERROR: datacenter name invalid.  (%s)\n", name_string_t::valid_char_msg);
                return EXIT_FAILURE;
            }
            *datacenter_name = tmp;
        }

        std::string primary_key = vm["primary-key"].as<std::string>();

        std::string separators = vm["separators"].as<std::string>();
        if (separators.empty()) {
            return EXIT_FAILURE;
        }
        std::string input_filepath = vm["input-file"].as<std::string>();
        if (input_filepath.empty()) {
            fprintf(stderr, "Please supply an --input-file option.\n");
            return EXIT_FAILURE;
        }

        extproc::spawner_t::info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        json_import_target_t target;
        target.db_name = db_name;
        target.datacenter_name = datacenter_name;
        target.table_name = table_name;
        target.primary_key = primary_key;

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
    } catch (const boost::program_options::error &ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        // TODO: Sigh.
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }
}


int main_rethinkdb_porcelain(int argc, char *argv[]) {
    try {
        po::variables_map vm;
        if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_porcelain_options())) {
            return EXIT_FAILURE;
        }

        if (vm.count("help") > 0) {
            help_rethinkdb_porcelain();
            return EXIT_SUCCESS;
        }

        const base_path_t base_path(vm["directory"].as<std::string>());
        const std::string logfilepath = get_logfilepath(base_path);

        std::string machine_name_str = vm["machine-name"].as<std::string>();
        name_string_t machine_name;
        if (!machine_name.assign_value(machine_name_str)) {
            fprintf(stderr, "ERROR: machine-name invalid.  (%s)\n", name_string_t::valid_char_msg);
            return EXIT_FAILURE;
        }
        std::vector<host_and_port_t> joins;
        if (vm.count("join") > 0) {
            joins = vm["join"].as<std::vector<host_and_port_t> >();
        }

        service_address_ports_t address_ports;
        address_ports = get_service_address_ports(vm);

        std::string web_path = get_web_path(vm, argv);

        io_backend_t io_backend;
        if (!pull_io_backend_option(vm, &io_backend)) {
            fprintf(stderr, "ERROR: selected io-backend is invalid or unsupported.\n");
            return EXIT_FAILURE;
        }

        extproc::spawner_t::info_t spawner_info;
        extproc::spawner_t::create(&spawner_info);

        const int num_workers = vm["cores"].as<int>();
        if (num_workers <= 0 || num_workers > MAX_THREADS) {
            fprintf(stderr, "ERROR: number specified for cores to utilize must be between 1 and %d\n", MAX_THREADS);
            return EXIT_FAILURE;
        }

        if (write_pid_file(vm) != EXIT_SUCCESS) {
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

        install_fallback_log_writer(logfilepath);

        serve_info_t serve_info(&spawner_info, joins, address_ports, web_path,
                                optional_variable_value<std::string>(vm["config-file"]));


        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_porcelain,
                                       base_path,
                                       machine_name,
                                       io_backend,
                                       new_directory,
                                       serve_info,
                                       &result),
                           num_workers);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const boost::program_options::error &ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return EXIT_FAILURE;
    }
}

void help_rethinkdb_porcelain() {
    std::stringstream sstream;
    sstream << get_rethinkdb_porcelain_options_visible();
    help_pager_t help;
    help.pagef("Running 'rethinkdb' will create a new data directory or use an existing one,\n");
    help.pagef("  and serve as a RethinkDB cluster node.\n");
    help.pagef("%s\n", sstream.str().c_str());
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
    std::stringstream sstream;
    sstream << get_rethinkdb_create_options_visible();
    help_pager_t help;
    help.pagef("'rethinkdb create' is used to prepare a directory to act"
                " as the storage location for a RethinkDB cluster node.\n");
    help.pagef("%s\n", sstream.str().c_str());
}

void help_rethinkdb_serve() {
    std::stringstream sstream;
    sstream << get_rethinkdb_serve_options_visible();
    help_pager_t help;
    help.pagef("'rethinkdb serve' is the actual process for a RethinkDB cluster node.\n");
    help.pagef("%s\n", sstream.str().c_str());
}

void help_rethinkdb_proxy() {
    std::stringstream sstream;
    sstream << get_rethinkdb_proxy_options_visible();
    help_pager_t help;
    help.pagef("'rethinkdb proxy' serves as a proxy to an existing RethinkDB cluster.\n");
    help.pagef("%s\n", sstream.str().c_str());
}

void help_rethinkdb_import() {
    std::stringstream sstream;
    sstream << get_rethinkdb_import_options();
    help_pager_t help;
    help.pagef("'rethinkdb import' imports content from a CSV file.\n");
    help.pagef("%s\n", sstream.str().c_str());
}
