#include "clustering/administration/main/command_line.hpp"

#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "arch/runtime/starter.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
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

std::string errno_to_string(int err) {
    char buffer[200];
    char *res = strerror_r(err, buffer, sizeof(buffer));
    return std::string(res);
}

bool check_existence(const std::string& file_path) {
    return 0 == access(file_path.c_str(), F_OK);
}

void run_rethinkdb_create(const std::string &filepath, const std::string &machine_name, const io_backend_t io_backend, bool *result_out) {
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

void run_rethinkdb_import(extproc::spawner_t::info_t *spawner_info, std::vector<host_and_port_t> joins, int client_port, json_import_target_t target, std::string separators, std::string input_filepath, bool *result_out) {
    os_signal_cond_t sigint_cond;
    guarantee(!joins.empty());

    csv_to_json_importer_t importer(separators, input_filepath);

    // TODO: Make the peer port be configurable?
    *result_out = run_json_import(spawner_info, look_up_peers_addresses(joins), 0, client_port, target, &importer, &sigint_cond);
}

void run_rethinkdb_serve(extproc::spawner_t::info_t *spawner_info, const std::string &filepath, const std::vector<host_and_port_t> &joins, service_ports_t ports, const io_backend_t io_backend, bool *result_out, std::string web_assets) {
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
                            ports,
                            store.read_machine_id(),
                            store.read_metadata(),
                            web_assets,
                            &sigint_cond);
    } catch (const metadata_persistence::file_in_use_exc_t &ex) {
        logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
        *result_out = false;
    }
}

void run_rethinkdb_porcelain(extproc::spawner_t::info_t *spawner_info, const std::string &filepath, const std::string &machine_name, const std::vector<host_and_port_t> &joins, service_ports_t ports, const io_backend_t io_backend, bool *result_out, std::string web_assets, bool new_directory) {
    os_signal_cond_t sigint_cond;

    logINF("Checking if directory '%s' already existed...\n", filepath.c_str());
    if (!new_directory) {
        logINF("It already existed.  Loading data...\n");

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
                                ports,
                                store.read_machine_id(), store.read_metadata(),
                                web_assets,
                                &sigint_cond);
        } catch (const metadata_persistence::file_in_use_exc_t &ex) {
            logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
            *result_out = false;
        }

    } else {
        logINF("It did not already exist. It has been created.\n");

        machine_id_t our_machine_id = generate_uuid();

        cluster_semilattice_metadata_t semilattice_metadata;

        machine_semilattice_metadata_t our_machine_metadata;
        our_machine_metadata.name = vclock_t<std::string>(machine_name, our_machine_id);
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
            bool db_name_success = db_name.assign("test");
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
                                ports,
                                our_machine_id, semilattice_metadata,
                                web_assets,
                                &sigint_cond);
        } catch (const metadata_persistence::file_in_use_exc_t &ex) {
            logINF("Directory '%s' is in use by another rethinkdb process.\n", filepath.c_str());
            *result_out = false;
        }
    }
}

void run_rethinkdb_proxy(extproc::spawner_t::info_t *spawner_info, const std::vector<host_and_port_t> &joins, service_ports_t ports, const io_backend_t io_backend, bool *result_out, std::string web_assets) {
    os_signal_cond_t sigint_cond;
    guarantee(!joins.empty());

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(io_backend, &io_backender);

    *result_out = serve_proxy(spawner_info,
                              io_backender.get(),
                              look_up_peers_addresses(joins),
                              ports,
                              generate_uuid(), cluster_semilattice_metadata_t(),
                              web_assets,
                              &sigint_cond);
}

po::options_description get_machine_options(UNUSED bool omit_hidden) {
    po::options_description desc("Machine name options");
    desc.add_options()
        ("machine-name,n", po::value<std::string>()->default_value("NN"), "The name for this machine (as will appear in the metadata).");
    return desc;
}

po::options_description get_file_options(UNUSED bool omit_hidden) {
    po::options_description desc("File path options");
    desc.add_options()
        ("directory,d", po::value<std::string>()->default_value("rethinkdb_cluster_data"), "specify directory to store data and metadata");
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
    if (colon_loc == std::string::npos) {
        throw po::validation_error(po::validation_error::invalid_option_value, word);
    } else {
        std::string host = word.substr(0, colon_loc);
        int port = atoi(word.substr(colon_loc + 1).c_str());
        if (host.size() == 0 || port == 0) {
            throw po::validation_error(po::validation_error::invalid_option_value, word);
        }
        value_out = host_and_port_t(host, port);
    }
}

po::options_description get_network_options(bool omit_hidden) {
    po::options_description desc("Network options");
    desc.add_options()
        ("cluster-port", po::value<int>()->default_value(port_defaults::peer_port), "port for receiving connections from other nodes")
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("http-port", po::value<int>()->default_value(port_defaults::http_port), "port for http admin console (defaults to `port + 1000`)")
        ("reql-port", po::value<int>()->default_value(port_defaults::reql_port), "port for rethinkdb protocol to be hosted on")
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to");

    if (!omit_hidden) {
        desc.add_options()("port-offset,o", po::value<int>()->default_value(port_defaults::port_offset), "set up parsers for namespaces on the namespace's port + this value (for development)");
    }
    return desc;
}

po::options_description get_disk_options(UNUSED bool omit_hidden) {
    po::options_description desc("Disk I/O options");
    desc.add_options()
        ("io-backend", po::value<std::string>()->default_value("pool"), "event backend to use: native or pool.  Defaults to native.");
    return desc;
}

po::options_description get_cpu_options(UNUSED bool omit_hidden) {
    po::options_description desc("CPU options");
    desc.add_options()
        ("cores,c", po::value<int>()->default_value(get_cpu_count()), "the number of cores to utilize");
    return desc;
}

po::options_description get_rethinkdb_create_options(bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add(get_file_options(omit_hidden));
    desc.add(get_machine_options(omit_hidden));
    desc.add(get_disk_options(omit_hidden));
    return desc;
}

po::options_description get_rethinkdb_serve_options(bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add(get_file_options(omit_hidden));
    desc.add(get_network_options(omit_hidden));
    desc.add(get_disk_options(omit_hidden));
    desc.add(get_cpu_options(omit_hidden));
    return desc;
}

po::options_description get_rethinkdb_proxy_options(bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add(get_network_options(omit_hidden));
    desc.add(get_disk_options(omit_hidden));
    desc.add_options()
        ("log-file", po::value<std::string>()->default_value("log_file"), "specify log file");
    return desc;
}

po::options_description get_rethinkdb_admin_options(bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add_options()
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to")
        ("exit-failure,x", po::value<bool>()->zero_tokens(), "exit with an error code immediately if a command fails");
    desc.add(get_disk_options(omit_hidden));
    // TODO: The admin client doesn't use the io-backend option!  So why are we calling get_disk_options()?
    return desc;
}

po::options_description get_rethinkdb_import_options(UNUSED bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add_options()
        DEBUG_ONLY(("client-port", po::value<int>()->default_value(port_defaults::client_port), "port to use when connecting to other nodes (for development)"))
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to")
        // Default value of empty string?  Because who knows what the fuck it returns with
        // no default value.  Or am I supposed to wade my way back into the
        // program_options documentation again?
        ("table", po::value<std::string>()->default_value(""), "the database and table into which to import, of the format 'database.table'")
        ("datacenter", po::value<std::string>()->default_value(""), "the datacenter into which to create a table")
        ("primary-key", po::value<std::string>()->default_value("id"), "the primary key to create a new table with, or expected primary key")
        ("separators,s", po::value<std::string>()->default_value("\t,"), "list of characters to be used as whitespace -- uses \"\\t,\" by default")
        ("input-file", po::value<std::string>()->default_value(""), "the csv input file");

    return desc;
}

po::options_description get_rethinkdb_porcelain_options(bool omit_hidden = false) {
    po::options_description desc("Allowed options");
    desc.add(get_file_options(omit_hidden));
    desc.add(get_machine_options(omit_hidden));
    desc.add(get_network_options(omit_hidden));
    desc.add(get_disk_options(omit_hidden));
    desc.add(get_cpu_options(omit_hidden));
    return desc;
}

// Returns true upon success.
MUST_USE bool pull_io_backend_option(const po::variables_map& vm, io_backend_t *out) {
    std::string io_backend = vm["io-backend"].as<std::string>();
    if (io_backend == "pool") {
        *out = aio_pool;
    } else if (io_backend == "native") {
        *out = aio_native;
    } else {
        return false;
    }

    return true;
}

MUST_USE bool parse_commands(int argc, char *argv[], po::variables_map *vm, const po::options_description& options) {
    try {
        po::store(po::parse_command_line(argc, argv, options), *vm);
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
    if (!parse_commands(argc, argv, &vm, get_rethinkdb_create_options())) {
        return EXIT_FAILURE;
    }

    io_backend_t io_backend;
    if (!pull_io_backend_option(vm, &io_backend)) {
        return EXIT_FAILURE;
    }

    std::string filepath = vm["directory"].as<std::string>();
    std::string logfilepath = get_logfilepath(filepath);

    std::string machine_name = vm["machine-name"].as<std::string>();

    const int num_workers = get_cpu_count();

    if (check_existence(filepath)) {
        fprintf(stderr, "The path '%s' already exists.  Delete it and try again.\n", filepath.c_str());
        return 1;
    }

    int res = mkdir(filepath.c_str(), 0755);
    if (res != 0) {
        fprintf(stderr, "Could not create directory: %s\n", errno_to_string(errno).c_str());
        return 1;
    }

    install_fallback_log_writer(logfilepath);

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_create, filepath, machine_name, io_backend, &result),
                       num_workers);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main_rethinkdb_serve(int argc, char *argv[]) {
    po::variables_map vm;
    if (!parse_commands(argc, argv, &vm, get_rethinkdb_serve_options())) {
        return EXIT_FAILURE;
    }

    std::string filepath = vm["directory"].as<std::string>();
    std::string logfilepath = get_logfilepath(filepath);

    std::vector<host_and_port_t> joins;
    if (vm.count("join") > 0) {
        joins = vm["join"].as<std::vector<host_and_port_t> >();
    }
    int port = vm["cluster-port"].as<int>();
    int http_port = vm["http-port"].as<int>();
#ifndef NDEBUG
    int client_port = vm["client-port"].as<int>();
#else
    int client_port = port_defaults::client_port;
#endif
    int reql_port = vm["reql-port"].as<int>();
    int port_offset = vm["port-offset"].as<int>();

    path_t web_path = parse_as_path(argv[0]);
    web_path.nodes.pop_back();
    web_path.nodes.push_back("web");


    io_backend_t io_backend;
    if (!pull_io_backend_option(vm, &io_backend)) {
        return EXIT_FAILURE;
    }

    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);

    const int num_workers = vm["cores"].as<int>();
    if (num_workers <= 0 || num_workers > MAX_THREADS) {
        fprintf(stderr, "ERROR: number specified for cores to utilize must be between 1 and %d\n", MAX_THREADS);
        return EXIT_FAILURE;
    }

    if (!check_existence(filepath)) {
        fprintf(stderr, "ERROR: The directory '%s' does not exist.  Run 'rethinkdb create -d \"%s\"' and try again.\n", filepath.c_str(), filepath.c_str());
        return EXIT_FAILURE;
    }

    install_fallback_log_writer(logfilepath);

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_serve, &spawner_info, filepath, joins,
                                   service_ports_t(port, client_port, http_port, reql_port, port_offset),
                                   io_backend,
                                   &result, render_as_path(web_path)),
                       num_workers);

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
    if (!parse_commands(argc, argv, &vm, get_rethinkdb_proxy_options())) {
        return EXIT_FAILURE;
    }

    if (!vm.count("join")) {
        printf("No --join option(s) given. A proxy needs to connect to something!\n"
               "Run 'rethinkdb proxy help' for more information.\n");
        return EXIT_FAILURE;
     }

    std::string logfilepath = vm["log-file"].as<std::string>();
    install_fallback_log_writer(logfilepath);

    std::vector<host_and_port_t> joins = vm["join"].as<std::vector<host_and_port_t> >();
    int port = vm["cluster-port"].as<int>();
    int http_port = vm["http-port"].as<int>();
#ifndef NDEBUG
    int client_port = vm["client-port"].as<int>();
#else
    int client_port = port_defaults::client_port;
#endif
    int reql_port = vm["reql-port"].as<int>();
    int port_offset = vm["port-offset"].as<int>();

    path_t web_path = parse_as_path(argv[0]);
    web_path.nodes.pop_back();
    web_path.nodes.push_back("web");


    io_backend_t io_backend;
    if (!pull_io_backend_option(vm, &io_backend)) {
        return EXIT_FAILURE;
    }

    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);

    const int num_workers = get_cpu_count();

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_proxy, &spawner_info, joins,
                                   service_ports_t(port, client_port, http_port, reql_port, port_offset),
                                   io_backend,
                                   &result, render_as_path(web_path)),
                       num_workers);

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
        std::vector<host_and_port_t> joins;
        if (vm.count("join") > 0) {
            joins = vm["join"].as<std::vector<host_and_port_t> >();
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
            printf("--table option should have format database_name.table_name\n");
            return EXIT_FAILURE;
        }

        name_string_t db_name;
        if (!db_name.assign(table_name_str)) {
            printf("--table: database name invalid (use A-Za-z0-9_), e.g. database_name.table_name\n");
        }

        name_string_t table_name;
        if (!table_name.assign(table_name_str)) {
            printf("--table: table name invalid (use A-Za-z0-9_), e.g. database_name.table_name\n");
            return EXIT_FAILURE;
        }

        std::string datacenter_name_arg = vm["datacenter"].as<std::string>();
        boost::optional<std::string> datacenter_name;
        if (!datacenter_name_arg.empty()) {
            datacenter_name = datacenter_name_arg;
        }

        std::string primary_key = vm["primary-key"].as<std::string>();

        std::string separators = vm["separators"].as<std::string>();
        if (separators.empty()) {
            return EXIT_FAILURE;
        }
        std::string input_filepath = vm["input-file"].as<std::string>();
        if (input_filepath.empty()) {
            printf("Please supply an --input-file option.\n");
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
        run_in_thread_pool(boost::bind(&run_rethinkdb_import, &spawner_info, joins, client_port,
                                       target, separators, input_filepath, &result),
                           num_workers);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception& ex) {
        // TODO: Sigh.
        logERR("%s\n", ex.what());
        return EXIT_FAILURE;
    }
}


int main_rethinkdb_porcelain(int argc, char *argv[]) {
    po::variables_map vm;
    if (!parse_commands(argc, argv, &vm, get_rethinkdb_porcelain_options())) {
        return EXIT_FAILURE;
    }

    std::string filepath = vm["directory"].as<std::string>();
    std::string logfilepath = get_logfilepath(filepath);

    std::string machine_name = vm["machine-name"].as<std::string>();
    std::vector<host_and_port_t> joins;
    if (vm.count("join") > 0) {
        joins = vm["join"].as<std::vector<host_and_port_t> >();
    }
    int port = vm["cluster-port"].as<int>();
    int http_port = vm["http-port"].as<int>();
#ifndef NDEBUG
    int client_port = vm["client-port"].as<int>();
#else
    int client_port = port_defaults::client_port;
#endif
    int reql_port = vm["reql-port"].as<int>();
    int port_offset = vm["port-offset"].as<int>();

    path_t web_path = parse_as_path(argv[0]);
    web_path.nodes.pop_back();
    web_path.nodes.push_back("web");


    io_backend_t io_backend;
    if (!pull_io_backend_option(vm, &io_backend)) {
        return EXIT_FAILURE;
    }

    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);

    const int num_workers = vm["cores"].as<int>();
    if (num_workers <= 0 || num_workers > MAX_THREADS) {
        fprintf(stderr, "ERROR: number specified for cores to utilize must be between 1 and %d\n", MAX_THREADS);
        return EXIT_FAILURE;
    }

    bool new_directory = false;
    // Attempt to create the directory early so that the log file can use it.
    if (!check_existence(filepath)) {
        new_directory = true;
        int mkdir_res = mkdir(filepath.c_str(), 0755);
        if (mkdir_res != 0) {
            fprintf(stderr, "Could not create directory: %s\n", errno_to_string(errno).c_str());
            return EXIT_FAILURE;
        }
    }

    install_fallback_log_writer(logfilepath);

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_porcelain, &spawner_info, filepath, machine_name, joins,
                                   service_ports_t(port, client_port, http_port, reql_port, port_offset),
                                   io_backend,
                                   &result, render_as_path(web_path), new_directory),
                       num_workers);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

void help_rethinkdb_create() {
    printf("'rethinkdb create' is used to prepare a directory to act "
           "as the storage location for a RethinkDB cluster node.\n");
    std::stringstream sstream;
#ifndef NDEBUG
    sstream << get_rethinkdb_create_options();
#else
    sstream << get_rethinkdb_create_options(true);
#endif
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_serve() {
    printf("'rethinkdb serve' is the actual process for a RethinkDB cluster node.\n");
    std::stringstream sstream;
#ifndef NDEBUG
    sstream << get_rethinkdb_serve_options();
#else
    sstream << get_rethinkdb_create_options(true);
#endif
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_proxy() {
    printf("'rethinkdb proxy' serves as a proxy to an existing RethinkDB cluster.\n");
    std::stringstream sstream;
#ifndef NDEBUG
    sstream << get_rethinkdb_proxy_options();
#else
    sstream << get_rethinkdb_proxy_options(true);
#endif
    printf("%s\n", sstream.str().c_str());
}

void help_rethinkdb_import() {
    printf("'rethinkdb import' imports content from a CSV file.\n");
    std::stringstream s;
#ifndef NDEBUG
    s << get_rethinkdb_import_options();
#else
    s << get_rethinkdb_import_options(true);
#endif
    printf("%s\n", s.str().c_str());
}
