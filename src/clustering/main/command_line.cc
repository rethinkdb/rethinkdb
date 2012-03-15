#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include "arch/arch.hpp"
#include "arch/os_signal.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/persist.hpp"
#include "clustering/main/command_line.hpp"
#include "clustering/main/serve.hpp"
#include "mock/dummy_protocol.hpp"

namespace po = boost::program_options;

static const int default_peer_port = 20300;

class host_and_port_t {
public:
    host_and_port_t(const std::string& h, int p) : host(h), port(p) { }
    std::string host;
    int port;
};

void run_rethinkdb_create(const std::string &filepath, bool *result_out) {

    if (metadata_persistence::check_existence(filepath)) {
        std::cout << "ERROR: The path '" << filepath << "' already exists. "
            "Delete it and try again." << std::endl;
        *result_out = false;
        return;
    }

    machine_id_t our_machine_id = generate_uuid();
    std::cout << "Our machine ID: " << our_machine_id << std::endl;

    cluster_semilattice_metadata_t metadata;
    metadata.machines.machines.insert(std::make_pair(
        our_machine_id,
        machine_semilattice_metadata_t()
        ));

    metadata_persistence::create(filepath, our_machine_id, metadata);

    std::cout << "Created directory '" << filepath << "' and a metadata file "
        "inside it." << std::endl;

    *result_out = true;
}

std::vector<peer_address_t> look_up_peers_addresses(std::vector<host_and_port_t> names) {
    std::vector<peer_address_t> peers;
    for (int i = 0; i < (int)names.size(); i++) {
        peers.push_back(peer_address_t(ip_address_t(names[i].host), names[i].port));
    }
    return peers;
}

void run_rethinkdb_serve(const std::string &filepath, const std::vector<host_and_port_t> &joins, int port, bool *result_out) {

    os_signal_cond_t c;

    if (!metadata_persistence::check_existence(filepath)) {
        std::cout << "ERROR: The directory '" << filepath << "' does not "
            "exist. Run 'rethinkdb create -d \"" << filepath << "\"' and try "
            "again." << std::endl;
        *result_out = false;
        return;
    }

    machine_id_t persisted_machine_id;
    cluster_semilattice_metadata_t persisted_semilattice_metadata;

    try {
        metadata_persistence::read(filepath, &persisted_machine_id, &persisted_semilattice_metadata);
    } catch (metadata_persistence::file_exc_t e) {
        std::cout << "ERROR: Could not read metadata file: " << e.what() << ". "
            << std::endl;
        *result_out = false;
        return;
    }

    *result_out = serve(filepath, look_up_peers_addresses(joins), port, persisted_machine_id, persisted_semilattice_metadata);
}

void run_rethinkdb_porcelain(const std::string &filepath, const std::vector<host_and_port_t> &joins, int port, bool *result_out) {

    os_signal_cond_t c;

    std::cout << "Checking if directory '" << filepath << "' already "
        "exists..." << std::endl;
    if (metadata_persistence::check_existence(filepath)) {
        std::cout << "It already exists. Loading data..." << std::endl;

        machine_id_t persisted_machine_id;
        cluster_semilattice_metadata_t persisted_semilattice_metadata;
        metadata_persistence::read(filepath, &persisted_machine_id, &persisted_semilattice_metadata);

        *result_out = serve(filepath, look_up_peers_addresses(joins), port, persisted_machine_id, persisted_semilattice_metadata);

    } else {
        std::cout << "It does not already exist. Creating it..." << std::endl;

        machine_id_t our_machine_id = generate_uuid();
        cluster_semilattice_metadata_t semilattice_metadata;

        if (joins.empty()) {
            std::cout << "Creating a default namespace and default data center "
                "for your convenience. (This is because you ran 'rethinkdb' "
                "without 'create', 'serve', or '--join', and the directory '" <<
                filepath << "' did not already exist.)" << std::endl;

            datacenter_id_t datacenter_id = generate_uuid();
            datacenter_semilattice_metadata_t datacenter_metadata;
            datacenter_metadata.name = vclock_t<std::string>("Welcome", our_machine_id);
            semilattice_metadata.datacenters.datacenters.insert(std::make_pair(
                datacenter_id,
                deletable_t<datacenter_semilattice_metadata_t>(datacenter_metadata)
                ));

            /* Add ourselves as a member of the "Welcome" datacenter. */
            machine_semilattice_metadata_t our_machine_metadata;
            our_machine_metadata.datacenter = vclock_t<datacenter_id_t>(datacenter_id, our_machine_id);
            semilattice_metadata.machines.machines.insert(std::make_pair(
                our_machine_id,
                deletable_t<machine_semilattice_metadata_t>(our_machine_metadata)
                ));

            namespace_id_t namespace_id = generate_uuid();
            namespace_semilattice_metadata_t<memcached_protocol_t> namespace_metadata;

            namespace_metadata.name = vclock_t<std::string>("Welcome", our_machine_id);

            persistable_blueprint_t<memcached_protocol_t> blueprint;
            std::map<key_range_t, blueprint_details::role_t> roles;
            roles.insert(std::make_pair(key_range_t::entire_range(), blueprint_details::role_primary));
            blueprint.machines_roles.insert(std::make_pair(our_machine_id, roles));
            namespace_metadata.blueprint = vclock_t<persistable_blueprint_t<memcached_protocol_t> >(blueprint, our_machine_id);

            namespace_metadata.primary_datacenter = vclock_t<datacenter_id_t>(datacenter_id, our_machine_id);

            std::map<datacenter_id_t, int> affinities;
            affinities.insert(std::make_pair(datacenter_id, 0));
            namespace_metadata.replica_affinities = vclock_t<std::map<datacenter_id_t, int> >(affinities, our_machine_id);

            std::set<key_range_t> shards;
            shards.insert(key_range_t::entire_range());
            namespace_metadata.shards = vclock_t<std::set<key_range_t> >(shards, our_machine_id);

            semilattice_metadata.memcached_namespaces.namespaces.insert(std::make_pair(namespace_id, namespace_metadata));

        } else {
            semilattice_metadata.machines.machines.insert(std::make_pair(
                our_machine_id,
                machine_semilattice_metadata_t()
                ));
        }

        metadata_persistence::create(filepath, our_machine_id, semilattice_metadata);

        *result_out = serve(filepath, look_up_peers_addresses(joins), port, our_machine_id, semilattice_metadata);
    }
}

po::options_description get_file_option() {
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

po::options_description get_network_options() {
    po::options_description desc("Network options");
    desc.add_options()
        ("port", po::value<int>()->default_value(default_peer_port), "port for communicating with other nodes")
        ("join,j", po::value<std::vector<host_and_port_t> >()->composing(), "host:port of a node that we will connect to");
    return desc;
}

po::options_description get_rethinkdb_create_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_option());
    return desc;
}

po::options_description get_rethinkdb_serve_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_option());
    desc.add(get_network_options());
    return desc;
}

po::options_description get_rethinkdb_porcelain_options() {
    po::options_description desc("Allowed options");
    desc.add(get_file_option());
    desc.add(get_network_options());
    return desc;
}

int main_rethinkdb_create(int argc, char *argv[]) {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, get_rethinkdb_create_options()), vm);
    po::notify(vm);

    std::string filepath = vm["directory"].as<std::string>();

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_create, filepath, &result));

    return result ? 0 : 1;
}

int main_rethinkdb_serve(int argc, char *argv[]) {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, get_rethinkdb_serve_options()), vm);
    po::notify(vm);

    std::string filepath = vm["directory"].as<std::string>();
    std::vector<host_and_port_t> joins;
    if (vm.count("join") > 0) {
        joins = vm["join"].as<std::vector<host_and_port_t> >();
    }
    int port = vm["port"].as<int>();

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_serve, filepath, joins, port, &result));

    return result ? 0 : 1;
}

int main_rethinkdb_porcelain(int argc, char *argv[]) {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, get_rethinkdb_porcelain_options()), vm);
    po::notify(vm);

    std::string filepath = vm["directory"].as<std::string>();
    std::vector<host_and_port_t> joins;
    if (vm.count("join") > 0) {
        joins = vm["join"].as<std::vector<host_and_port_t> >();
    }
    int port = vm["port"].as<int>();

    bool result;
    run_in_thread_pool(boost::bind(&run_rethinkdb_porcelain, filepath, joins, port, &result));

    return result ? 0 : 1;
}

void help_rethinkdb_create() {
    std::cout << "'rethinkdb create' is used to prepare a directory to act "
        "as the storage location for a RethinkDB cluster node." << std::endl;
    std::cout << get_rethinkdb_create_options() << std::endl;
}

void help_rethinkdb_serve() {
    std::cout << "'rethinkdb serve' is the actual process for a RethinkDB "
        "cluster node." << std::endl;
    std::cout << get_rethinkdb_serve_options() << std::endl;
}
