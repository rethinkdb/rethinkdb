// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/command_line.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>  // NOLINT(readability/streams). Needed for use with program_options.  Sorry.

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "arch/arch.hpp"
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

namespace po = boost::program_options;

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

int write_pid_file(const po::variables_map& vm) {
    if (vm.count("pid-file") && vm["pid-file"].as<std::string>().length()) {
        // Be very careful about modifying this. It is important that the code that removes the
        // pid-file only run if the checks here pass. Right now, this is guaranteed by the return on
        // failure here.
        if (!access(vm["pid-file"].as<std::string>().c_str(), F_OK)) {
            fprintf(stderr, "ERROR: The pid-file specified already exists. This might mean that an instance is already running.\n");
            return EXIT_FAILURE;
        }
        if (!numwrite(vm["pid-file"].as<std::string>().c_str(), getpid())) {
            fprintf(stderr, "ERROR: Writing to the specified pid-file failed.\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void remove_pid_file(const po::variables_map& vm) {
    if (vm.count("pid-file") && vm["pid-file"].as<std::string>().length()) {
        remove(vm["pid-file"].as<std::string>().c_str());
    }
}

class host_and_port_t {
public:
    host_and_port_t(const std::string& h, int p) : host(h), port(p) { }
    std::string host;
    int port;
};

std::string metadata_file(const std::string& file_path) {
    return file_path + "/metadata";
}

std::string get_logfilepath(const std::string& file_path) {
    return file_path + "/log_file";
}

bool check_existence(const std::string& file_path) {
    return 0 == access(file_path.c_str(), F_OK);
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
        result.nodes.push_back("web");
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

void run_rethinkdb_create(const std::string &filepath, const name_string_t &machine_name, const io_backend_t io_backend, bool *result_out) {
    machine_id_t our_machine_id = generate_uuid();
    logINF("Our machine ID: %s\n", uuid_to_str(our_machine_id).c_str());

    cluster_semilattice_metadata_t metadata;

    machine_semilattice_metadata_t machine_semilattice_metadata;
    machine_semilattice_metadata.name = machine_semilattice_metadata.name.make_new_version(machine_name, our_machine_id);
    machine_semilattice_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
    metadata.machines.machines.insert(std::make_pair(our_machine_id, machine_semilattice_metadata));

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(io_backend, &io_backender);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    try {
        metadata_persistence::persistent_file_t store(io_backender.get(), metadata_file(filepath), &metadata_perfmon_collection, our_machine_id, metadata);
        logINF("Created directory '%s' and a metadata file inside it.\n", filepath.c_str());
        *result_out = true;
    } catch (const metadata_persistence::file_in_use_exc_t &ex) {
        logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
        *result_out = false;
    }
}

peer_address_set_t look_up_peers_addresses(std::vector<host_and_port_t> names) {
    peer_address_set_t peers;
    for (size_t i = 0; i < names.size(); ++i) {
        peers.insert(peer_address_t(ip_address_t::from_hostname(names[i].host),
                                    names[i].port));
    }
    return peers;
}

void run_rethinkdb_admin(const std::vector<host_and_port_t> &joins, int client_port, const std::vector<std::string>& command_args, bool exit_on_failure, bool *result_out) {
    os_signal_cond_t sigint_cond;
    *result_out = true;
    std::string host_port;

    if (!joins.empty())
        host_port = strprintf("%s:%d", joins[0].host.c_str(), joins[0].port);

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

void run_rethinkdb_serve(extproc::spawner_t::info_t *spawner_info,
                         const std::string &filepath,
                         const std::vector<host_and_port_t> &joins,
                         service_address_ports_t address_ports,
                         const io_backend_t io_backend,
                         bool *result_out,
                         std::string web_assets) {
    os_signal_cond_t sigint_cond;

    if (!check_existence(filepath)) {
        logERR("ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", filepath.c_str(), filepath.c_str());
        *result_out = false;
        return;
    }

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(io_backend, &io_backender);

    perfmon_collection_t metadata_perfmon_collection;
    perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");

    try {
        metadata_persistence::persistent_file_t store(io_backender.get(), metadata_file(filepath), &metadata_perfmon_collection);

        *result_out = serve(spawner_info,
                            io_backender.get(),
                            filepath, &store,
                            look_up_peers_addresses(joins),
                            address_ports,
                            store.read_machine_id(),
                            store.read_metadata(),
                            web_assets,
                            &sigint_cond);
    } catch (const metadata_persistence::file_in_use_exc_t &ex) {
        logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
        *result_out = false;
    } catch (const host_lookup_exc_t &ex) {
        logERR("%s\n", ex.what());
        *result_out = false;
    }
}

void run_rethinkdb_porcelain(extproc::spawner_t::info_t *spawner_info,
                             const std::string &filepath,
                             const name_string_t &machine_name,
                             const std::vector<host_and_port_t> &joins,
                             service_address_ports_t address_ports,
                             const io_backend_t io_backend,
                             bool *result_out,
                             std::string web_assets,
                             bool new_directory) {
    logINF("Running %s...\n", RETHINKDB_VERSION_STR);
    os_signal_cond_t sigint_cond;

    if (!new_directory) {
        logINF("Loading data from directory %s\n", filepath.c_str());

        scoped_ptr_t<io_backender_t> io_backender;
        make_io_backender(io_backend, &io_backender);

        perfmon_collection_t metadata_perfmon_collection;
        perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");
        try {
            metadata_persistence::persistent_file_t store(io_backender.get(), metadata_file(filepath), &metadata_perfmon_collection);

            *result_out = serve(spawner_info,
                                io_backender.get(),
                                filepath, &store,
                                look_up_peers_addresses(joins),
                                address_ports,
                                store.read_machine_id(), store.read_metadata(),
                                web_assets,
                                &sigint_cond);
        } catch (const metadata_persistence::file_in_use_exc_t &ex) {
            logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
            *result_out = false;
        } catch (const host_lookup_exc_t &ex) {
            logERR("%s\n", ex.what());
            *result_out = false;
        }

    } else {
        logINF("Creating directory %s\n", filepath.c_str());

        machine_id_t our_machine_id = generate_uuid();

        cluster_semilattice_metadata_t semilattice_metadata;

        machine_semilattice_metadata_t our_machine_metadata;
        our_machine_metadata.name = vclock_t<name_string_t>(machine_name, our_machine_id);
        our_machine_metadata.datacenter = vclock_t<datacenter_id_t>(nil_uuid(), our_machine_id);
        semilattice_metadata.machines.machines.insert(std::make_pair(our_machine_id, our_machine_metadata));
        if (joins.empty()) {
            logINF("Creating a default database for your convenience. (This is because you ran 'rethinkdb' "
                   "without 'create', 'serve', or '--join', and the directory '%s' did not already exist.)\n",
                   filepath.c_str());

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

        scoped_ptr_t<io_backender_t> io_backender;
        make_io_backender(io_backend, &io_backender);

        perfmon_collection_t metadata_perfmon_collection;
        perfmon_membership_t metadata_perfmon_membership(&get_global_perfmon_collection(), &metadata_perfmon_collection, "metadata");
        try {
            metadata_persistence::persistent_file_t store(io_backender.get(), metadata_file(filepath), &metadata_perfmon_collection, our_machine_id, semilattice_metadata);

            *result_out = serve(spawner_info,
                                io_backender.get(),
                                filepath, &store,
                                look_up_peers_addresses(joins),
                                address_ports,
                                our_machine_id, semilattice_metadata,
                                web_assets,
                                &sigint_cond);
        } catch (const metadata_persistence::file_in_use_exc_t &ex) {
            logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
            *result_out = false;
        } catch (const host_lookup_exc_t &ex) {
            logERR("%s\n", ex.what());
            *result_out = false;
        }
    }
}

void run_rethinkdb_proxy(extproc::spawner_t::info_t *spawner_info,
                         const std::vector<host_and_port_t> &joins,
                         service_address_ports_t address_ports,
                         bool *result_out,
                         std::string web_assets) {
    os_signal_cond_t sigint_cond;
    guarantee(!joins.empty());

    try {
        *result_out = serve_proxy(spawner_info,
                                  look_up_peers_addresses(joins),
                                  address_ports,
                                  generate_uuid(), cluster_semilattice_metadata_t(),
                                  web_assets,
                                  &sigint_cond);
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

po::options_description get_rethinkdb_create_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options());
    desc.add(get_disk_options());
    return desc;
}

po::options_description get_rethinkdb_create_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_file_options());
    desc.add(get_machine_options_visible());
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
    return desc;
}

po::options_description get_rethinkdb_proxy_options() {
    po::options_description desc("Allowed options");
    desc.add(get_network_options());
    desc.add(get_web_options());
    desc.add(get_service_options());
    desc.add_options()
        ("log-file", po::value<std::string>()->default_value("log_file"), "specify log file");
    return desc;
}

po::options_description get_rethinkdb_proxy_options_visible() {
    po::options_description desc("Allowed options");
    desc.add(get_network_options());
    desc.add(get_web_options_visible());
    desc.add(get_service_options());
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
        po::store(po::parse_command_line(argc, argv, options), *vm);
    } catch (const po::multiple_occurrences& ex) {
        logERR("flag specified too many times\n");
        return false;
    } catch (const po::unknown_option& ex) {
        logERR("%s\n", ex.what());
        return false;
    } catch (const po::validation_error& ex) {
        logERR("%s\n", ex.what());
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
        logERR("flag specified too many times\n");
        conf_file.close();
        return false;
    } catch (const po::unknown_option& ex) {
        logERR("%s\n", ex.what());
        conf_file.close();
        return false;
    } catch (const po::validation_error& ex) {
        logERR("%s\n", ex.what());
        conf_file.close();
        return false;
    }
    return true;
}

MUST_USE bool parse_commands_deep(int argc, char *argv[], po::variables_map *vm, const po::options_description& options) {
    po::options_description opt2 = config_file_attach_wrapper(options);
    try {
        po::store(po::parse_command_line(argc, argv, opt2), *vm);
        if (vm->count("config-file") && (*vm)["config-file"].as<std::string>().length()) {
            if (!parse_config_file_flat((*vm)["config-file"].as<std::string>(), vm, options)) {
                return false;
            }
        }
        po::notify(*vm);
    } catch (const po::multiple_occurrences& ex) {
        logERR("flag specified too many times\n");
        return false;
    } catch (const po::unknown_option& ex) {
        logERR("%s\n", ex.what());
        return false;
    } catch (const po::validation_error& ex) {
        logERR("%s\n", ex.what());
        return false;
    }
    return true;
}

int main_rethinkdb_create(int argc, char *argv[]) {
    po::variables_map vm;
    if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_create_options())) {
        return EXIT_FAILURE;
    }

    io_backend_t io_backend;
    if (!pull_io_backend_option(vm, &io_backend)) {
        fprintf(stderr, "ERROR: selected io-backend is invalid or unsupported.\n");
        return EXIT_FAILURE;
    }

    std::string filepath = vm["directory"].as<std::string>();
    std::string logfilepath = get_logfilepath(filepath);

    std::string machine_name_str = vm["machine-name"].as<std::string>();
    name_string_t machine_name;
    if (!machine_name.assign_value(machine_name_str)) {
        fprintf(stderr, "ERROR: machine-name '%s' is invalid.  (%s)", machine_name_str.c_str(), name_string_t::valid_char_msg);
        return EXIT_FAILURE;
    }

    boost::system::error_code ec;
    bool new_directory = boost::filesystem::create_directories(filepath, ec);
    if (!new_directory) {
        if (ec)
            fprintf(stderr, "Could not create directory: %s\n", ec.message().c_str());
        else
            fprintf(stderr, "The path '%s' already exists.  Delete it and try again.\n", filepath.c_str());
        return EXIT_FAILURE;
    }

    install_fallback_log_writer(logfilepath);

    const int num_workers = get_cpu_count();

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_create, filepath, machine_name, io_backend, &result),
                       num_workers);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main_rethinkdb_serve(int argc, char *argv[]) {
    po::variables_map vm;
    if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_serve_options())) {
        return EXIT_FAILURE;
    }

    std::string filepath = vm["directory"].as<std::string>();
    std::string logfilepath = get_logfilepath(filepath);

    std::vector<host_and_port_t> joins;
    if (vm.count("join") > 0) {
        joins = vm["join"].as<std::vector<host_and_port_t> >();
    }

    service_address_ports_t address_ports;
    try {
        address_ports = get_service_address_ports(vm);
    } catch (const address_lookup_exc_t& ex) {
        fprintf(stderr, "ERROR: %s\n", ex.what());
        return EXIT_FAILURE;
    }

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

    if (!check_existence(filepath)) {
        fprintf(stderr, "ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", filepath.c_str(), filepath.c_str());
        return EXIT_FAILURE;
    }

    install_fallback_log_writer(logfilepath);

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_serve, &spawner_info, filepath, joins,
                                   address_ports,
                                   io_backend,
                                   &result, web_path),
                       num_workers);

    remove_pid_file(vm);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
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
        logERR("%s\n", ex.what());
        result = false;
    }

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main_rethinkdb_proxy(int argc, char *argv[]) {
    po::variables_map vm;
    if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_proxy_options())) {
        return EXIT_FAILURE;
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
    try {
        address_ports = get_service_address_ports(vm);
    } catch (const address_lookup_exc_t& ex) {
        fprintf(stderr, "ERROR: %s\n", ex.what());
        return EXIT_FAILURE;
    }

    std::string web_path = get_web_path(vm, argv);

    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);

    const int num_workers = get_cpu_count();

    if (write_pid_file(vm) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_proxy,
                                   &spawner_info,
                                   joins,
                                   address_ports,
                                   &result, web_path),
                       num_workers);

    remove_pid_file(vm);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
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

        // TODO: Does this not work with a zero count?
        if (vm.count("join") == 0) {
            fprintf(stderr, "--join HOST:PORT must be specified\n");
            return EXIT_FAILURE;
        }
        std::vector<host_and_port_t> joins;
        joins = vm["join"].as<std::vector<host_and_port_t> >();

        std::set<ip_address_t> local_addresses;
        try {
            local_addresses = get_local_addresses(vm);
        } catch (const address_lookup_exc_t& ex) {
            fprintf(stderr, "ERROR: %s\n", ex.what());
            return EXIT_FAILURE;
        }

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
    } catch (const std::exception& ex) {
        // TODO: Sigh.
        logERR("%s\n", ex.what());
        return EXIT_FAILURE;
    }
}


int main_rethinkdb_porcelain(int argc, char *argv[]) {
    try {
        po::variables_map vm;
        if (!parse_commands_deep(argc, argv, &vm, get_rethinkdb_porcelain_options())) {
            return EXIT_FAILURE;
        }

        std::string filepath = vm["directory"].as<std::string>();
        std::string logfilepath = get_logfilepath(filepath);

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
        try {
            address_ports = get_service_address_ports(vm);
        } catch (const address_lookup_exc_t& ex) {
            fprintf(stderr, "ERROR: %s\n", ex.what());
            return EXIT_FAILURE;
        }

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

        // Attempt to create the directory early so that the log file can use it.
        boost::system::error_code ec;
        bool new_directory = boost::filesystem::create_directories(filepath, ec);
        if (!new_directory) {
            if (ec) {
                fprintf(stderr, "Could not create directory: %s\n", ec.message().c_str());
                return EXIT_FAILURE;
            }
        }

        install_fallback_log_writer(logfilepath);

        bool result;
        run_in_thread_pool(boost::bind(&run_rethinkdb_porcelain,
                                       &spawner_info,
                                       filepath,
                                       machine_name,
                                       joins,
                                       address_ports,
                                       io_backend,
                                       &result,
                                       web_path,
                                       new_directory),
                           num_workers);

        remove_pid_file(vm);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const boost::program_options::error &e) {
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }
}

void help_rethinkdb_create() {
    printf("'rethinkdb create' is used to prepare a directory to act "
           "as the storage location for a RethinkDB cluster node.\n");
    std::stringstream sstream;
    sstream << get_rethinkdb_create_options_visible();
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_serve() {
    printf("'rethinkdb serve' is the actual process for a RethinkDB cluster node.\n");
    std::stringstream sstream;
    sstream << get_rethinkdb_serve_options_visible();
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_proxy() {
    printf("'rethinkdb proxy' serves as a proxy to an existing RethinkDB cluster.\n");
    std::stringstream sstream;
    sstream << get_rethinkdb_proxy_options_visible();
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_import() {
    printf("'rethinkdb import' imports content from a CSV file.\n");
    std::stringstream s;
    s << get_rethinkdb_import_options();
    printf("%s\n", s.str().c_str());
}
