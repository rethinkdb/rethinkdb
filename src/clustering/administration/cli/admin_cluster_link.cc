#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <stdarg.h>
#include <stdio.h>

#include <curl/curl.h>

#include <map>
#include <stdexcept>
#include <iostream>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/administration/suggester.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "do_on_thread.hpp"
#include "perfmon.hpp"

std::string admin_value_to_string(const perfmon_result_t& stats) {
    std::string result;

    if (stats.is_string())
        result.assign(*stats.get_string() + "\n");
    else if (stats.is_map())
        for (perfmon_result_t::const_iterator i = stats.begin(); i != stats.end(); ++i)
            if (i->second->is_string())
                result += i->first + ": " + admin_value_to_string(*i->second);
            else
                result += i->first + ":\n" + admin_value_to_string(*i->second);

    return result;
}

// TODO: use key_to_escaped_string
std::string admin_value_to_string(const memcached_protocol_t::region_t& region) {
    std::string shard_str;
    if (region.left == store_key_t())
        shard_str += "inf-";
    else
        shard_str += "\"" + key_to_str(region.left) + "\"-";
    if (region.right.unbounded)
        shard_str += "inf";
    else
        shard_str += "\"" + key_to_str(region.right.key) + "\"";
    return shard_str;
}

std::string admin_value_to_string(const mock::dummy_protocol_t::region_t& region) {
    return mock::to_string(region);
}

std::string admin_value_to_string(int value) {
    return strprintf("%i", value);
}

std::string admin_value_to_string(const boost::uuids::uuid& uuid) {
    return uuid_to_str(uuid);
}

std::string admin_value_to_string(const std::string& str) {
    return "\"" + str + "\"";
}

std::string admin_value_to_string(const std::map<boost::uuids::uuid, int>& value) {
    std::string result;
    size_t count = 0;
    for (std::map<boost::uuids::uuid, int>::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        result += strprintf("%s: %i%s", uuid_to_str(i->first).c_str(), i->second, count == value.size() ? "" : ", ");
    }
    return result;
}

std::string admin_value_to_string(const std::set<mock::dummy_protocol_t::region_t>& value) {
    std::string result;
    size_t count = 0;
    for (std::set<mock::dummy_protocol_t::region_t>::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        result += strprintf("%s%s", admin_value_to_string(*i).c_str(), count == value.size() ? "" : ", ");
    }
    return result;
}

std::string admin_value_to_string(const std::set<key_range_t>& value) {
    std::string result;
    size_t count = 0;
    for (std::set<key_range_t>::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        result += strprintf("%s%s", admin_value_to_string(*i).c_str(), count == value.size() ? "" : ", ");
    }
    return result;
}

template <class protocol_t>
std::string admin_value_to_string(const region_map_t<protocol_t, boost::uuids::uuid>& value) {
    std::string result;
    size_t count = 0;
    for (typename region_map_t<protocol_t, boost::uuids::uuid>::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        result += strprintf("%s: %s%s", admin_value_to_string(i->first).c_str(), uuid_to_str(i->second).c_str(), count == value.size() ? "" : ", ");
    }
    return result;
}

template <class protocol_t>
std::string admin_value_to_string(const region_map_t<protocol_t, std::set<boost::uuids::uuid> >& value) {
    std::string result;
    size_t count = 0;
    for (typename region_map_t<protocol_t, std::set<boost::uuids::uuid> >::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        //TODO: print more detail
        result += strprintf("%s: %ld machine%s%s", admin_value_to_string(i->first).c_str(), i->second.size(), i->second.size() == 1 ? "" : "s", count == value.size() ? "" : ", ");
    }
    return result;
}

void admin_print_table(const std::vector<std::vector<std::string> >& table) {
    std::vector<int> column_widths;

    if (table.size() == 0)
        return;

    // Verify that the vectors are consistent size
    for (size_t i = 1; i < table.size(); ++i)
        if (table[i].size() != table[0].size())
            throw admin_cluster_exc_t("unexpected error when printing table");

    // Determine the maximum size of each column
    for (size_t i = 0; i < table[0].size(); ++i) {
        int max = table[0][i].length();

        for (size_t j = 1; j < table.size(); ++j)
            if ((int)table[j][i].length() > max)
                max = table[j][i].length();

        column_widths.push_back(max);
    }

    // Print out each line, spacing each column
    for (size_t i = 0; i < table.size(); ++i) {
        for (size_t j = 0; j < table[i].size(); ++j)
            printf("%-*s", column_widths[j] + 2, table[i][j].c_str());
        printf("\n");
    }
}

// Truncate a uuid for easier user-interface
std::string admin_cluster_link_t::truncate_uuid(const boost::uuids::uuid& uuid) {
    if (uuid.is_nil())
        return std::string("none");
    else
        return uuid_to_str(uuid).substr(0, uuid_output_length);
}

admin_cluster_link_t::admin_cluster_link_t(const std::set<peer_address_t> &joins, int client_port, signal_t *interruptor) :
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
    unsatisfiable_goals_issue_tracker_feed(&issue_aggregator, &unsatisfiable_goals_issue_tracker),
    initial_joiner(&connectivity_cluster, &connectivity_cluster_run, joins, 5000),
    curl_handle(curl_easy_init()),
    curl_header_list(curl_slist_append(NULL, "Content-Type:application/json")),
    sync_peer_id(nil_uuid())
{
    wait_interruptible(initial_joiner.get_ready_signal(), interruptor);
    if (!initial_joiner.get_success())
            throw admin_cluster_exc_t("failed to join cluster");

    std::set<peer_id_t> peer_set = connectivity_cluster.get_peers_list();
    for (std::set<peer_id_t>::iterator i = peer_set.begin(); i != peer_set.end(); ++i)
        if (*i != connectivity_cluster.get_me())
            sync_peer_id = *i;

    // TODO: get the http port of the server through some more intelligent means (once it exists)
    peer_address_t sync_peer_address = connectivity_cluster.get_peer_address(sync_peer_id);
    sync_peer.assign(strprintf("http://%s:%i/ajax/", sync_peer_address.ip.as_dotted_decimal().c_str(), sync_peer_address.port + 1000));

    curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_header_list);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 3000);

    // Callback when receiving data so we can look up some things (like new objects' uuids)
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, handle_post_result);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &post_result);
}

admin_cluster_link_t::~admin_cluster_link_t() {
    curl_slist_free_all(curl_header_list);
    curl_easy_cleanup(curl_handle);
    clear_metadata_maps();
}

void admin_cluster_link_t::update_metadata_maps() {
    clear_metadata_maps();

    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    add_subset_to_maps("machines", cluster_metadata.machines.machines);
    add_subset_to_maps("datacenters", cluster_metadata.datacenters.datacenters);
    add_subset_to_maps("dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
    add_subset_to_maps("memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
}

void admin_cluster_link_t::clear_metadata_maps() {
    // All metadata infos will be in the name_map and uuid_map exactly one each
    for (std::map<std::string, metadata_info_t*>::iterator i = uuid_map.begin(); i != uuid_map.end(); ++i)
        delete i->second;

    name_map.clear();
    uuid_map.clear();
}

template <class T>
void admin_cluster_link_t::add_subset_to_maps(const std::string& base, T& data_map) {
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        metadata_info_t* info = new metadata_info_t;
        info->uuid = uuid_to_str(i->first);
        info->path.push_back(base);
        info->path.push_back(info->uuid);

        if (!i->second.get().name.in_conflict()) {
            info->name = i->second.get().name.get();
            name_map.insert(std::make_pair<std::string, metadata_info_t*>(info->name, info));
        }
        uuid_map.insert(std::make_pair<std::string, metadata_info_t*>(info->uuid, info));
    }
}

void admin_cluster_link_t::sync_from() {
    try {
        cond_t interruptor;
        semilattice_metadata->sync_from(sync_peer_id, &interruptor);
    } catch (sync_failed_exc_t& ex) {
        throw admin_no_connection_exc_t("connection lost to cluster");
    }

    semilattice_metadata = semilattice_manager_cluster.get_root_view();
    update_metadata_maps();
}

std::vector<std::string> admin_cluster_link_t::get_ids_internal(const std::string& base, const std::string& path) {
    std::vector<std::string> results;

    // TODO: check for uuid collisions, give longer completions
    // Build completion values
    for (std::map<std::string, metadata_info_t*>::iterator i = uuid_map.lower_bound(base); i != uuid_map.end() && i->first.find(base) == 0; ++i)
        if (path.empty() || i->second->path[0] == path)
            results.push_back(i->first.substr(0, uuid_output_length));

    for (std::map<std::string, metadata_info_t*>::iterator i = name_map.lower_bound(base); i != name_map.end() && i->first.find(base) == 0; ++i)
        if (path.empty() || i->second->path[0] == path)
            results.push_back(i->first);

    return results;
}

std::vector<std::string> admin_cluster_link_t::get_ids(const std::string& base) {
    return get_ids_internal(base, "");
}

std::vector<std::string> admin_cluster_link_t::get_machine_ids(const std::string& base) {
    return get_ids_internal(base, "machines");
}

std::vector<std::string> admin_cluster_link_t::get_namespace_ids(const std::string& base) {
    std::vector<std::string> namespaces = get_ids_internal(base, "dummy_namespaces");
    std::vector<std::string> delta = get_ids_internal(base, "memcached_namespaces");
    std::copy(delta.begin(), delta.end(), std::back_inserter(namespaces));
    return namespaces;
}

std::vector<std::string> admin_cluster_link_t::get_datacenter_ids(const std::string& base) {
    return get_ids_internal(base, "datacenters");
}

std::vector<std::string> admin_cluster_link_t::get_conflicted_ids(const std::string& base UNUSED) {
    std::set<std::string> unique_set;
    std::vector<std::string> results;

    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > conflicts = vector_clock_conflict_issue_tracker.get_vector_clock_issues();

    for (std::list<clone_ptr_t<vector_clock_conflict_issue_t> >::iterator i = conflicts.begin(); i != conflicts.end(); ++i)
        unique_set.insert(uuid_to_str(i->get()->object_id));

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i)
        if (i->find(base) == 0)
            results.push_back(i->substr(0, uuid_output_length));

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i) {
        std::map<std::string, metadata_info_t*>::iterator info = uuid_map.find(*i);
        if (info != uuid_map.end() && info->second->name.find(base) == 0)
            results.push_back(info->second->name);
    }

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

admin_cluster_link_t::metadata_info_t* admin_cluster_link_t::get_info_from_id(const std::string& id) {
    // Names must be an exact match, but uuids can be prefix substrings
    if (name_map.count(id) == 0) {
        std::map<std::string, metadata_info_t*>::iterator item = uuid_map.lower_bound(id);

        if (id.length() < minimum_uuid_substring)
            throw admin_parse_exc_t("identifier not found, too short to specify a uuid: " + id);

        if (item == uuid_map.end() || item->first.find(id) != 0)
            throw admin_cluster_exc_t("identifier not found: " + id);

        // Make sure that the found id is unique
        ++item;
        if (item != uuid_map.end() && item->first.find(id) == 0)
            throw admin_cluster_exc_t("uuid not unique: " + id);

        return uuid_map.lower_bound(id)->second;
    } else if (name_map.count(id) != 1) {
        std::stringstream exception_info;
        exception_info << "'" << id << "' not unique, possible objects:";
        for (std::map<std::string, metadata_info_t*>::iterator item = name_map.lower_bound(id); item != name_map.end() && item->first == id; ++item) {
            if (item->second->path[0] == "datacenters")
                exception_info << std::endl << "datacenter    " << item->second->uuid.substr(0, uuid_output_length);
            else if (item->second->path[0] == "dummy_namespaces")
                exception_info << std::endl << "namespace (d) " << item->second->uuid.substr(0, uuid_output_length);
            else if (item->second->path[0] == "memcached_namespaces")
                exception_info << std::endl << "namespace (m) " << item->second->uuid.substr(0, uuid_output_length);
            else if (item->second->path[0] == "machines")
                exception_info << std::endl << "machine       " << item->second->uuid.substr(0, uuid_output_length);
            else
                exception_info << std::endl << "unknown       " << item->second->uuid.substr(0, uuid_output_length);
        }
        throw admin_cluster_exc_t(exception_info.str());
    }

    return name_map.find(id)->second;
}

datacenter_id_t get_machine_datacenter(const std::string& id, const machine_id_t& machine, cluster_semilattice_metadata_t& cluster_metadata) {
    machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.find(machine);

    if (i == cluster_metadata.machines.machines.end())
        throw admin_cluster_exc_t("unexpected error, machine not found: " + uuid_to_str(machine));

    if (i->second.is_deleted())
        throw admin_cluster_exc_t("unexpected error, machine is deleted: " + uuid_to_str(machine));

    if (i->second.get().datacenter.in_conflict())
        throw admin_cluster_exc_t("datacenter is in conflict for machine " + id);

    return i->second.get().datacenter.get();
}

void admin_cluster_link_t::do_admin_pin_shard(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string ns(data.params["namespace"][0]);
    std::vector<std::string> ns_path(get_info_from_id(ns)->path);
    std::string shard_str(data.params["key"][0]);
    std::string primary;
    std::vector<std::string> secondaries;
    std::string post_path(path_to_str(ns_path));

    if (ns_path[0] == "dummy_namespaces")
        throw admin_cluster_exc_t("pinning not supported for dummy namespaces");
    else if(ns_path[0] != "memcached_namespaces")
        throw admin_parse_exc_t("object is not a namespace: " + ns);

    if (data.params.count("primary") == 1)
        primary.assign(data.params["primary"][0]);

    if (data.params.count("secondary") != 0)
        secondaries = data.params["secondary"];

    // Break up shard string into left and right
    // Need to look at the raw input since the parser fucks everything up
    // TODO: for now, assuming that a shard boundary does not contain the character '-', and that no boundaries are named '%inf'
    size_t split = shard_str.find("-");
    if (split == std::string::npos)
        throw admin_parse_exc_t("incorrect shard specifier format");
    // TODO: use escaped_string_to_key
    shard_input_t shard_in;
    shard_in.left.key = shard_str.substr(0, split);
    shard_in.left.inf = shard_in.left.key == "%inf";
    shard_in.right.key = shard_str.substr(split + 1);
    shard_in.right.inf = shard_in.right.key == "%inf";

    if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end() || i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, could not find namespace: " + ns);

        // If no primaries or secondaries are given, we list the current machine assignments
        if (primary.empty() && secondaries.empty())
            list_pinnings(i->second.get_mutable(), shard_in, cluster_metadata);
        else
            do_admin_pin_shard_internal(i->second.get_mutable(), shard_in, primary, secondaries, cluster_metadata, post_path);
    } else
        throw admin_cluster_exc_t("unexpected error, unknown namespace protocol");
}

template <class protocol_t>
typename protocol_t::region_t admin_cluster_link_t::find_shard_in_namespace(namespace_semilattice_metadata_t<protocol_t>& ns,
                                                                            const shard_input_t& shard_in) {
    typename protocol_t::region_t shard;
    typename std::set<typename protocol_t::region_t>::iterator s = ns.shards.get_mutable().begin();
    while (s != ns.shards.get_mutable().end()) {
        if ((shard_in.left.inf && s->left == store_key_t::min()) ||
            (!shard_in.left.key.empty() && s->left == store_key_t(shard_in.left.key))) {
            if (shard_in.right.inf && !s->right.unbounded)
                throw admin_cluster_exc_t("could not find specified shard");
            else if (!shard_in.right.key.empty() && s->right.key != store_key_t(shard_in.right.key))
                throw admin_cluster_exc_t("could not find specified shard");
            shard = *s;
            break;
        }

        if (shard_in.right.inf && s->right.unbounded) {
            // If the left was specified, it must not have matched, so we fail
            if (shard_in.left.inf || !shard_in.left.key.empty())
                throw admin_cluster_exc_t("could not find specified shard");
            shard = *s;
            break;
        } else if (!shard_in.right.key.empty() && s->right.key == store_key_t(shard_in.right.key)) {
            // If the left was specified, it must not have matched, so we fail
            if (shard_in.left.inf || !shard_in.left.key.empty())
                throw admin_cluster_exc_t("could not find specified shard");
            shard = *s;
            break;
        }
        ++s;
    }

    if (s == ns.shards.get_mutable().end())
        throw admin_cluster_exc_t("could not find specified shard");

    return shard;
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_pin_shard_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                                       const shard_input_t& shard_in,
                                                       const std::string& primary_str,
                                                       const std::vector<std::string>& secondary_strs,
                                                       cluster_semilattice_metadata_t& cluster_metadata,
                                                       const std::string& post_path) {
    machine_id_t primary(nil_uuid());
    std::multimap<datacenter_id_t, machine_id_t> datacenter_use;
    std::multimap<datacenter_id_t, machine_id_t> old_datacenter_use;
    typename region_map_t<protocol_t, machine_id_t>::iterator primary_shard;
    typename region_map_t<protocol_t, std::set<machine_id_t> >::iterator secondaries_shard;
    std::set<machine_id_t> secondaries;

    // Check that none of the required fields are in conflict
    if (ns.shards.in_conflict())
        throw admin_cluster_exc_t("namespace shards are in conflict, run 'help resolve' for more information");
    if (ns.primary_pinnings.in_conflict())
        throw admin_cluster_exc_t("namespace primary pinnings are in conflict, run 'help resolve' for more information");
    if (ns.secondary_pinnings.in_conflict())
        throw admin_cluster_exc_t("namespace secondary pinnings are in conflict, run 'help resolve' for more information");
    if (ns.replica_affinities.in_conflict())
        throw admin_cluster_exc_t("namespace replica affinities are in conflict, run 'help resolve' for more information");
    if (ns.primary_datacenter.in_conflict())
        throw admin_cluster_exc_t("namespace primary datacenter is in conflict, run 'help resolve' for more information");

    // Verify that the selected shard exists, and convert it into a region_t
    typename protocol_t::region_t shard = find_shard_in_namespace(ns, shard_in);

    // Verify primary is a valid machine and matches the primary datacenter
    if (!primary_str.empty()) {
        std::vector<std::string> primary_path(get_info_from_id(primary_str)->path);
        primary = str_to_uuid(primary_path[1]);
        if (primary_path[0] != "machines")
            throw admin_parse_exc_t("object is not a machine: " + primary_str);
        if (get_machine_datacenter(primary_str, str_to_uuid(primary_path[1]), cluster_metadata) != ns.primary_datacenter.get())
            throw admin_parse_exc_t("machine " + primary_str + " does not belong to the primary datacenter");
    }

    // Verify secondaries are valid machines and store by datacenter for later
    for (size_t i = 0; i < secondary_strs.size(); ++i) {
        std::vector<std::string> secondary_path(get_info_from_id(secondary_strs[i])->path);
        machine_id_t machine = str_to_uuid(secondary_path[1]);
        if (secondary_path[0] != "machines")
            throw admin_parse_exc_t("object is not a machine: " + secondary_strs[i]);

        datacenter_id_t datacenter = get_machine_datacenter(secondary_strs[i], machine, cluster_metadata);
        if (ns.replica_affinities.get().count(datacenter) == 0)
            throw admin_parse_exc_t("machine " + secondary_strs[i] + " belongs to a datacenter with no affinity to namespace");

        datacenter_use.insert(std::make_pair(datacenter, machine));
    }

    // Find the secondary pinnings and build the old datacenter pinning map if it exists
    for (secondaries_shard = ns.secondary_pinnings.get_mutable().begin(); secondaries_shard != ns.secondary_pinnings.get_mutable().end(); ++secondaries_shard)
        if (secondaries_shard->first.contains_key(shard.left))
            break;

    if (secondaries_shard != ns.secondary_pinnings.get_mutable().end())
        for (std::set<machine_id_t>::iterator i = secondaries_shard->second.begin(); i != secondaries_shard->second.begin(); ++i)
            old_datacenter_use.insert(std::make_pair(get_machine_datacenter(uuid_to_str(*i), *i, cluster_metadata), *i));

    // Build the full set of secondaries, carry over any datacenters that were ignored in the command
    std::map<datacenter_id_t, int> affinities = ns.replica_affinities.get();
    for (std::map<datacenter_id_t, int>::iterator i = affinities.begin(); i != affinities.end(); ++i) {
        if (datacenter_use.count(i->first) == 0) {
            // No machines specified for this datacenter, copy over any from the old stuff
            for (std::map<datacenter_id_t, machine_id_t>::iterator j = old_datacenter_use.lower_bound(i->first); j != old_datacenter_use.end() && j->first == i->first; ++j)
                secondaries.insert(j->second);
        } else {
            // Copy over all the specified machines for this datacenter
            for (std::map<datacenter_id_t, machine_id_t>::iterator j = old_datacenter_use.lower_bound(i->first); j != old_datacenter_use.end() && j->first == i->first; ++j)
                secondaries.insert(j->second);
        }
    }

    // Set primary and secondaries
    if (!primary_str.empty())
        insert_pinning(ns.primary_pinnings.get_mutable(), shard, primary);
    if (!secondary_strs.empty())
        insert_pinning(ns.secondary_pinnings.get_mutable(), shard, secondaries);

    // Post changes to the sync peer
    if (!primary_str.empty())
        post_metadata(post_path + "/primary_pinnings", ns.primary_pinnings.get_mutable());
    if (!secondary_strs.empty())
        post_metadata(post_path + "/secondary_pinnings", ns.secondary_pinnings.get_mutable());
}

template <class map_type, class value_type>
void admin_cluster_link_t::insert_pinning(map_type& region_map, const key_range_t& shard, value_type& value) {
    map_type new_map;
    bool shard_done = false;

    for (typename map_type::iterator i = region_map.begin(); i != region_map.end(); ++i) {
        if (i->first.contains_key(shard.left)) {
            if (i->first.left != shard.left) {
                key_range_t new_shard(key_range_t::closed, i->first.left, key_range_t::open, shard.left);
                new_map.set(new_shard, i->second);
            }
            new_map.set(shard, value);
            if (i->first.right.key != shard.right.key) {
                // TODO: what if the shard we're looking for staggers the right bound
                key_range_t new_shard;
                new_shard.left = shard.right.key;
                new_shard.right = i->first.right;
                new_map.set(new_shard, i->second);
            }
            shard_done = true;
        } else {
            // just copy over the shard
            new_map.set(i->first, i->second);
        }
    }

    if (!shard_done)
        throw admin_cluster_exc_t("unexpected error, did not find the specified shard");

    region_map = new_map;
}

// TODO: templatize on protocol
void admin_cluster_link_t::do_admin_split_shard(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> split_points(data.params["split-points"]);
    bool errored = false;

    if (ns_path[0] == "memcached_namespaces") {
        namespace_id_t ns_id(str_to_uuid(ns_path[1]));
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator ns_it =
            cluster_metadata.memcached_namespaces.namespaces.find(ns_id);

        if (ns_it == cluster_metadata.memcached_namespaces.namespaces.end() || ns_it->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error when looking up namespace: " + ns_path[1]);

        namespace_semilattice_metadata_t<memcached_protocol_t>& ns = ns_it->second.get_mutable();

        if (ns.shards.in_conflict())
            throw admin_cluster_exc_t("namespace shards are in conflict, run 'help resolve' for more information");

        for (size_t i = 0; i < split_points.size(); ++i) {
            // TODO: use escaped_string_to_key
            store_key_t key(split_points[i]);
            if (ns.shards.get().empty()) {
                // this should never happen, but try to handle it anyway
                key_range_t left(key_range_t::none, store_key_t(), key_range_t::open, store_key_t(key));
                key_range_t right(key_range_t::closed, store_key_t(key), key_range_t::none, store_key_t());
                ns.shards.get_mutable().insert(left);
                ns.shards.get_mutable().insert(right);
            } else {
                try {
                    // TODO: use a better search than linear
                    std::set<key_range_t>::iterator shard = ns.shards.get_mutable().begin();
                    while (true) {
                        if (shard == ns.shards.get_mutable().end())
                            throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);
                        if (shard->contains_key(key))
                            break;
                        ++shard;
                    }

                    // Create the two new shards to be inserted
                    key_range_t left;
                    left.left = shard->left;
                    left.right = key_range_t::right_bound_t(key);
                    key_range_t right;
                    right.left = key;
                    right.right = shard->right;

                    ns.shards.get_mutable().erase(shard);
                    ns.shards.get_mutable().insert(left);
                    ns.shards.get_mutable().insert(right);
                } catch (std::exception& ex) {
                    printf("%s\n", ex.what());
                    errored = true;
                }
            }
        }

        // Any time shards are changed, we destroy existing pinnings
        // Use 'resolve' because they should be cleared even if in conflict
        // ID sent to the vector clock doesn't matter here since we're setting the metadata through HTTP (TODO: change this if that is no longer true)
        region_map_t<memcached_protocol_t, machine_id_t> new_primaries(memcached_protocol_t::region_t::universe(), nil_uuid());
        region_map_t<memcached_protocol_t, std::set<machine_id_t> > new_secondaries(memcached_protocol_t::region_t::universe(), std::set<machine_id_t>());

        ns.primary_pinnings = ns.primary_pinnings.make_resolving_version(new_primaries, connectivity_cluster.get_me().get_uuid());
        ns.secondary_pinnings = ns.secondary_pinnings.make_resolving_version(new_secondaries, connectivity_cluster.get_me().get_uuid());

        std::string post_path(path_to_str(ns_path));
        post_metadata(post_path + "/shards", ns.shards.get_mutable());
        post_metadata(post_path + "/primary_pinnings/resolve", ns.primary_pinnings.get_mutable());
        post_metadata(post_path + "/secondary_pinnings/resolve", ns.secondary_pinnings.get_mutable());

    } else if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("splitting not supported for dummy namespaces");
    } else
        throw admin_cluster_exc_t("invalid object type");

    if (errored && split_points.size() > 1)
        throw admin_cluster_exc_t("not all split points were successfully added");
}

void admin_cluster_link_t::do_admin_merge_shard(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> split_points(data.params["split-points"]);
    bool errored = false;

    if (ns_path[0] == "memcached_namespaces") {
        namespace_id_t ns_id(str_to_uuid(ns_path[1]));
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator ns_it =
            cluster_metadata.memcached_namespaces.namespaces.find(ns_id);

        if (ns_it == cluster_metadata.memcached_namespaces.namespaces.end() || ns_it->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error when looking up namespace: " + ns_path[1]);

        namespace_semilattice_metadata_t<memcached_protocol_t>& ns = ns_it->second.get_mutable();

        if (ns.shards.in_conflict())
            throw admin_cluster_exc_t("namespace shards are in conflict, run 'help resolve' for more information");

        for (size_t i = 0; i < split_points.size(); ++i) {
            // TODO: use escaped_string_to_key
            store_key_t key(split_points[i]);
            try {
                // TODO: use a better search than linear
                std::set<key_range_t>::iterator shard = ns.shards.get_mutable().begin();

                if (shard == ns.shards.get_mutable().end())
                    throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);

                std::set<key_range_t>::iterator prev = shard++;
                while (true) {
                    if (shard == ns.shards.get_mutable().end())
                        throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);
                    if (shard->contains_key(key))
                        break;
                    prev = shard++;
                }

                if (shard->left != store_key_t(key))
                    throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);

                // Create the new shard to be inserted
                key_range_t merged;
                merged.left = prev->left;
                merged.right = shard->right;

                ns.shards.get_mutable().erase(shard);
                ns.shards.get_mutable().erase(prev);
                ns.shards.get_mutable().insert(merged);
            } catch (std::exception& ex) {
                printf("%s\n", ex.what());
                errored = true;
            }
        }

        // Any time shards are changed, we destroy existing pinnings
        // Use 'resolve' because they should be cleared even if in conflict
        // ID sent to the vector clock doesn't matter here since we're setting the metadata through HTTP (TODO: change this if that is no longer true)
        region_map_t<memcached_protocol_t, machine_id_t> new_primaries(memcached_protocol_t::region_t::universe(), nil_uuid());
        region_map_t<memcached_protocol_t, std::set<machine_id_t> > new_secondaries(memcached_protocol_t::region_t::universe(), std::set<machine_id_t>());

        ns.primary_pinnings = ns.primary_pinnings.make_resolving_version(new_primaries, connectivity_cluster.get_me().get_uuid());
        ns.secondary_pinnings = ns.secondary_pinnings.make_resolving_version(new_secondaries, connectivity_cluster.get_me().get_uuid());

        std::string post_path(path_to_str(ns_path));
        post_metadata(post_path + "/shards", ns.shards.get_mutable());
        post_metadata(post_path + "/primary_pinnings/resolve", ns.primary_pinnings.get_mutable());
        post_metadata(post_path + "/secondary_pinnings/resolve", ns.secondary_pinnings.get_mutable());

    } else if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("merging not supported for dummy namespaces");
    } else
        throw admin_cluster_exc_t("invalid object type");

    if (errored && split_points.size() > 1)
        throw admin_cluster_exc_t("not all split points were successfully removed");
}

void admin_cluster_link_t::do_admin_list(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string id = (data.params.count("filter") == 0 ? "" : data.params["filter"][0]);
    bool long_format = (data.params.count("long") == 1);

    if (id != "namespaces" && data.params.count("protocol") != 0)
        throw admin_parse_exc_t("--protocol option only valid when listing namespaces");

    if (id == "machines") {
        list_machines(long_format, cluster_metadata);
    } else if (id == "datacenters") {
        list_datacenters(long_format, cluster_metadata);
    } else if (id == "namespaces") {
        if (data.params.count("protocol") == 0)
            list_namespaces("", long_format, cluster_metadata);
        else
            list_namespaces(data.params["protocol"][0], long_format, cluster_metadata);
    } else if (id == "issues") {
        list_issues(long_format);
    } else if (id == "directory") {
        list_directory(long_format);
    } else if (id == "stats") {
        list_stats(data);
    } else if (id.empty()) {
        list_all(long_format, cluster_metadata);
    } else {
        // TODO: special formatting for each object type, instead of JSON
        metadata_info_t *info = get_info_from_id(id);
        boost::uuids::uuid obj_id = str_to_uuid(info->uuid);
        if (info->path[0] == "datacenters") {
            datacenters_semilattice_metadata_t::datacenter_map_t::iterator i = cluster_metadata.datacenters.datacenters.find(obj_id);
            if (i == cluster_metadata.datacenters.datacenters.end() || i->second.is_deleted())
                throw admin_cluster_exc_t("object not found: " + id);
            list_single_datacenter(obj_id, i->second.get_mutable(), cluster_metadata);
        } else if (info->path[0] == "dummy_namespaces") {
            namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(obj_id);
            if (i == cluster_metadata.dummy_namespaces.namespaces.end() || i->second.is_deleted())
                throw admin_cluster_exc_t("object not found: " + id);
            list_single_namespace(obj_id, i->second.get_mutable(), cluster_metadata, "dummy");
        } else if (info->path[0] == "memcached_namespaces") {
            namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(obj_id);
            if (i == cluster_metadata.memcached_namespaces.namespaces.end() || i->second.is_deleted())
                throw admin_cluster_exc_t("object not found: " + id);
            list_single_namespace(obj_id, i->second.get_mutable(), cluster_metadata, "memcached");
        } else if (info->path[0] == "machines") {
            machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.find(obj_id);
            if (i == cluster_metadata.machines.machines.end() || i->second.is_deleted())
                throw admin_cluster_exc_t("object not found: " + id);
            list_single_machine(obj_id, i->second.get_mutable(), cluster_metadata);
        } else
            throw admin_cluster_exc_t("unexpected error, object found, but type not recognized");
    }
}

template <class protocol_t>
void admin_cluster_link_t::list_pinnings(namespace_semilattice_metadata_t<protocol_t>& ns, const shard_input_t& shard_in, cluster_semilattice_metadata_t& cluster_metadata) {
    if (ns.blueprint.in_conflict())
        throw admin_cluster_exc_t("namespace blueprint is in conflict");
    if (ns.shards.in_conflict())
        throw admin_cluster_exc_t("namespace shards are in conflict");

    // Search through for the shard
    typename protocol_t::region_t shard = find_shard_in_namespace(ns, shard_in);

    list_pinnings_internal(ns.blueprint.get(), shard, cluster_metadata);
}

template <class bp_type>
void admin_cluster_link_t::list_pinnings_internal(const bp_type& bp,
                                                  const key_range_t& shard,
                                                  cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("type");
    delta.push_back("machine");
    delta.push_back("datacenter");
    table.push_back(delta);

    for (typename bp_type::role_map_t::const_iterator i = bp.machines_roles.begin(); i != bp.machines_roles.end(); ++i) {
        typename bp_type::region_to_role_map_t::const_iterator j = i->second.find(shard);
        if (j != i->second.end() && j->second != blueprint_details::role_nothing) {
            delta.clear();

            if (j->second == blueprint_details::role_primary)
                delta.push_back("primary");
            else if (j->second == blueprint_details::role_secondary)
                delta.push_back("secondary");
            else
                throw admin_cluster_exc_t("unexpected error, unrecognized role type encountered");

            delta.push_back(truncate_uuid(i->first));

            // Find the machine to get the datacenter
            machines_semilattice_metadata_t::machine_map_t::iterator m = cluster_metadata.machines.machines.find(i->first);
            if (m == cluster_metadata.machines.machines.end() || m->second.is_deleted())
                throw admin_cluster_exc_t("unexpected error, blueprint invalid");
            else if (m->second.get().datacenter.in_conflict())
                delta.push_back("<conflict>");
            else
                delta.push_back(truncate_uuid(m->second.get().datacenter.get()));

            table.push_back(delta);
        }
    }

    if (table.size() > 1)
        admin_print_table(table);
}

struct admin_stats_request_t {
    explicit admin_stats_request_t(mailbox_manager_t *mailbox_manager) :
        response_mailbox(mailbox_manager,
                         boost::bind(&promise_t<perfmon_result_t>::pulse, &stats_promise, _1),
                         mailbox_callback_mode_inline) { }
    promise_t<perfmon_result_t> stats_promise;
    mailbox_t<void(perfmon_result_t)> response_mailbox;
};

void admin_cluster_link_t::list_stats(admin_command_parser_t::command_data& data) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager.get_root_view()->get();
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    boost::ptr_map<machine_id_t, admin_stats_request_t> request_map;
    signal_timer_t timer(5000); // 5 second timeout to get all stats

    // Get the set of machines to request stats from and construct mailboxes for the responses
    if (data.params.count("machine") == 0) {
        for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.begin();
             i != cluster_metadata.machines.machines.end(); ++i)
            if (!i->second.is_deleted()) {
                machine_id_t target = i->first;
                request_map.insert(target, new admin_stats_request_t(&mailbox_manager));
            }
    } else {
        std::string machine_str(data.params["machine"][0]);
        metadata_info_t *info = get_info_from_id(machine_str);
        if (info->path[0] != "machines")
            throw admin_parse_exc_t("object is not a machine: " + machine_str);
        machine_id_t target = str_to_uuid(info->uuid);
        request_map.insert(target, new admin_stats_request_t(&mailbox_manager));
    }

    if (request_map.empty())
        throw admin_cluster_exc_t("no machines to query stats from");

    // Send the requests
    for (boost::ptr_map<machine_id_t, admin_stats_request_t>::iterator i = request_map.begin(); i != request_map.end(); ++i) {
        bool found = false;
        // Find machine in directory
        // TODO: do a better than linear search
        for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator j = directory.begin(); j != directory.end(); ++j) {
            if (j->second.machine_id == i->first) {
                found = true;
                send(&mailbox_manager,
                     j->second.get_stats_mailbox_address,
                     i->second->response_mailbox.get_address(),
                     std::set<stat_manager_t::stat_id_t>());
            }
        }

        if (!found)
            throw admin_cluster_exc_t("Could not locate machine in directory: " + uuid_to_str(i->first));
    }

    // Wait for responses and output them, filtering as necessary
    for (boost::ptr_map<machine_id_t, admin_stats_request_t>::iterator i = request_map.begin(); i != request_map.end(); ++i) {
        signal_t *stats_ready = i->second->stats_promise.get_ready_signal();
        wait_any_t waiter(&timer, stats_ready);
        waiter.wait();

        if (stats_ready->is_pulsed()) {
            perfmon_result_t stats = i->second->stats_promise.wait();
            printf("machine %s:\n%s\n", uuid_to_str(i->first).c_str(), admin_value_to_string(stats).c_str());
        }
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
    for (std::list<clone_ptr_t<global_issue_t> >::iterator i = issues.begin(); i != issues.end(); ++i) {
        puts((*i)->get_description().c_str());
    }
}

template <class map_type>
void admin_cluster_link_t::list_all_internal(const std::string& type, bool long_format, map_type& obj_map, std::vector<std::vector<std::string> >& table) {
    std::vector<std::string> delta;
    for (typename map_type::iterator i = obj_map.begin(); i != obj_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            delta.push_back(type);

            if (long_format)
                delta.push_back(uuid_to_str(i->first));
            else
                delta.push_back(truncate_uuid(i->first));

            if (i->second.get().name.in_conflict())
                delta.push_back("<conflict>");
            else
                delta.push_back(i->second.get().name.get());

            table.push_back(delta);
        }
    }
}

void admin_cluster_link_t::list_all(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("type");
    delta.push_back("uuid");
    delta.push_back("name");
    table.push_back(delta);

    list_all_internal("machine", long_format, cluster_metadata.machines.machines, table);
    list_all_internal("datacenter", long_format, cluster_metadata.datacenters.datacenters, table);
    // TODO: better differentiation between namespace types
    list_all_internal("namespace (d)", long_format, cluster_metadata.dummy_namespaces.namespaces, table);
    list_all_internal("namespace (m)", long_format, cluster_metadata.memcached_namespaces.namespaces, table);

    if (table.size() > 1)
        admin_print_table(table);
}

std::map<datacenter_id_t, admin_cluster_link_t::datacenter_info_t> admin_cluster_link_t::build_datacenter_info(cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<datacenter_id_t, datacenter_info_t> results;
    std::map<machine_id_t, machine_info_t> machine_data = build_machine_info(cluster_metadata);

    for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted() && !i->second.get().datacenter.in_conflict()) {
            datacenter_id_t datacenter = i->second.get().datacenter.get();

            results[datacenter].machines += 1;

            std::map<machine_id_t, machine_info_t>::iterator info = machine_data.find(i->first);
            if (info != machine_data.end()) {
                results[datacenter].primaries += info->second.primaries;
                results[datacenter].secondaries += info->second.secondaries;
            }
        }
    }

    // TODO: this will list affinities, but not actual state (in case of impossible requirements)
    add_datacenter_affinities(cluster_metadata.dummy_namespaces.namespaces, results);
    add_datacenter_affinities(cluster_metadata.memcached_namespaces.namespaces, results);

    return results;
}

template <class map_type>
void admin_cluster_link_t::add_datacenter_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t>& results) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i)
        if (!i->second.is_deleted()) {
            if (!i->second.get().primary_datacenter.in_conflict())
                ++results[i->second.get().primary_datacenter.get()].namespaces;

            if (!i->second.get().replica_affinities.in_conflict()) {
                std::map<datacenter_id_t, int> affinities = i->second.get().replica_affinities.get();
                for (std::map<datacenter_id_t, int>::iterator j = affinities.begin(); j != affinities.end(); ++j)
                    if (j->second > 0)
                        ++results[j->first].namespaces;
            }
        }
}

void admin_cluster_link_t::list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;
    std::map<datacenter_id_t, datacenter_info_t> long_info;

    delta.push_back("uuid");
    delta.push_back("name");
    if (long_format) {
        delta.push_back("machines");
        delta.push_back("namespaces");
        delta.push_back("primaries");
        delta.push_back("secondaries");
    }

    table.push_back(delta);

    if (long_format)
        long_info = build_datacenter_info(cluster_metadata);

    for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator i = cluster_metadata.datacenters.datacenters.begin(); i != cluster_metadata.datacenters.datacenters.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();
            if (long_format)
                delta.push_back(uuid_to_str(i->first));
            else
                delta.push_back(truncate_uuid(i->first));
            if (i->second.get().name.in_conflict())
                delta.push_back("<conflict>");
            else
                delta.push_back(i->second.get().name.get());
            if (long_format) {
                char buffer[64];
                datacenter_info_t info = long_info[i->first];
                snprintf(buffer, sizeof(buffer), "%ld", info.machines);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%ld", info.namespaces);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%ld", info.primaries);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%ld", info.secondaries);
                delta.push_back(buffer);
            }
            table.push_back(delta);
        }
    }

    if (table.size() > 1)
        admin_print_table(table);
}

template <class ns_type>
admin_cluster_link_t::namespace_info_t admin_cluster_link_t::get_namespace_info(ns_type& ns) {
    namespace_info_t result;

    if (ns.shards.in_conflict())
        result.shards = -1;
    else
        result.shards = ns.shards.get().size();

    // For replicas, go through the blueprint and sum up all roles
    if (ns.blueprint.in_conflict())
        result.replicas = -1;
    else
        result.replicas = get_replica_count_from_blueprint(ns.blueprint.get_mutable());

    if (ns.primary_datacenter.in_conflict())
        result.primary.assign("<conflict>");
    else
        result.primary.assign(uuid_to_str(ns.primary_datacenter.get()));

    return result;
}

template <class bp_type>
size_t admin_cluster_link_t::get_replica_count_from_blueprint(const bp_type& bp) {
    size_t count = 0;

    for (typename bp_type::role_map_t::const_iterator j = bp.machines_roles.begin();
         j != bp.machines_roles.end(); ++j) {
        for (typename bp_type::region_to_role_map_t::const_iterator k = j->second.begin();
             k != j->second.end(); ++k) {
            if (k->second == blueprint_details::role_primary)
                ++count;
            else if (k->second == blueprint_details::role_secondary)
                ++count;
        }
    }
    return count;
}

void admin_cluster_link_t::list_namespaces(const std::string& type, bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> header;

    header.push_back("uuid");
    header.push_back("name");
    header.push_back("protocol");
    if (long_format) {
        header.push_back("shards");
        header.push_back("replicas");
        header.push_back("primary");
    }

    table.push_back(header);

    if (type.empty()) {
        add_namespaces("dummy", long_format, cluster_metadata.dummy_namespaces.namespaces, table);
        add_namespaces("memcached", long_format, cluster_metadata.memcached_namespaces.namespaces, table);
    } else if (type == "dummy") {
        add_namespaces(type, long_format, cluster_metadata.dummy_namespaces.namespaces, table);
    } else if (type == "memcached") {
        add_namespaces(type, long_format, cluster_metadata.memcached_namespaces.namespaces, table);
    } else
        throw admin_parse_exc_t("unrecognized namespace type: " + type);

    if (table.size() > 1)
        admin_print_table(table);
}

template <class map_type>
void admin_cluster_link_t::add_namespaces(const std::string& protocol, bool long_format, map_type& namespaces, std::vector<std::vector<std::string> >& table) {

    std::vector<std::string> delta;
    for (typename map_type::iterator i = namespaces.begin(); i != namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            if (long_format)
                delta.push_back(uuid_to_str(i->first));
            else
                delta.push_back(truncate_uuid(i->first));

            if (!i->second.get().name.in_conflict())
                delta.push_back(i->second.get().name.get());
            else
                delta.push_back("<conflict>");

            delta.push_back(protocol);

            if (long_format) {
                char buffer[64];
                namespace_info_t info = get_namespace_info(i->second.get_mutable());

                if (info.shards != -1) {
                    snprintf(buffer, sizeof(buffer), "%i", info.shards);
                    delta.push_back(buffer);
                } else
                    delta.push_back("<conflict>");

                if (info.replicas != -1) {
                    snprintf(buffer, sizeof(buffer), "%i", info.replicas);
                    delta.push_back(buffer);
                } else
                    delta.push_back("<conflict>");

                delta.push_back(info.primary);
            }

            table.push_back(delta);
        }
    }
}

std::map<machine_id_t, admin_cluster_link_t::machine_info_t> admin_cluster_link_t::build_machine_info(cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager.get_root_view()->get();
    std::map<machine_id_t, machine_info_t> results;

    // Initialize each machine, reachable machines are in the directory
    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); ++i) {
        results.insert(std::make_pair(i->second.machine_id, machine_info_t()));
        results[i->second.machine_id].status.assign("reach");
    }

    // Unreachable machines will be found in the metadata but not the directory
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i)
        if (!i->second.is_deleted()) {
            if (results.count(i->first) == 0) {
                results.insert(std::make_pair(i->first, machine_info_t()));
                results[i->first].status.assign("unreach");
            }
        }

    // Go through namespaces
    build_machine_info_internal(cluster_metadata.dummy_namespaces.namespaces, results);
    build_machine_info_internal(cluster_metadata.memcached_namespaces.namespaces, results);

    return results;
}

template <class map_type>
void admin_cluster_link_t::build_machine_info_internal(const map_type& ns_map, std::map<machine_id_t, machine_info_t>& results) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (i->second.is_deleted() || i->second.get().blueprint.in_conflict())
            continue;

        add_machine_info_from_blueprint(i->second.get().blueprint.get_mutable(), results);
    }
}

template <class bp_type>
void admin_cluster_link_t::add_machine_info_from_blueprint(const bp_type& bp, std::map<machine_id_t, machine_info_t>& results) {
    for (typename bp_type::role_map_t::const_iterator j = bp.machines_roles.begin();
         j != bp.machines_roles.end(); ++j) {
        if (results.count(j->first) == 0)
            continue;

        bool machine_used = false;

        for (typename bp_type::region_to_role_map_t::const_iterator k = j->second.begin();
             k != j->second.end(); ++k) {
            if (k->second == blueprint_details::role_primary) {
                ++results[j->first].primaries;
                machine_used = true;
            } else if (k->second == blueprint_details::role_secondary) {
                ++results[j->first].secondaries;
                machine_used = true;
            }
        }

        if (machine_used)
            ++results[j->first].namespaces;
    }
}

void admin_cluster_link_t::list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<machine_id_t, machine_info_t> long_info;
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("uuid");
    delta.push_back("name");
    delta.push_back("datacenter");
    if (long_format) {
        delta.push_back("status");
        delta.push_back("namespaces");
        delta.push_back("primaries");
        delta.push_back("secondaries");
    }

    table.push_back(delta);

    if (long_format)
        long_info = build_machine_info(cluster_metadata);

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            if (long_format)
                delta.push_back(uuid_to_str(i->first));
            else
                delta.push_back(truncate_uuid(i->first));

            if (!i->second.get().name.in_conflict())
                delta.push_back(i->second.get().name.get());
            else
                delta.push_back("<conflict>");

            if (!i->second.get().datacenter.in_conflict()) {
                if (i->second.get().datacenter.get().is_nil())
                    delta.push_back("none");
                else if (long_format)
                    delta.push_back(uuid_to_str(i->second.get().datacenter.get()));
                else
                    delta.push_back(truncate_uuid(i->second.get().datacenter.get()));
            } else
                delta.push_back("<conflict>");

            if (long_format) {
                char buffer[64];
                machine_info_t info = long_info[i->first];
                delta.push_back(info.status);
                snprintf(buffer, sizeof(buffer), "%ld", info.namespaces);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%ld", info.primaries);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%ld", info.secondaries);
                delta.push_back(buffer);
            }

            table.push_back(delta);
        }
    }

    // TODO: sort by datacenter and name

    if (table.size() > 1)
        admin_print_table(table);
}

void admin_cluster_link_t::do_admin_create_datacenter(admin_command_parser_t::command_data& data) {
    std::string uuid = create_metadata("datacenters");
    post_metadata("datacenters/" + uuid + "/name", data.params["name"][0]);
    printf("uuid: %s\n", uuid.c_str());
}


void admin_cluster_link_t::do_admin_create_namespace(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string protocol(data.params["protocol"][0]);
    std::string port_str(data.params["port"][0]);
    std::string name(data.params["name"][0]);
    int port = atoi(port_str.c_str());
    std::string datacenter_id(data.params["primary"][0]);
    metadata_info_t *datacenter_info(get_info_from_id(datacenter_id));
    datacenter_id_t primary(str_to_uuid(datacenter_info->path[1]));

    // Make sure port is a number
    for (size_t i = 0; i < port_str.length(); ++i)
        if (port_str[i] < '0' || port_str[i] > '9')
            throw admin_parse_exc_t("port is not a number");

    if (port > 65536)
        throw admin_parse_exc_t("port is too large: " + port_str);

    if (datacenter_info->path[0] != "datacenters")
        throw admin_parse_exc_t("namespace primary is not a datacenter: " + datacenter_id);

    if (protocol == "memcached")
        do_admin_create_namespace_internal<memcached_protocol_t>(name, port, primary, "memcached_namespaces");
    else if (protocol == "dummy")
        do_admin_create_namespace_internal<mock::dummy_protocol_t>(name, port, primary, "dummy_namespaces");
    else
        throw admin_parse_exc_t("unrecognized protocol: " + protocol);
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_create_namespace_internal(std::string& name,
                                                              int port,
                                                              datacenter_id_t& primary,
                                                              const std::string& path) {
    std::string uuid = create_metadata(path);
    std::string full_path(path + "/" + uuid);

    post_metadata(full_path + "/name", name);
    post_metadata(full_path + "/primary_uuid", primary);
    post_metadata(full_path + "/port", port);

    printf("uuid: %s\n", uuid.c_str());
}

void admin_cluster_link_t::do_admin_set_datacenter(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string obj_id(data.params["id"][0]);
    metadata_info_t *obj_info(get_info_from_id(obj_id));
    std::string datacenter_id(data.params["datacenter"][0]);
    metadata_info_t *datacenter_info(get_info_from_id(datacenter_id));
    datacenter_id_t datacenter_uuid(str_to_uuid(datacenter_info->uuid));

    // Target must be a datacenter in all existing use cases
    if (datacenter_info->path[0] != "datacenters")
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);

    std::string post_path(path_to_str(obj_info->path));
    if (obj_info->path[0] == "memcached_namespaces" || obj_info->path[0] == "dummy_namespaces")
        post_path += "/primary_uuid";
    else if (obj_info->path[0] == "machines") {
        post_path += "/datacenter_uuid";
        // Any pinnings involving this machine must be culled
        datacenter_id_t old_datacenter(datacenter_uuid);
        machine_id_t machine(str_to_uuid(obj_info->uuid));

        machines_semilattice_metadata_t::machine_map_t::iterator m = cluster_metadata.machines.machines.find(machine);
        if (m != cluster_metadata.machines.machines.end() && !m->second.is_deleted() && !m->second.get().datacenter.in_conflict())
            old_datacenter = m->second.get().datacenter.get();

        // If the datacenter has changed (or we couldn't determine the old datacenter uuid), clear pinnings
        if (old_datacenter != datacenter_uuid) {
            remove_machine_pinnings(machine, "memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
            remove_machine_pinnings(machine, "dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
        }
    } else
        throw admin_cluster_exc_t("target object is not a namespace or machine");

    post_metadata(post_path, datacenter_uuid);
}

template <class protocol_t>
void admin_cluster_link_t::remove_machine_pinnings(const machine_id_t& machine,
                                                   const std::string& post_path,
                                                   std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map) {
    // TODO: what if a pinning is in conflict with this machine specified, but is resolved later?
    // perhaps when a resolve is issued, check to make sure it is still valid

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        namespace_semilattice_metadata_t<protocol_t>& ns = i->second.get_mutable();
        std::string ns_post_path = post_path + "/" + uuid_to_str(i->first);
        bool changed = false;

        // Check for and remove the machine in primary pinnings
        if (!ns.primary_pinnings.in_conflict())
            for (typename region_map_t<protocol_t, machine_id_t>::iterator j = ns.primary_pinnings.get_mutable().begin();
                 j != ns.primary_pinnings.get_mutable().end(); ++j) {
                if (j->second == machine) {
                    changed = true;
                    j->second = nil_uuid();
                }
            }

        if (changed)
            post_metadata(ns_post_path + "/primary_pinnings", ns.primary_pinnings.get_mutable());

        changed = false;
        // Check for and remove the machine in secondary pinnings
        if (!ns.secondary_pinnings.in_conflict())
            for (typename region_map_t<protocol_t, std::set<machine_id_t> >::iterator j = ns.secondary_pinnings.get_mutable().begin();
                 j != ns.secondary_pinnings.get_mutable().end(); ++j)
                changed |= (j->second.erase(machine) > 0);

        if (changed)
            post_metadata(ns_post_path + "/secondary_pinnings", ns.secondary_pinnings.get_mutable());
    }
}

void admin_cluster_link_t::do_admin_set_name(admin_command_parser_t::command_data& data) {
    // TODO: make sure names aren't silly things like uuids or reserved strings
    std::vector<std::string> obj_path(get_info_from_id(data.params["id"][0])->path);

    std::string post_path(path_to_str(obj_path));
    post_path += "/name";
    post_metadata(post_path, data.params["new-name"][0]);
}

void admin_cluster_link_t::do_admin_set_acks(admin_command_parser_t::command_data& data) {
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> dc_path(get_info_from_id(data.params["datacenter"][0])->path);
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string acks_str = data.params["num-acks"][0].c_str();
    std::string post_path(path_to_str(ns_path));
    post_path += "/ack_expectations";

    // Make sure num-acks is a number
    for (size_t i = 0; i < acks_str.length(); ++i)
        if (acks_str[i] < '0' || acks_str[i] > '9')
            throw admin_parse_exc_t("num-acks is not a number");

    if (dc_path[0] != "datacenters")
        throw admin_parse_exc_t(data.params["datacenter"][0] + " is not a datacenter");

    if (ns_path[0] == "dummy_namespaces") {
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.dummy_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_acks_internal(i->second.get_mutable(), str_to_uuid(dc_path[1]), atoi(acks_str.c_str()), post_path);

    } else if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_acks_internal(i->second.get_mutable(), str_to_uuid(dc_path[1]), atoi(acks_str.c_str()), post_path);

    } else
        throw admin_parse_exc_t(data.params["namespace"][0] + " is not a namespace");
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_set_acks_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_acks, const std::string& post_path) {
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

    ns.ack_expectations.get_mutable()[datacenter] = num_acks;

    post_metadata(post_path, ns.ack_expectations.get_mutable());
}

void admin_cluster_link_t::do_admin_set_replicas(admin_command_parser_t::command_data& data) {
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> dc_path(get_info_from_id(data.params["datacenter"][0])->path);
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string replicas_str = data.params["num-replicas"][0].c_str();
    int num_replicas = atoi(replicas_str.c_str());
    std::string post_path(path_to_str(ns_path));
    post_path += "/replica_affinities";

    // Make sure num-acks is a number
    for (size_t i = 0; i < replicas_str.length(); ++i)
        if (replicas_str[i] < '0' || replicas_str[i] > '9')
            throw admin_parse_exc_t("num-replicas is not a number");

    if (dc_path[0] != "datacenters")
        throw admin_parse_exc_t(data.params["datacenter"][0] + " is not a datacenter");

    datacenter_id_t datacenter(str_to_uuid(dc_path[1]));
    if (get_machine_count_in_datacenter(cluster_metadata, datacenter) < (size_t)num_replicas)
        throw admin_cluster_exc_t("the number of replicas cannot be more than the number of machines in the datacenter");

    if (ns_path[0] == "dummy_namespaces") {
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.dummy_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_replicas_internal(i->second.get_mutable(), datacenter, num_replicas, post_path);

    } else if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_parse_exc_t("unexpected error, namespace not found");
        if (i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected error, namespace has been deleted");
        do_admin_set_replicas_internal(i->second.get_mutable(), datacenter, num_replicas, post_path);

    } else
        throw admin_parse_exc_t(data.params["namespace"][0] + " is not a namespace");
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_set_replicas_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_replicas, const std::string& post_path) {
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
    if (i == ns.ack_expectations.get().end())
        ns.ack_expectations.get_mutable()[datacenter] = 0;
    else if (i->second > num_replicas)
        throw admin_cluster_exc_t("the number of replicas for this datacenter cannot be less than the number of acks");

    ns.replica_affinities.get_mutable()[datacenter] = num_replicas - (is_primary ? 1 : 0);

    post_metadata(post_path, ns.replica_affinities.get_mutable());
}

void admin_cluster_link_t::do_admin_remove(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> ids = data.params["id"];
    bool errored = false;

    for (size_t i = 0; i < ids.size(); ++i) {
        try {
            metadata_info_t *obj_info = get_info_from_id(ids[i]);

            // TODO: in case of machine, check if it's up and ask for confirmation if it is

            delete_metadata(path_to_str(obj_info->path));

            // Clean up any hanging references
            if (obj_info->path[0] == "machines") {
                machine_id_t machine(str_to_uuid(obj_info->uuid));
                remove_machine_pinnings(machine, "memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
                remove_machine_pinnings(machine, "dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
            } else if (obj_info->path[0] == "datacenters") {
                datacenter_id_t datacenter(str_to_uuid(obj_info->uuid));
                remove_datacenter_references(datacenter, cluster_metadata);
            }
        } catch (std::exception& ex) {
            puts(ex.what());
            errored = true;
        }
    }

    if (errored)
        throw admin_cluster_exc_t("not all removes were successful");
}

void admin_cluster_link_t::remove_datacenter_references(const datacenter_id_t& datacenter, cluster_semilattice_metadata_t& cluster_metadata) {
    datacenter_id_t nil_id(nil_uuid());

    // Go through machines
    for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        std::string machine_post_path("machines/" + uuid_to_str(i->first));
        if (!i->second.get().datacenter.in_conflict() && i->second.get().datacenter.get() == datacenter)
            post_metadata(machine_post_path + "/datacenter_uuid", nil_id);
    }

    remove_datacenter_references_from_namespaces(datacenter, "memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
    remove_datacenter_references_from_namespaces(datacenter, "dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
}

template <class protocol_t>
void admin_cluster_link_t::remove_datacenter_references_from_namespaces(const datacenter_id_t& datacenter,
                                                                        const std::string& post_path,
                                                                        std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map) {
    datacenter_id_t nil_id(nil_uuid());

    for (typename std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >::iterator i = ns_map.begin();
         i != ns_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        namespace_semilattice_metadata_t<protocol_t>& ns = i->second.get_mutable();
        std::string ns_post_path(post_path + "/" + uuid_to_str(i->first));

        if (!ns.primary_datacenter.in_conflict() && ns.primary_datacenter.get() == datacenter)
            post_metadata(ns_post_path + "/primary_uuid", nil_id);

        if (!ns.replica_affinities.in_conflict() && ns.replica_affinities.get_mutable().erase(datacenter) > 0)
            post_metadata(ns_post_path + "/replica_affinities", ns.replica_affinities.get_mutable());

        if (!ns.ack_expectations.in_conflict() && ns.ack_expectations.get_mutable().erase(datacenter) > 0)
            post_metadata(ns_post_path + "/ack_expectations", ns.ack_expectations.get_mutable());
    }
}

template <class protocol_t>
void admin_cluster_link_t::list_single_namespace(const namespace_id_t& ns_id,
                                                 namespace_semilattice_metadata_t<protocol_t>& ns,
                                                 cluster_semilattice_metadata_t& cluster_metadata,
                                                 const std::string& protocol) {
    if (ns.name.in_conflict() || ns.name.get_mutable().empty())
        printf("namespace %s\n", uuid_to_str(ns_id).c_str());
    else
        printf("namespace '%s' %s\n", ns.name.get_mutable().c_str(), uuid_to_str(ns_id).c_str());

    // Print primary datacenter
    if (!ns.primary_datacenter.in_conflict()) {
        datacenters_semilattice_metadata_t::datacenter_map_t::iterator dc = cluster_metadata.datacenters.datacenters.find(ns.primary_datacenter.get());
        if (dc == cluster_metadata.datacenters.datacenters.end() ||
            dc->second.is_deleted() ||
            dc->second.get_mutable().name.in_conflict() ||
            dc->second.get_mutable().name.get().empty())
            printf("primary datacenter %s\n", uuid_to_str(ns.primary_datacenter.get()).c_str());
        else
            printf("primary datacenter '%s' %s\n", dc->second.get_mutable().name.get().c_str(), uuid_to_str(ns.primary_datacenter.get()).c_str());
    } else
        printf("primary datacenter <conflict>\n");

    // Print port
    if (ns.port.in_conflict())
        printf("running %s protocol on port <conflict>\n", protocol.c_str());
    else
        printf("running %s protocol on port %i\n", protocol.c_str(), ns.port.get());
    printf("\n");

    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    // Print configured affinities and ack expectations
    delta.push_back("uuid");
    delta.push_back("name");
    delta.push_back("replicas");
    delta.push_back("acks");
    table.push_back(delta);

    for (datacenters_semilattice_metadata_t::datacenter_map_t::iterator i = cluster_metadata.datacenters.datacenters.begin();
         i != cluster_metadata.datacenters.datacenters.end(); ++i) {
        if (!i->second.is_deleted()) {
            bool affinity = false;
            delta.clear();
            delta.push_back(uuid_to_str(i->first));
            delta.push_back(i->second.get_mutable().name.in_conflict() ? "<conflict>" : i->second.get_mutable().name.get());

            if (!ns.primary_datacenter.in_conflict() && ns.primary_datacenter.get() == i->first) {
                if (ns.replica_affinities.get_mutable().count(i->first) == 1)
                    delta.push_back(strprintf("%i", ns.replica_affinities.get_mutable()[i->first] + 1));
                else
                    delta.push_back("1");

                affinity = true;
            } else if (ns.replica_affinities.get_mutable().count(i->first) == 1) {
                delta.push_back(strprintf("%i", ns.replica_affinities.get_mutable()[i->first]));
                affinity = true;
            } else
                delta.push_back("0");

            if (ns.ack_expectations.get_mutable().count(i->first) == 1) {
                delta.push_back(strprintf("%i", ns.ack_expectations.get_mutable()[i->first]));
                affinity = true;
            } else
                delta.push_back("0");

            if (affinity)
                table.push_back(delta);
        }
    }

    printf("affinity with %ld datacenter%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1)
        admin_print_table(table);
    printf("\n");

    if (ns.shards.in_conflict()) {
        printf("shards in conflict\n");
    } else if (ns.blueprint.in_conflict()) {
        printf("cluster blueprint in conflict\n");
    } else {
        // Print shard hosting
        table.clear();
        delta.clear();
        delta.push_back("shard");
        delta.push_back("machine uuid");
        delta.push_back("name");
        delta.push_back("primary");
        table.push_back(delta);

        add_single_namespace_replicas(ns.shards.get_mutable(),
                                      ns.blueprint.get_mutable(),
                                      cluster_metadata.machines.machines,
                                      table);

        printf("%ld replica%s for %ld shard%s\n",
               table.size() - 1, table.size() == 2 ? "" : "s",
               ns.shards.get_mutable().size(), ns.shards.get_mutable().size() == 1 ? "" : "s");
        if (table.size() > 1)
            admin_print_table(table);
    }
}

template <class protocol_t>
void admin_cluster_link_t::add_single_namespace_replicas(std::set<typename protocol_t::region_t>& shards,
                                                         persistable_blueprint_t<protocol_t>& blueprint,
                                                         machines_semilattice_metadata_t::machine_map_t& machine_map,
                                                         std::vector<std::vector<std::string> >& table) {
    std::vector<std::string> delta;

    for (typename std::set<typename protocol_t::region_t>::iterator s = shards.begin(); s != shards.end(); ++s) {
        std::string shard_str(admin_value_to_string(*s));

        // First add the primary host
        for (typename persistable_blueprint_t<protocol_t>::role_map_t::iterator i = blueprint.machines_roles.begin();
             i != blueprint.machines_roles.end(); ++i) {
            typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::iterator j = i->second.find(*s);
            if (j != i->second.end() && j->second == blueprint_details::role_primary) {
                delta.clear();
                delta.push_back(shard_str);
                delta.push_back(uuid_to_str(i->first));

                machines_semilattice_metadata_t::machine_map_t::iterator m = machine_map.find(i->first);
                if (m == machine_map.end() || m->second.is_deleted()) // This shouldn't really happen, but oh well
                    delta.push_back(std::string());
                else if (m->second.get_mutable().name.in_conflict())
                    delta.push_back("<conflict>");
                else
                    delta.push_back(m->second.get_mutable().name.get());

                delta.push_back("yes");
                table.push_back(delta);
            }
        }

        // Then add all the secondaries
        for (typename persistable_blueprint_t<protocol_t>::role_map_t::iterator i = blueprint.machines_roles.begin();
             i != blueprint.machines_roles.end(); ++i) {
            typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::iterator j = i->second.find(*s);
            if (j != i->second.end() && j->second == blueprint_details::role_secondary) {
                delta.clear();
                delta.push_back(shard_str);
                delta.push_back(uuid_to_str(i->first));

                machines_semilattice_metadata_t::machine_map_t::iterator m = machine_map.find(i->first);
                if (m == machine_map.end() || m->second.is_deleted()) // This shouldn't really happen, but oh well
                    delta.push_back(std::string());
                else if (m->second.get_mutable().name.in_conflict())
                    delta.push_back("<conflict>");
                else
                    delta.push_back(m->second.get_mutable().name.get());

                delta.push_back("no");
                table.push_back(delta);
            }
        }
    }
}

void admin_cluster_link_t::list_single_datacenter(const datacenter_id_t& dc_id,
                                                  datacenter_semilattice_metadata_t& dc,
                                                  cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;
    if (dc.name.in_conflict() || dc.name.get_mutable().empty())
        printf("datacenter %s\n", uuid_to_str(dc_id).c_str());
    else
        printf("datacenter '%s' %s\n", dc.name.get_mutable().c_str(), uuid_to_str(dc_id).c_str());
    printf("\n");

    // Get a list of machines in the datacenter
    delta.push_back("uuid");
    delta.push_back("name");
    table.push_back(delta);

    for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted() &&
            !i->second.get_mutable().datacenter.in_conflict() &&
            i->second.get_mutable().datacenter.get() == dc_id) {
            delta.clear();
            delta.push_back(uuid_to_str(i->first));
            delta.push_back(i->second.get_mutable().name.in_conflict() ? "<conflict>" : i->second.get_mutable().name.get());
            table.push_back(delta);
        }
    }

    printf("%ld machine%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1)
        admin_print_table(table);
    printf("\n");

    // Get a list of namespaces hosted by the datacenter
    table.clear();
    delta.clear();
    delta.push_back("uuid");
    delta.push_back("name");
    delta.push_back("protocol");
    delta.push_back("primary");
    delta.push_back("replicas");
    table.push_back(delta);

    add_single_datacenter_affinities(dc_id, cluster_metadata.dummy_namespaces.namespaces, table, "dummy");
    add_single_datacenter_affinities(dc_id, cluster_metadata.memcached_namespaces.namespaces, table, "memcached");

    printf("%ld namespace%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1)
        admin_print_table(table);
}

template <class map_type>
void admin_cluster_link_t::add_single_datacenter_affinities(const datacenter_id_t& dc_id, map_type& ns_map, std::vector<std::vector<std::string> >& table, const std::string& protocol) {
    std::vector<std::string> delta;

    for (typename map_type::iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            typename map_type::mapped_type::value_t& ns = i->second.get_mutable();
            size_t replicas(0);

            delta.clear();
            delta.push_back(uuid_to_str(i->first));
            delta.push_back(ns.name.in_conflict() ? "<conflict>" : ns.name.get());
            delta.push_back(protocol);

            // TODO: this will only list the replicas required by the user, not the actual replicas (in case of impossible requirements)
            if (!ns.primary_datacenter.in_conflict() &&
                ns.primary_datacenter.get() == dc_id) {
                delta.push_back("yes");
                ++replicas;
            } else
                delta.push_back("no");

            if (!ns.replica_affinities.in_conflict() &&
                ns.replica_affinities.get_mutable().count(dc_id) == 1)
                replicas += ns.replica_affinities.get_mutable()[dc_id];

            delta.push_back(strprintf("%ld", replicas));

            if (replicas > 0)
                table.push_back(delta);
        }
    }
}

void admin_cluster_link_t::list_single_machine(const machine_id_t& machine_id,
                                               machine_semilattice_metadata_t& machine,
                                               cluster_semilattice_metadata_t& cluster_metadata) {
    if (machine.name.in_conflict() || machine.name.get_mutable().empty())
        printf("machine %s\n", uuid_to_str(machine_id).c_str());
    else
        printf("machine '%s' %s\n", machine.name.get_mutable().c_str(), uuid_to_str(machine_id).c_str());

    // Print datacenter
    if (!machine.datacenter.in_conflict()) {
        datacenters_semilattice_metadata_t::datacenter_map_t::iterator dc = cluster_metadata.datacenters.datacenters.find(machine.datacenter.get());
        if (dc == cluster_metadata.datacenters.datacenters.end() ||
            dc->second.is_deleted() ||
            dc->second.get_mutable().name.in_conflict() ||
            dc->second.get_mutable().name.get().empty())
            printf("in datacenter %s\n", uuid_to_str(machine.datacenter.get()).c_str());
        else
            printf("in datacenter '%s' %s\n", dc->second.get_mutable().name.get().c_str(), uuid_to_str(machine.datacenter.get()).c_str());
    } else
        printf("in datacenter <conflict>\n");
    printf("\n");

    // Print hosted replicas
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> header;

    header.push_back("namespace");
    header.push_back("name");
    header.push_back("shard");
    header.push_back("primary");
    table.push_back(header);

    size_t namespace_count = 0;
    namespace_count += add_single_machine_replicas(machine_id, cluster_metadata.dummy_namespaces.namespaces, table);
    namespace_count += add_single_machine_replicas(machine_id, cluster_metadata.memcached_namespaces.namespaces, table);

    printf("hosting %ld replica%s from %ld namespace%s\n", table.size() - 1, table.size() == 2 ? "" : "s", namespace_count, namespace_count == 1 ? "" : "s");
    if (table.size() > 1)
        admin_print_table(table);
}

template <class map_type>
size_t admin_cluster_link_t::add_single_machine_replicas(const machine_id_t& machine_id,
                                                         map_type& ns_map,
                                                         std::vector<std::vector<std::string> >& table) {
    size_t matches(0);

    for (typename map_type::iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        // TODO: if there is a blueprint conflict, the values won't be accurate
        if (!i->second.is_deleted() && !i->second.get_mutable().blueprint.in_conflict()) {
            typename map_type::mapped_type::value_t& ns = i->second.get_mutable();
            std::string uuid(uuid_to_str(i->first));
            std::string name(ns.name.in_conflict() ? "<conflict>" : ns.name.get_mutable());
            matches += add_single_machine_blueprint(machine_id, ns.blueprint.get_mutable(), table, uuid, name);
        }
    }

    return matches;
}

template <class protocol_t>
bool admin_cluster_link_t::add_single_machine_blueprint(const machine_id_t& machine_id,
                                                        persistable_blueprint_t<protocol_t>& blueprint,
                                                        std::vector<std::vector<std::string> >& table,
                                                        const std::string& ns_uuid,
                                                        const std::string& ns_name) {
    std::vector<std::string> delta;
    bool match(false);

    typename persistable_blueprint_t<protocol_t>::role_map_t::iterator machine_entry = blueprint.machines_roles.find(machine_id);
    if (machine_entry == blueprint.machines_roles.end())
        return false;

    for (typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::iterator i = machine_entry->second.begin();
         i != machine_entry->second.end(); ++i) {
        if (i->second == blueprint_details::role_primary || i->second == blueprint_details::role_secondary) {
            delta.push_back(ns_uuid);
            delta.push_back(ns_name);

            // Build a string for the shard
            delta.push_back(admin_value_to_string(i->first));

            if (i->second == blueprint_details::role_primary)
                delta.push_back("yes");
            else
                delta.push_back("no");

            table.push_back(delta);
            match = true;
        } else
            continue;
    }


    return match;
}

void admin_cluster_link_t::do_admin_resolve(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string obj_id = data.params["id"][0];
    std::string field = data.params["field"][0];
    metadata_info_t *obj_info = get_info_from_id(obj_id);

    if (obj_info->path[0] == "machines") {
        machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.find(str_to_uuid(obj_info->uuid));
        if (i == cluster_metadata.machines.machines.end() || i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        resolve_machine_value(i->second.get_mutable(), field, "machines" + obj_info->uuid);
    } else if (obj_info->path[0] == "datacenters") {
        datacenters_semilattice_metadata_t::datacenter_map_t::iterator i = cluster_metadata.datacenters.datacenters.find(str_to_uuid(obj_info->uuid));
        if (i == cluster_metadata.datacenters.datacenters.end() || i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        resolve_datacenter_value(i->second.get_mutable(), field, "datacenters" + obj_info->uuid);
    } else if (obj_info->path[0] == "dummy_namespaces") {
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = cluster_metadata.dummy_namespaces.namespaces.find(str_to_uuid(obj_info->uuid));
        if (i == cluster_metadata.dummy_namespaces.namespaces.end() || i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        resolve_namespace_value(i->second.get_mutable(), field, "dummy_namespaces" + obj_info->uuid);
    } else if (obj_info->path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(obj_info->uuid));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end() || i->second.is_deleted())
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        resolve_namespace_value(i->second.get_mutable(), field, "memcached_namespaces" + obj_info->uuid);
    } else
        throw admin_cluster_exc_t("unexpected object type encountered: " + obj_info->path[0]);
}

template <class T>
void admin_cluster_link_t::resolve_value(const vclock_t<T>& field, const std::string& field_name, const std::string& post_path) {
    if (!field.in_conflict())
        throw admin_cluster_exc_t("value is not in conflict");

    std::vector<T> values = field.get_all_values();

    printf("%ld values\n", values.size());
    for (size_t i = 0; i < values.size(); ++i)
        printf(" %ld: %s\n", i + 1, admin_value_to_string(values[i]).c_str());
    printf(" 0: cancel\n");
    printf("select: ");

    std::string selection;
    getline(std::cin, selection);
    int index = atoi(selection.c_str());

    if (index < 0 || (size_t)index > values.size())
        throw admin_cluster_exc_t("invalid selection");
    else if (index == 0) {
        throw admin_cluster_exc_t("cancelled");
    } else if (index != 0) {
        std::string full_post_path = post_path + "/" + field_name + "/resolve";
        post_metadata(full_post_path, values[index - 1]);
    }
}

void admin_cluster_link_t::resolve_machine_value(machine_semilattice_metadata_t& machine,
                                                 const std::string& field,
                                                 const std::string& post_path) {
    if (field == "name")
        resolve_value(machine.name, "name", post_path);
    else if (field == "datacenter")
        resolve_value(machine.datacenter, "datacenter_uuid", post_path);
    else
        throw admin_cluster_exc_t("unknown machine field: " + field);
}

void admin_cluster_link_t::resolve_datacenter_value(datacenter_semilattice_metadata_t& dc,
                                                    const std::string& field,
                                                    const std::string& post_path) {
    if (field == "name")
        resolve_value(dc.name, "name", post_path);
    else
        throw admin_cluster_exc_t("unknown datacenter field: " + field);
}

template <class protocol_t>
void admin_cluster_link_t::resolve_namespace_value(namespace_semilattice_metadata_t<protocol_t>& ns,
                                                   const std::string& field,
                                                   const std::string& post_path) {
    if (field == "name")
        resolve_value(ns.name, "name", post_path);
    else if (field == "datacenter")
        resolve_value(ns.primary_datacenter, "primary_uuid", post_path);
    else if (field == "replicas")
        resolve_value(ns.replica_affinities, "replica_affinities", post_path);
    else if (field == "acks")
        resolve_value(ns.ack_expectations, "ack_expectations", post_path);
    else if (field == "shards")
        resolve_value(ns.shards, "shards", post_path);
    else if (field == "port")
        resolve_value(ns.port, "port", post_path);
    else if (field == "primary_pinnings")
        resolve_value(ns.primary_pinnings, "primary_pinnings", post_path);
    else if (field == "secondary_pinnings")
        resolve_value(ns.secondary_pinnings, "secondary_pinnings", post_path);
    else
        throw admin_cluster_exc_t("unknown namespace field: " + field);
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
        if (!i->second.is_deleted() &&
            !i->second.get().datacenter.in_conflict() &&
            i->second.get().datacenter.get() == datacenter)
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

std::string admin_cluster_link_t::path_to_str(const std::vector<std::string>& path) {
    if (path.size() == 0)
        return std::string();

    std::string result(path[0]);
    for (size_t i = 1; i < path.size(); ++i)
        result += "/" + path[i];

    return result;
}

size_t admin_cluster_link_t::handle_post_result(char *ptr, size_t size, size_t nmemb, void *param) {
    std::string *result = reinterpret_cast<std::string*>(param);
    result->append(ptr, size * nmemb);
    return size * nmemb;
}

template <class T>
void admin_cluster_link_t::post_metadata(std::string path, T& metadata) {
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
    post_internal(path, cJSON_print_std_string(render_as_json(&metadata, json_ctx)));
}

void admin_cluster_link_t::delete_metadata(const std::string& path) {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    post_internal(path, "{ }");
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, NULL);
}

std::string admin_cluster_link_t::create_metadata(const std::string& path) {
    post_internal(path + "/new", "{ }");

    scoped_cJSON_t result(cJSON_Parse(post_result.c_str()));

    if (result.get() == NULL)
        throw admin_cluster_exc_t("unexpected error, failed to parse result");

    if (cJSON_GetArraySize(result.get()) != 1)
        throw admin_cluster_exc_t("unexpected error, failed to parse result");

    cJSON* new_item = cJSON_GetArrayItem(result.get(), 0);
    return std::string(new_item->string);
}

void admin_cluster_link_t::post_internal(std::string path, std::string data) {
    post_result.clear();
    curl_easy_setopt(curl_handle, CURLOPT_URL, (sync_peer + path).c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.length());
    CURLcode ret = curl_easy_perform(curl_handle);

    if (ret != 0)
        throw admin_no_connection_exc_t("error when posting data to sync peer");
}

