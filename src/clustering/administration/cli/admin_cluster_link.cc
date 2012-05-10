#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <stdarg.h>
#include <stdio.h>

#include <map>
#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/suggester.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"

// Truncate a uuid for easier user-interface
std::string admin_cluster_link_t::truncate_uuid(const boost::uuids::uuid& uuid) {
    return uuid_to_str(uuid).substr(0, uuid_output_length);
}

// TODO: this was copied from semilattice_http_app, unify them
void admin_cluster_link_t::fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;

    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it = cluster_metadata->machines.machines.begin();
            it != cluster_metadata->machines.machines.end();
            it++) {
        if (!it->second.is_deleted()) {
            machine_assignments[it->first] = it->second.get().datacenter.get();
        }
    }

    std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > reactor_directory;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager.get_root_view()->get();
    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator it = directory.begin(); it != directory.end(); it++) {
        reactor_directory.insert(std::make_pair(it->first, it->second.memcached_namespaces));
        machine_id_translation_table.insert(std::make_pair(it->first, it->second.machine_id));
    }

    fill_in_blueprints_for_protocol<memcached_protocol_t>(&cluster_metadata->memcached_namespaces,
            reactor_directory,
            machine_id_translation_table,
            machine_assignments,
            connectivity_cluster.get_me().get_uuid());
}

// TODO: timeout connecting to cluster, throw admin_no_connection_exc_t if failed
admin_cluster_link_t::admin_cluster_link_t(const std::set<peer_address_t> &joins, int client_port) :
    local_issue_tracker(),
    log_writer("./rethinkdb_log_file", &local_issue_tracker), // TODO: come up with something else for this file
    connectivity_cluster(),
    message_multiplexer(&connectivity_cluster),
    mailbox_manager_client(&message_multiplexer, 'M'),
    mailbox_manager(&mailbox_manager_client),
    stat_manager(&mailbox_manager),
    log_server(&mailbox_manager, &log_writer),
    mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),
    semilattice_manager_client(&message_multiplexer, 'S'),
    semilattice_manager_cluster(&semilattice_manager_client, cluster_semilattice_metadata_t()),
    semilattice_manager_client_run(&semilattice_manager_client, &semilattice_manager_cluster),
    directory_manager_client(&message_multiplexer, 'D'),
    our_directory_metadata(cluster_directory_metadata_t(connectivity_cluster.get_me().get_uuid(), std::vector<std::string>(), stat_manager.get_address(), log_server.get_business_card())),
    directory_read_manager(connectivity_cluster.get_connectivity_service()),
    directory_write_manager(&directory_manager_client, our_directory_metadata.get_watchable()),
    directory_manager_client_run(&directory_manager_client, &directory_read_manager),
    message_multiplexer_run(&message_multiplexer),
    connectivity_cluster_run(&connectivity_cluster, 0, &message_multiplexer_run, client_port),
    semilattice_metadata(semilattice_manager_cluster.get_root_view()),
    issue_aggregator(),
    remote_issue_tracker(
        directory_read_manager.get_root_view()->subview(
            field_getter_t<std::list<clone_ptr_t<local_issue_t> >, cluster_directory_metadata_t>(&cluster_directory_metadata_t::local_issues)),
        directory_read_manager.get_root_view()->subview( 
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))
        ),
    remote_issue_tracker_feed(&issue_aggregator, &remote_issue_tracker),
    machine_down_issue_tracker(semilattice_metadata,
        directory_read_manager.get_root_view()->subview(field_getter_t<machine_id_t, cluster_directory_metadata_t>(&cluster_directory_metadata_t::machine_id))),
    machine_down_issue_tracker_feed(&issue_aggregator, &machine_down_issue_tracker),
    name_conflict_issue_tracker(semilattice_metadata),
    name_conflict_issue_tracker_feed(&issue_aggregator, &name_conflict_issue_tracker),
    vector_clock_conflict_issue_tracker(semilattice_metadata),
    vector_clock_issue_tracker_feed(&issue_aggregator, &vector_clock_conflict_issue_tracker),
    mc_pinnings_shards_mismatch_issue_tracker(metadata_field(&cluster_semilattice_metadata_t::memcached_namespaces, semilattice_metadata)),
    mc_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &mc_pinnings_shards_mismatch_issue_tracker),
    dummy_pinnings_shards_mismatch_issue_tracker(metadata_field(&cluster_semilattice_metadata_t::dummy_namespaces, semilattice_metadata)),
    dummy_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &dummy_pinnings_shards_mismatch_issue_tracker),
    unsatisfiable_goals_issue_tracker(semilattice_metadata),
    unsatisfiable_goals_issue_tracker_feed(&issue_aggregator, &unsatisfiable_goals_issue_tracker)
{
    boost::scoped_ptr<initial_joiner_t> initial_joiner;
    if (!joins.empty()) {
        initial_joiner.reset(new initial_joiner_t(&connectivity_cluster, &connectivity_cluster_run, joins));
        initial_joiner->get_ready_signal()->wait_lazily_unordered();   /* TODO: Listen for `SIGINT`? */
    }

    std::set<peer_id_t> peers = connectivity_cluster.get_peers_list();
    for (std::set<peer_id_t>::iterator i = peers.begin(); i != peers.end(); ++i)
        if (*i != connectivity_cluster.get_me())
            sync_peer = *i;
    rassert(!sync_peer.is_nil());
}

template <class T>
void admin_cluster_link_t::add_subset_to_uuid_path_map(const std::string& base, T& data_map) {
    std::vector<std::string> base_path;
    base_path.push_back(base);
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        std::string uuid(uuid_to_str(i->first));
        std::vector<std::string> current_path(base_path);
        current_path.push_back(uuid);
        uuid_to_path.insert(std::make_pair<std::string, std::vector<std::string> >(uuid, current_path));
    }
}

void admin_cluster_link_t::init_uuid_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata) {
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
    uuid_to_path.clear();
    add_subset_to_uuid_path_map("machines", cluster_metadata.machines.machines);
    add_subset_to_uuid_path_map("datacenters", cluster_metadata.datacenters.datacenters);
    add_subset_to_uuid_path_map("dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
    add_subset_to_uuid_path_map("memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
}

template <class T>
void admin_cluster_link_t::add_subset_to_name_path_map(const std::string& base, T& data_map) {
    std::vector<std::string> base_path;
    base_path.push_back(base);
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        std::string name(i->second.get().name.get());
        std::string uuid(uuid_to_str(i->first));
        std::vector<std::string> current_path(base_path);
        current_path.push_back(uuid);
        name_to_path.insert(std::make_pair<std::string, std::vector<std::string> >(name, current_path));
    }
}

void admin_cluster_link_t::init_name_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata) {
    name_to_path.clear();
    add_subset_to_name_path_map("machines", cluster_metadata.machines.machines);
    add_subset_to_name_path_map("datacenters", cluster_metadata.datacenters.datacenters);
    add_subset_to_name_path_map("dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
    add_subset_to_name_path_map("memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
}

void admin_cluster_link_t::sync_from() {
    // cond_t interruptor;
    // semilattice_metadata->sync_from(sync_peer, &interruptor);
    semilattice_metadata = semilattice_manager_cluster.get_root_view();
    init_uuid_to_path_map(semilattice_metadata->get());
    init_name_to_path_map(semilattice_metadata->get());
}

void admin_cluster_link_t::sync_to() {
    cond_t interruptor;
    // semilattice_metadata->sync_to(sync_peer, &interruptor);
}

std::vector<std::string> admin_cluster_link_t::get_ids(const std::string& base) {
    std::vector<std::string> results;
    sync_from();

    // Build completion values
    for (std::map<std::string, std::vector<std::string> >::iterator i = uuid_to_path.lower_bound(base); i != uuid_to_path.end() && i->first.find(base) == 0; ++i)
        results.push_back(i->first.substr(0, uuid_output_length));

    for (std::map<std::string, std::vector<std::string> >::iterator i = name_to_path.lower_bound(base); i != name_to_path.end() && i->first.find(base) == 0; ++i)
        results.push_back(i->first);

    return results;
}

boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > admin_cluster_link_t::traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata)
{
    // as we traverse the json sub directories this will keep track of where we are
    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(new json_adapter_t<cluster_semilattice_metadata_t, namespace_metadata_ctx_t>(&cluster_metadata));

    std::vector<std::string>::const_iterator it = path.begin();

    // Traverse through the subfields until we're done with the url
    while (it != path.end()) {
        json_adapter_if_t<namespace_metadata_ctx_t>::json_adapter_map_t subfields = json_adapter_head->get_subfields(json_ctx);
        if (subfields.find(*it) == subfields.end()) {
            throw std::runtime_error("path not found: " + *it);
        }
        json_adapter_head = subfields[*it];
        it++;
    }

    return json_adapter_head;
}

bool is_uuid(const std::string& value) {
    try {
        str_to_uuid(value);
    } catch (std::runtime_error& ex) {
        return false;
    }
    return true;
}

std::vector<std::string> admin_cluster_link_t::get_path_from_id(const std::string& id) {
    // Names must be an exact match, but uuids can be prefix substrings
    if (name_to_path.count(id) == 0) {
        std::map<std::string, std::vector<std::string> >::iterator item = uuid_to_path.lower_bound(id);

        if (id.length() < minimum_uuid_substring)
            throw admin_parse_exc_t("identifier not found, too short to specify a uuid: " + id);

        if (item == uuid_to_path.end() || item->first.find(id) != 0)
            throw admin_cluster_exc_t("identifier not found: " + id);

        // Make sure that the found id is unique
        ++item;
        if (item != uuid_to_path.end() && item->first.find(id) == 0)
            throw admin_cluster_exc_t("uuid not unique: " + id);

        return uuid_to_path.lower_bound(id)->second;
    } else if (name_to_path.count(id) != 1) {
        throw admin_cluster_exc_t("name not unique: " + id);
    }

    return name_to_path.find(id)->second;
}

void admin_cluster_link_t::do_admin_split_shard(admin_command_parser_t::command_data& data) {
    std::vector<std::string> split_points(data.params["split-points"]);
    std::string ns(data.params["namespace"][0]);

    for (std::vector<std::string>::iterator i = split_points.begin(); i != split_points.end(); ++i) {
        printf("NYI: adding split point '%s' to namespace '%s'\n", i->c_str(), ns.c_str());
    }
}

void admin_cluster_link_t::do_admin_merge_shard(admin_command_parser_t::command_data& data) {
    std::vector<std::string> split_points(data.params["split-points"]);
    std::string ns(data.params["namespace"][0]);

    for (std::vector<std::string>::iterator i = split_points.begin(); i != split_points.end(); ++i) {
        printf("NYI: merging split point '%s' in namespace '%s'\n", i->c_str(), ns.c_str());
    }
}

void admin_cluster_link_t::do_admin_list(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string id = (data.params.count("filter") == 0 ? "" : data.params["filter"][0]);
    bool long_format = (data.params.count("long") == 1);
    std::vector<std::string> obj_path;

    if (id != "namespaces" && data.params.count("protocol") != 0)
        throw admin_parse_exc_t("--protocol option only valid when listing namespaces");

    if (id == "machines") {
        list_machines(long_format, cluster_metadata);
    } else if (id == "datacenters") {
        list_datacenters(long_format, cluster_metadata);
    } else if (id == "namespaces") {
        if (data.params.count("protocol") == 0) {
            list_dummy_namespaces(long_format, cluster_metadata);
            list_memcached_namespaces(long_format, cluster_metadata);
        } else {
            std::string protocol = data.params["protocol"][0];
            if (protocol == "memcached")
                list_memcached_namespaces(long_format, cluster_metadata);
            else if (protocol == "dummy")
                list_dummy_namespaces(long_format, cluster_metadata);
            else
                throw admin_parse_exc_t("unrecognized protocol: " + protocol);
        }
    } else if (id == "issues") {
        list_issues(long_format);
    } else if (id == "directory") {
        list_directory(long_format);
    } else if (!id.empty()) {
        // TODO: special formatting for each object type, instead of JSON
        namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
        obj_path = get_path_from_id(id);
        puts(cJSON_print_std_string(scoped_cJSON_t(traverse_directory(obj_path, json_ctx, cluster_metadata)->render(json_ctx)).get()).c_str());
    } else {
        list_machines(long_format, cluster_metadata);
        list_datacenters(long_format, cluster_metadata);
        list_dummy_namespaces(long_format, cluster_metadata);
        list_memcached_namespaces(long_format, cluster_metadata);
    }
}

void admin_cluster_link_t::list_directory(bool long_format UNUSED) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager.get_root_view()->get();

    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); i++) {
        printf("%s  ", uuid_to_str(i->second.machine_id).c_str());
        for (std::vector<std::string>::iterator j = i->second.ips.begin(); j != i->second.ips.end(); ++j)
            printf(" %s", j->c_str());
        printf("\n");
    }
}

void admin_cluster_link_t::list_issues(bool long_format UNUSED) {
    std::list<clone_ptr_t<global_issue_t> > issues = issue_aggregator.get_issues();
    puts("Issues: ");

    for (std::list<clone_ptr_t<global_issue_t> >::iterator i = issues.begin(); i != issues.end(); ++i) {
        puts((*i)->get_description().c_str());
    }
}

void admin_cluster_link_t::list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    puts("Datacenters:");
    for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator i = cluster_metadata.datacenters.datacenters.begin(); i != cluster_metadata.datacenters.datacenters.end(); ++i) {
        if (!i->second.is_deleted()) {
            printf(" %s", truncate_uuid(i->first).c_str());
            if (long_format)
                printf(" %s", i->second.get().name.get().c_str());
            printf("\n");
        }
    }
}

void admin_cluster_link_t::list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    puts("Dummy Namespaces:");
    for (namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::const_iterator i = cluster_metadata.dummy_namespaces.namespaces.begin(); i != cluster_metadata.dummy_namespaces.namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            printf(" %s", truncate_uuid(i->first).c_str());
            if (long_format)
                printf(" %s", i->second.get().name.get().c_str());
            printf("\n");
        }
    }
}

void admin_cluster_link_t::list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    puts("Memcached Namespaces:");
    for (namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::const_iterator i = cluster_metadata.memcached_namespaces.namespaces.begin(); i != cluster_metadata.memcached_namespaces.namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            printf(" %s", truncate_uuid(i->first).c_str());
            if (long_format)
                printf(" %s", i->second.get().name.get().c_str());
            printf("\n");
        }
    }
}

void admin_cluster_link_t::list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    puts("Machines:");
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            printf(" %s", truncate_uuid(i->first).c_str());
            if (long_format)
                printf(" %s", i->second.get().name.get().c_str());
            printf("\n");
        }
    }
}

void admin_cluster_link_t::do_admin_create_datacenter(admin_command_parser_t::command_data& data) {
    std::vector<std::string> obj_path;
    obj_path.push_back("datacenters");
    obj_path.push_back("new");

    if (data.params.count("name") == 0)
        set_metadata_value(obj_path, "{ }");
    else
        set_metadata_value(obj_path, "{ \"name\": \"" + data.params["name"][0] + "\" }");
}


void admin_cluster_link_t::do_admin_create_namespace(admin_command_parser_t::command_data& data) {
    std::vector<std::string> obj_path;
    std::string value;

    // TODO: WTF?  Is this how we serialize things to json?  By
    // copying a string and hoping there are no special characters?
    value += "{ \"name\": \"" + (data.params.count("name") == 0 ? std::string() : data.params["name"][0]) + "\"";

    std::string protocol = data.params["protocol"][0];
    if (protocol == "memcached")
        obj_path.push_back("memcached_namespaces");
    else if (protocol == "dummy")
        obj_path.push_back("dummy_namespaces");
    else
        throw admin_parse_exc_t("unrecognized protocol: " + protocol);

    value += ", \"port\": " + data.params["port"][0];

    if (data.params.count("primary") == 1) {
        std::string primary = data.params["primary"][0];

        if (!is_uuid(primary)) {
            // Convert name to uuid
            std::map<std::string, std::vector<std::string> >::const_iterator i = name_to_path.find(primary);
            if (i == name_to_path.end())
                throw admin_parse_exc_t("unknown object: " + primary);

            // uuid should be the last component of the path
            primary = i->second[i->second.size() - 1];
        }

        value += ", \"primary_uuid\": \"" + primary + "\"";
    }

    obj_path.push_back("new");
    value += " }";

    set_metadata_value(obj_path, value);
}

void admin_cluster_link_t::do_admin_set_datacenter(admin_command_parser_t::command_data& data) {
    std::vector<std::string> target_path(get_path_from_id(data.params["id"][0]));
    std::string datacenter_id = data.params["datacenter"][0];
    std::vector<std::string> datacenter_path(get_path_from_id(datacenter_id));

    // Make sure the path is as expected
    if (!is_uuid(target_path[1]))
        throw admin_cluster_exc_t("unexpected error when looking up destination: " + datacenter_id);

    // Target must be a datacenter in all existing use cases
    if (datacenter_path[0] != "datacenters")
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);

    if (target_path[0] == "memcached_namespaces" || target_path[0] == "dummy_namespaces")
        target_path.push_back("primary_uuid");
    else if (target_path[0] == "machines")
        target_path.push_back("datacenter_uuid");
    else
        throw admin_parse_exc_t("target object is not a namespace or machine");

    if (data.params.count("resolve") == 1)
        target_path.push_back("resolve");

    // Convert target name to uuid if necessary
    set_metadata_value(target_path, "\"" + datacenter_path[1] + "\""); // TODO: adding quotes like this is kind of silly - better way to get past the json parsing?
}
void admin_cluster_link_t::do_admin_set_name(admin_command_parser_t::command_data& data) {
    // TODO: make sure names aren't silly things like uuids or reserved strings
    std::vector<std::string> path(get_path_from_id(data.params["id"][0]));
    path.push_back("name");

    if (data.params.count("resolve") == 1)
        path.push_back("resolve");

    set_metadata_value(path, "\"" + data.params["new-name"][0] + "\""); // TODO: adding quotes like this is kind of silly - better way to get past the json parsing?
}

void admin_cluster_link_t::do_admin_set_acks(admin_command_parser_t::command_data& data) {
    std::vector<std::string> ns_path(get_path_from_id(data.params["namespace"][0]));
    std::vector<std::string> dc_path(get_path_from_id(data.params["datacenter"][0]));
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string acks_str = data.params["num-acks"][0].c_str();

    // Make sure num-acks is a number
    for (size_t i = 0; i < acks_str.length(); ++i)
        if (acks_str[i] < '0' || acks_str[i] > '9')
            throw admin_parse_exc_t("num-acks is not a number");

    // TODO: use resolve flag

    if (dc_path[0] != "datacenters")
        throw admin_parse_exc_t(data.params["datacenter"][0] + " is not a datacenter");
    
    if (ns_path[0] == "dummy_namespaces") {
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.dummy_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_acks_internal(i->second.get_mutable(), str_to_uuid(dc_path[1]), atoi(acks_str.c_str()));

    } else if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_acks_internal(i->second.get_mutable(), str_to_uuid(dc_path[1]), atoi(acks_str.c_str()));

    } else
        throw admin_parse_exc_t(data.params["namespace"][0] + " is not a namespace");

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_set_acks_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_acks) {
    if (ns.primary_datacenter.in_conflict())
        throw admin_cluster_exc_t("namespace primary datacenter is in conflict, run 'help resolve' for more information");

    if (ns.replica_affinities.in_conflict())
        throw admin_cluster_exc_t("namespace replica affinities are in conflict, run 'help resolve' for more information");

    if (ns.ack_expectations.in_conflict())
        throw admin_cluster_exc_t("namespace ack expectations are in conflict, run 'help resolve' for more information");

    // Make sure the selected datacenter is assigned to the namespace and that the number of replicas is less than or equal to the number of acks
    std::map<datacenter_id_t, int>::iterator i = ns.replica_affinities.get().find(datacenter);
    bool is_primary = (datacenter == ns.primary_datacenter.get());
    if ((i == ns.replica_affinities.get().end() || i->second == 0) && !is_primary)
        throw admin_cluster_exc_t("the specified datacenter has no replica affinities with the given namespace");
    else if (num_acks > i->second + (is_primary ? 1 : 0))
        throw admin_cluster_exc_t("cannot assign more ack expectations than replicas in a datacenter");

    // TODO: get a lock to avoid a race condition
    ns.ack_expectations.get_mutable()[datacenter] = num_acks;
    ns.ack_expectations.upgrade_version(sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_set_replicas(admin_command_parser_t::command_data& data) {
    std::vector<std::string> ns_path(get_path_from_id(data.params["namespace"][0]));
    std::vector<std::string> dc_path(get_path_from_id(data.params["datacenter"][0]));
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string replicas_str = data.params["num-replicas"][0].c_str();
    int num_replicas = atoi(replicas_str.c_str());

    // Make sure num-acks is a number
    for (size_t i = 0; i < replicas_str.length(); ++i)
        if (replicas_str[i] < '0' || replicas_str[i] > '9')
            throw admin_parse_exc_t("num-replicas is not a number");

    // TODO: use resolve flag

    if (dc_path[0] != "datacenters")
        throw admin_parse_exc_t(data.params["datacenter"][0] + " is not a datacenter");

    // TODO: it might make more sense for this error to happen later
    datacenter_id_t datacenter(str_to_uuid(dc_path[1]));
    if (get_machine_count_in_datacenter(cluster_metadata, datacenter) < (size_t)num_replicas)
        throw admin_cluster_exc_t("the number of replicas cannot be more than the number of machines in the datacenter");
    
    if (ns_path[0] == "dummy_namespaces") {
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.dummy_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_replicas_internal(i->second.get_mutable(), datacenter, num_replicas);

    } else if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_replicas_internal(i->second.get_mutable(), datacenter, num_replicas);

    } else
        throw admin_parse_exc_t(data.params["namespace"][0] + " is not a namespace");

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_set_replicas_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_replicas) {
    if (ns.primary_datacenter.in_conflict())
        throw admin_cluster_exc_t("namespace primary datacenter is in conflict, run 'help resolve' for more information");

    if (ns.replica_affinities.in_conflict())
        throw admin_cluster_exc_t("namespace replica affinities are in conflict, run 'help resolve' for more information");

    if (ns.ack_expectations.in_conflict())
        throw admin_cluster_exc_t("namespace ack expectations are in conflict, run 'help resolve' for more information");

    bool is_primary = (datacenter == ns.primary_datacenter.get());
    if (is_primary && num_replicas == 0)
        throw admin_cluster_exc_t("the number of replicas for the primary datacenter cannot be 0");

    std::map<datacenter_id_t, int>::iterator i = ns.ack_expectations.get().find(datacenter);
    if (i == ns.ack_expectations.get().end()) {
        ns.ack_expectations.get_mutable()[datacenter] = 0;
        ns.ack_expectations.upgrade_version(sync_peer.get_uuid());
    } else if (i->second > num_replicas)
        throw admin_cluster_exc_t("the number of replicas for this datacenter cannot be less than the number of acks");

    // TODO: get a lock to avoid a race condition
    ns.replica_affinities.get_mutable()[datacenter] = num_replicas - (is_primary ? 1 : 0);
    ns.replica_affinities.upgrade_version(sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_remove(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
    std::vector<std::string> ids = data.params["id"];
    bool errored = false;

    for (size_t i = 0; i < ids.size(); ++i) {
        try {
            std::vector<std::string> path(get_path_from_id(ids[i]));
            boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(traverse_directory(path, json_ctx, cluster_metadata));

            json_adapter_head->erase(json_ctx);
            // TODO: clean up any hanging references to this object's uuid
        } catch (std::exception& ex) {
            puts(ex.what());
            errored = true;
        }
    }

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);

    if (errored)
        throw admin_cluster_exc_t("not all removes were successful");
}

void admin_cluster_link_t::set_metadata_value(const std::vector<std::string>& path, const std::string& value)
{
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    namespace_metadata_ctx_t json_ctx(sync_peer.get_uuid()); // Use peer rather than ourselves - TODO: race condition on sync? need to lock metadata
    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(traverse_directory(path, json_ctx, cluster_metadata));

    scoped_cJSON_t change(cJSON_Parse(value.c_str()));
    if (!change.get())
        throw admin_parse_exc_t("Json body failed to parse: " + value);

    logINF("Applying data %s", value.c_str());
    json_adapter_head->apply(change.get(), json_ctx);

    /* Fill in the blueprints */
    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

size_t admin_cluster_link_t::machine_count() const {
    size_t count = 0;
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i)
        if (!i->second.is_deleted())
            ++count;

    return count;
}

size_t admin_cluster_link_t::get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter) {
    size_t count = 0;

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i)
        if (!i->second.is_deleted() && i->second.get().datacenter.get() == datacenter)
            ++count;
    
    return count;
}

size_t admin_cluster_link_t::available_machine_count() {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager.get_root_view()->get();
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    size_t count = 0;

    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); i++) {
        // Check uuids vs machines in cluster
        machines_semilattice_metadata_t::machine_map_t::const_iterator machine = cluster_metadata.machines.machines.find(i->second.machine_id);
        if (machine != cluster_metadata.machines.machines.end() && !machine->second.is_deleted())
            ++count;
    }

    return count;
}

size_t admin_cluster_link_t::issue_count() {
    return issue_aggregator.get_issues().size();
}

