#define __STDC_FORMAT_MACROS
#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include <map>
#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "arch/io/network.hpp"
#include "clustering/administration/cli/key_parsing.hpp"
#include "clustering/administration/suggester.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view.hpp"
#include "memcached/protocol_json_adapter.hpp"
#include "perfmon/perfmon.hpp"
#include "perfmon/archive.hpp"

const std::vector<std::string>& guarantee_param_vec(const std::map<std::string, std::vector<std::string> >& params, const std::string& name) {
    std::map<std::string, std::vector<std::string> >::const_iterator it = params.find(name);
    guarantee(it != params.end());
    return it->second;
}

const std::string& guarantee_param_0(const std::map<std::string, std::vector<std::string> >& params, const std::string& name) {
    const std::vector<std::string> &vec = guarantee_param_vec(params, name);
    guarantee(vec.size() == 1);
    return vec[0];
}

std::string admin_cluster_link_t::peer_id_to_machine_name(const std::string& peer_id) {
    std::string result(peer_id);

    if (is_uuid(peer_id)) {
        peer_id_t peer(str_to_uuid(peer_id));

        if (peer == connectivity_cluster.get_me()) {
            result.assign("admin_cli");
        } else {
            std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();
            std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.find(peer);

            if (i != directory.end()) {
                try {
                    result = get_info_from_id(uuid_to_str(i->second.machine_id))->name;
                } catch (...) {
                }
            }
        }
    }

    return result;
}

void admin_cluster_link_t::admin_stats_to_table(const std::string& machine,
                                                const std::string& prefix,
                                                const perfmon_result_t& stats,
                                                std::vector<std::vector<std::string> > *table) {

    if (stats.is_string()) {
        // TODO: this should only happen in the case of an empty stats, which shouldn't really happen
        std::vector<std::string> delta;
        delta.push_back(machine);
        delta.push_back("-");
        delta.push_back("-");
        delta.push_back("-");
        table->push_back(delta);
    } else if (stats.is_map()) {
        for (perfmon_result_t::const_iterator i = stats.begin(); i != stats.end(); ++i) {
            if (i->second->is_string()) {
                std::vector<std::string> delta;
                delta.push_back(machine);
                delta.push_back(prefix);
                delta.push_back(i->first);
                delta.push_back(*i->second->get_string());
                table->push_back(delta);
            } else {
                std::string postfix(i->first);
                // Try to convert any uuids to a name, if that fails, ignore
                try {
                    uuid_t temp = str_to_uuid(postfix);
                    postfix = get_info_from_id(uuid_to_str(temp))->name;
                } catch (...) {
                    postfix = peer_id_to_machine_name(postfix);
                }
                admin_stats_to_table(machine, prefix.empty() ? postfix : (prefix + "/" + postfix), *i->second, table);
            }
        }
    }
}

std::string admin_value_to_string(const hash_region_t<key_range_t>& region) {
    return strprintf("%s", key_range_to_cli_str(region.inner).c_str());
}

std::string admin_value_to_string(const mock::dummy_protocol_t::region_t& region) {
    return mock::region_to_debug_str(region);
}

std::string admin_value_to_string(int value) {
    return strprintf("%i", value);
}

std::string admin_value_to_string(const uuid_t& uuid) {
    return uuid_to_str(uuid);
}

std::string admin_value_to_string(const std::string& str) {
    return "\"" + str + "\"";
}

std::string admin_value_to_string(const name_string_t& name) {
    return admin_value_to_string(name.str());
}

std::string admin_value_to_string(const std::map<uuid_t, int>& value) {
    std::string result;
    size_t count = 0;
    for (std::map<uuid_t, int>::const_iterator i = value.begin(); i != value.end(); ++i) {
        ++count;
        result += strprintf("%s: %i%s", uuid_to_str(i->first).c_str(), i->second, count == value.size() ? "" : ", ");
    }
    return result;
}

template <class protocol_t>
std::string admin_value_to_string(const nonoverlapping_regions_t<protocol_t>& value) {
    std::string result;
    bool first = true;
    for (typename nonoverlapping_regions_t<protocol_t>::iterator it = value.begin(); it != value.end(); ++it) {
        result += strprintf("%s%s", first ? "" : ", ", admin_value_to_string(*it).c_str());
        first = false;
    }
    return result;
}

template <class protocol_t>
std::string admin_value_to_string(const region_map_t<protocol_t, uuid_t>& value) {
    std::string result;
    bool first = true;
    for (typename region_map_t<protocol_t, uuid_t>::const_iterator i = value.begin(); i != value.end(); ++i) {
        result += strprintf("%s%s: %s", first ? "" : ", ", admin_value_to_string(i->first).c_str(), uuid_to_str(i->second).c_str());
        first = false;
    }
    return result;
}

template <class protocol_t>
std::string admin_value_to_string(const region_map_t<protocol_t, std::set<uuid_t> >& value) {
    std::string result;
    bool first = true;
    for (typename region_map_t<protocol_t, std::set<uuid_t> >::const_iterator i = value.begin(); i != value.end(); ++i) {
        //TODO: print more detail
        result += strprintf("%s%s: %zu machine%s", first ? "" : ", ", admin_value_to_string(i->first).c_str(), i->second.size(), i->second.size() == 1 ? "" : "s");
        first = false;
    }
    return result;
}

void admin_print_table(const std::vector<std::vector<std::string> >& table) {
    std::vector<int> column_widths;

    if (table.size() == 0) {
        return;
    }

    // Verify that the vectors are consistent size
    for (size_t i = 1; i < table.size(); ++i) {
        if (table[i].size() != table[0].size()) {
            throw admin_cluster_exc_t("unexpected error when printing table");
        }
    }

    // Determine the maximum size of each column
    for (size_t i = 0; i < table[0].size(); ++i) {
        size_t max = table[0][i].length();

        for (size_t j = 1; j < table.size(); ++j) {
            if (table[j][i].length() > max) {
                max = table[j][i].length();
            }
        }

        column_widths.push_back(max);
    }

    // Print out each line, spacing each column
    for (size_t i = 0; i < table.size(); ++i) {
        for (size_t j = 0; j < table[i].size(); ++j) {
            printf("%-*s", column_widths[j] + 2, table[i][j].c_str());
        }
        printf("\n");
    }
}

// Truncate a uuid for easier user-interface
std::string admin_cluster_link_t::truncate_uuid(const uuid_t& uuid) {
    if (uuid.is_nil()) {
        return std::string("none");
    } else {
        return uuid_to_str(uuid).substr(0, uuid_output_length);
    }
}

void admin_cluster_link_t::do_metadata_update(cluster_semilattice_metadata_t *cluster_metadata,
                                              metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t *change_request,
                                              bool prioritize_distribution) {
    std::string error;
    try {
        fill_in_blueprints(cluster_metadata, directory_read_manager->get_root_view()->get(), change_request_id, prioritize_distribution);
    } catch (const missing_machine_exc_t &ex) {
        error = strprintf("Warning: %s", ex.what());
    }

    if (!change_request->update(*cluster_metadata)) {
        throw admin_retry_exc_t();
    }

    // Print this out afterwards to avoid spammy warnings if retries must be performed
    if (!error.empty()) {
        printf("%s\n", error.c_str());
    }
}

admin_cluster_link_t::admin_cluster_link_t(const peer_address_set_t &joins, int client_port, signal_t *interruptor) :
    local_issue_tracker(),
    log_writer(&local_issue_tracker), // TODO: come up with something else for this file
    connectivity_cluster(),
    message_multiplexer(&connectivity_cluster),
    mailbox_manager_client(&message_multiplexer, 'M'),
    mailbox_manager(&mailbox_manager_client),
    stat_manager(&mailbox_manager),
    log_server(&mailbox_manager, &log_writer),
    mailbox_manager_client_run(&mailbox_manager_client, &mailbox_manager),
    semilattice_manager_client(&message_multiplexer, 'S'),
    semilattice_manager_cluster(new semilattice_manager_t<cluster_semilattice_metadata_t>(&semilattice_manager_client, cluster_semilattice_metadata_t())),
    semilattice_manager_client_run(&semilattice_manager_client, semilattice_manager_cluster.get()),
    semilattice_metadata(semilattice_manager_cluster->get_root_view()),
    metadata_change_handler(&mailbox_manager, semilattice_metadata),
    directory_manager_client(&message_multiplexer, 'D'),
    our_directory_metadata(cluster_directory_metadata_t(machine_id_t(connectivity_cluster.get_me().get_uuid()),
                                                        connectivity_cluster.get_me(),
                                                        get_ips(),
                                                        stat_manager.get_address(),
                                                        metadata_change_handler.get_request_mailbox_address(),
                                                        log_server.get_business_card(),
                                                        ADMIN_PEER)),
    directory_read_manager(new directory_read_manager_t<cluster_directory_metadata_t>(connectivity_cluster.get_connectivity_service())),
    directory_write_manager(new directory_write_manager_t<cluster_directory_metadata_t>(&directory_manager_client, our_directory_metadata.get_watchable())),
    directory_manager_client_run(&directory_manager_client, directory_read_manager.get()),
    message_multiplexer_run(&message_multiplexer),
    connectivity_cluster_run(&connectivity_cluster, 0, &message_multiplexer_run, client_port),
    admin_tracker(semilattice_metadata, directory_read_manager->get_root_view()),
    initial_joiner(&connectivity_cluster, &connectivity_cluster_run, joins, 5000)
{
    wait_interruptible(initial_joiner.get_ready_signal(), interruptor);
    if (!initial_joiner.get_success()) {
        throw admin_cluster_exc_t("failed to join cluster");
    }
}

admin_cluster_link_t::~admin_cluster_link_t() {
    clear_metadata_maps();
}

metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t admin_cluster_link_t::choose_sync_peer() {
    const std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();

    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator i = directory.begin(); i != directory.end(); ++i) {
        if (i->second.peer_type == SERVER_PEER) {
            change_request_id = i->second.machine_id;
            sync_peer_id = i->first;
            return i->second.semilattice_change_mailbox;
        }
    }
    throw admin_cluster_exc_t("no reachable server found in the cluster");
}

void admin_cluster_link_t::update_metadata_maps() {
    clear_metadata_maps();

    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    add_subset_to_maps("machines", cluster_metadata.machines.machines);
    add_subset_to_maps("databases", cluster_metadata.databases.databases);
    add_subset_to_maps("datacenters", cluster_metadata.datacenters.datacenters);
    add_subset_to_maps("rdb_namespaces", cluster_metadata.rdb_namespaces->namespaces);
    add_subset_to_maps("dummy_namespaces", cluster_metadata.dummy_namespaces->namespaces);
    add_subset_to_maps("memcached_namespaces", cluster_metadata.memcached_namespaces->namespaces);
}

void admin_cluster_link_t::clear_metadata_maps() {
    // All metadata infos will be in the name_map and uuid_map exactly one each
    for (std::map<std::string, metadata_info_t *>::iterator i = uuid_map.begin(); i != uuid_map.end(); ++i) {
        delete i->second;
    }

    name_map.clear();
    uuid_map.clear();
}

template <class T>
void admin_cluster_link_t::add_subset_to_maps(const std::string& base, const T& data_map) {
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        if (i->second.is_deleted()) {
            continue;
        }

        metadata_info_t * info = new metadata_info_t;
        info->uuid = i->first;
        std::string uuid_str = uuid_to_str(i->first);
        info->path.push_back(base);
        info->path.push_back(uuid_str);

        if (!i->second.get().name.in_conflict()) {
            info->name = i->second.get().name.get().str();
            name_map.insert(std::pair<std::string, metadata_info_t *>(info->name, info));
        }
        uuid_map.insert(std::pair<std::string, metadata_info_t *>(uuid_str, info));
    }
}

void admin_cluster_link_t::sync_from() {
    try {
        cond_t interruptor;
        std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();

        if (sync_peer_id.is_nil() || directory.count(sync_peer_id) == 0) {
            choose_sync_peer();
        }

        semilattice_metadata->sync_from(sync_peer_id, &interruptor);
    } catch (const sync_failed_exc_t& ex) {
        throw admin_no_connection_exc_t("connection lost to cluster");
    } catch (const admin_cluster_exc_t& ex) {
        // No sync peer, continue on with old data
    }

    semilattice_metadata = semilattice_manager_cluster->get_root_view();
    update_metadata_maps();
}

std::vector<std::string> admin_cluster_link_t::get_ids_internal(const std::string& base, const std::string& path) {
    std::vector<std::string> results;

    // TODO: check for uuid collisions, give longer completions
    // Build completion values
    for (std::map<std::string, metadata_info_t *>::iterator i = uuid_map.lower_bound(base);
         i != uuid_map.end() && i->first.find(base) == 0; ++i) {
        if (path.empty() || i->second->path[0] == path) {
            results.push_back(i->first.substr(0, uuid_output_length));
        }
    }

    for (std::map<std::string, metadata_info_t *>::iterator i = name_map.lower_bound(base);
         i != name_map.end() && i->first.find(base) == 0; ++i) {
        if (path.empty() || i->second->path[0] == path) {
            results.push_back(i->first);
        }
    }

    return results;
}

std::vector<std::string> admin_cluster_link_t::get_ids(const std::string& base) {
    return get_ids_internal(base, "");
}

std::vector<std::string> admin_cluster_link_t::get_machine_ids(const std::string& base) {
    return get_ids_internal(base, "machines");
}

std::vector<std::string> admin_cluster_link_t::get_namespace_ids(const std::string& base) {
    std::vector<std::string> namespaces = get_ids_internal(base, "rdb_namespaces");
    std::vector<std::string> delta = get_ids_internal(base, "memcached_namespaces");
    std::copy(delta.begin(), delta.end(), std::back_inserter(namespaces));
    delta = get_ids_internal(base, "dummy_namespaces");
    std::copy(delta.begin(), delta.end(), std::back_inserter(namespaces));
    return namespaces;
}

std::vector<std::string> admin_cluster_link_t::get_datacenter_ids(const std::string& base) {
    return get_ids_internal(base, "datacenters");
}

std::vector<std::string> admin_cluster_link_t::get_database_ids(const std::string& base) {
    return get_ids_internal(base, "databases");
}

std::vector<std::string> admin_cluster_link_t::get_conflicted_ids(const std::string& base) {
    std::set<std::string> unique_set;
    std::vector<std::string> results;

    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > conflicts = admin_tracker.vector_clock_conflict_issue_tracker.get_vector_clock_issues();

    for (std::list<clone_ptr_t<vector_clock_conflict_issue_t> >::iterator i = conflicts.begin(); i != conflicts.end(); ++i) {
        unique_set.insert(uuid_to_str(i->get()->object_id));
    }

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i) {
        if (i->find(base) == 0) {
            results.push_back(i->substr(0, uuid_output_length));
        }
    }

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i) {
        std::map<std::string, metadata_info_t *>::iterator info = uuid_map.find(*i);
        if (info != uuid_map.end() && info->second->name.find(base) == 0) {
            results.push_back(info->second->name);
        }
    }

    return results;
}

admin_cluster_link_t::metadata_info_t *admin_cluster_link_t::get_info_from_id(const std::string& id) {
    // Names must be an exact match, but uuids can be prefix substrings
    if (name_map.count(id) == 0) {
        std::map<std::string, metadata_info_t *>::iterator item = uuid_map.lower_bound(id);

        if (id.length() < minimum_uuid_substring) {
            throw admin_parse_exc_t("identifier not found, too short to specify a uuid: " + id);
        }

        if (item == uuid_map.end() || item->first.find(id) != 0) {
            throw admin_parse_exc_t("identifier not found: " + id);
        }

        // Make sure that the found id is unique
        ++item;
        if (item != uuid_map.end() && item->first.find(id) == 0) {
            throw admin_cluster_exc_t("uuid not unique: " + id);
        }

        return uuid_map.lower_bound(id)->second;
    } else if (name_map.count(id) != 1) {
        std::string exception_info(strprintf("'%s' not unique, possible objects:", id.c_str()));

        for (std::map<std::string, metadata_info_t *>::iterator item = name_map.lower_bound(id);
             item != name_map.end() && item->first == id; ++item) {
            if (item->second->path[0] == "datacenters") {
                exception_info += strprintf("\ndatacenter    %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            } else if (item->second->path[0] == "rdb_namespaces") {
                exception_info += strprintf("\ntable (r) %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            } else if (item->second->path[0] == "dummy_namespaces") {
                exception_info += strprintf("\ntable (d) %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            } else if (item->second->path[0] == "memcached_namespaces") {
                exception_info += strprintf("\nntable (m) %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            } else if (item->second->path[0] == "machines") {
                exception_info += strprintf("\nmachine       %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            } else {
                exception_info += strprintf("\nunknown       %s", uuid_to_str(item->second->uuid).substr(0, uuid_output_length).c_str());
            }
        }
        throw admin_cluster_exc_t(exception_info);
    }

    return name_map.find(id)->second;
}

datacenter_id_t get_machine_datacenter(const std::string& id, const machine_id_t& machine, const cluster_semilattice_metadata_t& cluster_metadata) {
    machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.find(machine);

    if (i == cluster_metadata.machines.machines.end()) {
        throw admin_cluster_exc_t("unexpected error, machine not found: " + uuid_to_str(machine));
    }

    if (i->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error, machine is deleted: " + uuid_to_str(machine));
    }

    if (i->second.get().datacenter.in_conflict()) {
        throw admin_cluster_exc_t("datacenter is in conflict for machine " + id);
    }

    return i->second.get().datacenter.get();
}

void admin_cluster_link_t::do_admin_pin_shard(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string ns = guarantee_param_0(data.params, "table");
    const std::vector<std::string> ns_path(get_info_from_id(ns)->path);
    std::string shard_str = guarantee_param_0(data.params, "key");
    std::string primary;
    std::vector<std::string> secondaries;

    if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("pinning not supported for dummy tables");
    } else if (ns_path[0] != "memcached_namespaces" && ns_path[0] != "rdb_namespaces") {
        throw admin_parse_exc_t("object is not a table: " + ns);
    }

    std::map<std::string, std::vector<std::string> >::const_iterator master_it = data.params.find("master");
    if (master_it != data.params.end()) {
        guarantee(master_it->second.size() == 1);
        primary.assign(master_it->second[0]);  // TODO: How do we know the vector is non-empty?
    }

    std::map<std::string, std::vector<std::string> >::const_iterator replicas_it = data.params.find("replicas");
    if (replicas_it != data.params.end()) {
        secondaries = replicas_it->second;
    }

    // Break up shard string into left and right
    size_t split = shard_str.find("-");
    if (shard_str.find("-inf") == 0) {
        split = shard_str.find("-", 1);
    }

    if (split == std::string::npos) {
        throw admin_parse_exc_t("incorrect shard specifier format");
    }

    shard_input_t shard_in;
    if (split != 0) {
        shard_in.left.exists = true;
        if (!cli_str_to_key(shard_str.substr(0, split), &shard_in.left.key)) {
            throw admin_parse_exc_t("could not parse key: " + shard_str.substr(0, split));
        }
    } else {
        shard_in.left.exists = false;
    }

    shard_in.right.unbounded = false;
    if (split < shard_str.length() - 1) {
        shard_in.right.exists = true;
        if (shard_str.substr(split + 1) == "+inf") {
            shard_in.right.unbounded = true;
        } else if (!cli_str_to_key(shard_str.substr(split + 1), &shard_in.right.key)) {
            throw admin_parse_exc_t("could not parse key: " + shard_str.substr(split + 1));
        }
    } else {
        shard_in.right.exists = false;
    }

    if (ns_path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        namespaces_semilattice_metadata_t<rdb_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.rdb_namespaces->namespaces.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected error, could not find table: " + ns);
        }

        // If no primaries or secondaries are given, we list the current machine assignments
        if (primary.empty() && secondaries.empty()) {
            list_pinnings(i->second.get_ref(), shard_in, cluster_metadata);
        } else {
            do_admin_pin_shard_internal(shard_in, primary, secondaries, cluster_metadata, i->second.get_mutable());
        }
    } else if (ns_path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces->namespaces.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected error, could not find table: " + ns);
        }

        // If no primaries or secondaries are given, we list the current machine assignments
        if (primary.empty() && secondaries.empty()) {
            list_pinnings(i->second.get_ref(), shard_in, cluster_metadata);
        } else {
            do_admin_pin_shard_internal(shard_in, primary, secondaries, cluster_metadata, i->second.get_mutable());
        }
    } else {
        throw admin_cluster_exc_t("unexpected error, unrecognized table protocol");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

template <class protocol_t>
typename protocol_t::region_t admin_cluster_link_t::find_shard_in_namespace(const namespace_semilattice_metadata_t<protocol_t>& ns,
                                                                            const shard_input_t& shard_in) {
    const nonoverlapping_regions_t<protocol_t> shards_value = ns.shards.get();
    for (typename std::set<typename protocol_t::region_t>::const_iterator s = shards_value.begin(); s != shards_value.end(); ++s) {
        // TODO: This is a low level assertion.
        guarantee(s->beg == 0 && s->end == HASH_REGION_HASH_SIZE);

        if (shard_in.left.exists && s->inner.left != shard_in.left.key) {
            // do nothing
        } else if (shard_in.right.exists && (shard_in.right.unbounded != s->inner.right.unbounded)) {
            // do nothing
        } else if (shard_in.right.exists && !shard_in.right.unbounded && (shard_in.right.key != s->inner.right.key)) {
            // do nothing
        } else {
            return *s;
        }
    }

    throw admin_cluster_exc_t("could not find specified shard");
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_pin_shard_internal(const shard_input_t& shard_in,
                                                       const std::string& primary_str,
                                                       const std::vector<std::string>& secondary_strs,
                                                       const cluster_semilattice_metadata_t& cluster_metadata,
                                                       namespace_semilattice_metadata_t<protocol_t> *ns) {
    machine_id_t primary(nil_uuid());
    std::multimap<datacenter_id_t, machine_id_t> datacenter_use;
    std::multimap<datacenter_id_t, machine_id_t> old_datacenter_use;
    std::set<machine_id_t> secondaries;
    bool set_primary(!primary_str.empty());
    bool set_secondary(!secondary_strs.empty());

    // Check that none of the required fields are in conflict
    if (ns->shards.in_conflict()) {
        throw admin_cluster_exc_t("table shards are in conflict, run 'help resolve' for more information");
    } else if (ns->primary_pinnings.in_conflict()) {
        throw admin_cluster_exc_t("table primary pinnings are in conflict, run 'help resolve' for more information");
    } else if (ns->secondary_pinnings.in_conflict()) {
        throw admin_cluster_exc_t("table secondary pinnings are in conflict, run 'help resolve' for more information");
    } else if (ns->replica_affinities.in_conflict()) {
        throw admin_cluster_exc_t("table replica affinities are in conflict, run 'help resolve' for more information");
    } else if (ns->primary_datacenter.in_conflict()) {
        throw admin_cluster_exc_t("table primary datacenter is in conflict, run 'help resolve' for more information");
    }

    // Verify that the selected shard exists, and convert it into a region_t
    typename protocol_t::region_t shard = find_shard_in_namespace(*ns, shard_in);

    // TODO: low-level hash_region_t shard assertion
    guarantee(shard.beg == 0 && shard.end == HASH_REGION_HASH_SIZE);

    // Verify primary is a valid machine and matches the primary datacenter
    if (set_primary) {
        std::vector<std::string> primary_path(get_info_from_id(primary_str)->path);
        primary = str_to_uuid(primary_path[1]);
        if (primary_path[0] != "machines") {
            throw admin_parse_exc_t("object is not a machine: " + primary_str);
        } else if (get_machine_datacenter(primary_str, str_to_uuid(primary_path[1]), cluster_metadata) != ns->primary_datacenter.get()) {
            throw admin_parse_exc_t("machine " + primary_str + " does not belong to the primary datacenter");
        }
    }

    // Verify secondaries are valid machines and store by datacenter for later
    for (size_t i = 0; i < secondary_strs.size(); ++i) {
        std::vector<std::string> secondary_path(get_info_from_id(secondary_strs[i])->path);
        machine_id_t machine = str_to_uuid(secondary_path[1]);

        if (set_primary && primary == machine) {
            throw admin_parse_exc_t("the same machine was specified as both a master and a replica: " + secondary_strs[i]);
        } else if (secondary_path[0] != "machines") {
            throw admin_parse_exc_t("object is not a machine: " + secondary_strs[i]);
        }

        datacenter_id_t datacenter = get_machine_datacenter(secondary_strs[i], machine, cluster_metadata);
        if (ns->replica_affinities.get().count(datacenter) == 0) {
            throw admin_parse_exc_t("machine " + secondary_strs[i] + " belongs to a datacenter with no affinity to the specified table");
        }

        datacenter_use.insert(std::make_pair(datacenter, machine));
    }

    typename region_map_t<protocol_t, std::set<machine_id_t> >::const_iterator secondaries_shard;

    const region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings = ns->secondary_pinnings.get();
    // Find the secondary pinnings and build the old datacenter pinning map if it exists
    for (secondaries_shard = secondary_pinnings.begin();
         secondaries_shard != secondary_pinnings.end(); ++secondaries_shard) {
        // TODO: low level hash_region_t assertion
        guarantee(secondaries_shard->first.beg == 0 && secondaries_shard->first.end == HASH_REGION_HASH_SIZE);

        if (secondaries_shard->first.inner.contains_key(shard.inner.left)) {
            break;
        }
    }

    if (secondaries_shard != secondary_pinnings.end()) {
        for (std::set<machine_id_t>::iterator i = secondaries_shard->second.begin(); i != secondaries_shard->second.end(); ++i) {
            old_datacenter_use.insert(std::make_pair(get_machine_datacenter(uuid_to_str(*i), *i, cluster_metadata), *i));
        }
    }

    // Build the full set of secondaries, carry over any datacenters that were ignored in the command
    std::map<datacenter_id_t, int> affinities = ns->replica_affinities.get();
    for (std::map<datacenter_id_t, int>::iterator i = affinities.begin(); i != affinities.end(); ++i) {
        if (datacenter_use.count(i->first) == 0) {
            // No machines specified for this datacenter, copy over any from the old stuff
            for (std::multimap<datacenter_id_t, machine_id_t>::iterator j = old_datacenter_use.lower_bound(i->first); j != old_datacenter_use.end() && j->first == i->first; ++j) {
                // Filter out the new primary (if it exists)
                if (j->second == primary) {
                    set_secondary = true;
                } else {
                    secondaries.insert(j->second);
                }
            }
        } else if (datacenter_use.count(i->first) <= static_cast<size_t>(i->second)) {
            // Copy over all the specified machines for this datacenter
            for (std::multimap<datacenter_id_t, machine_id_t>::iterator j = datacenter_use.lower_bound(i->first);
                 j != datacenter_use.end() && j->first == i->first; ++j) {
                secondaries.insert(j->second);
            }
        } else {
            throw admin_cluster_exc_t("too many replicas requested from datacenter: " + uuid_to_str(i->first));
        }
    }

    // If we are not setting the primary, but the secondaries contain the existing primary, we have to clear the primary pinning
    if (!set_primary && set_secondary) {
        const region_map_t<protocol_t, machine_id_t> primary_pinnings = ns->primary_pinnings.get();
        for (typename region_map_t<protocol_t, machine_id_t>::const_iterator primary_shard = primary_pinnings.begin();
            primary_shard != primary_pinnings.end();
             ++primary_shard) {

            // TODO: Low level assertion
            guarantee(primary_shard->first.beg == 0 && primary_shard->first.end == HASH_REGION_HASH_SIZE);

            if (primary_shard->first.inner.contains_key(shard.inner.left)) {
                if (secondaries.count(primary_shard->second) != 0)
                    set_primary = true;
                break;
            }
        }
    }

    // Set primary and secondaries - do this before posting any changes in case anything goes wrong
    if (set_primary) {
        insert_pinning(ns->primary_pinnings.get_mutable(), shard.inner, primary);
        ns->primary_pinnings.upgrade_version(change_request_id);
    }
    if (set_secondary) {
        insert_pinning(ns->secondary_pinnings.get_mutable(), shard.inner, secondaries);
        ns->primary_pinnings.upgrade_version(change_request_id);
    }
}

// TODO: WTF are these template parameters.
template <class map_type, class value_type>
void insert_pinning(map_type& region_map, const key_range_t& shard, value_type& value) {
    map_type new_map;
    bool shard_done = false;

    for (typename map_type::iterator i = region_map.begin(); i != region_map.end(); ++i) {
        // TODO: low level hash_region_t assertion.
        guarantee(i->first.beg == 0 && i->first.end == HASH_REGION_HASH_SIZE);
        if (i->first.inner.contains_key(shard.left)) {
            if (i->first.inner.left != shard.left) {
                key_range_t new_shard(key_range_t::closed, i->first.inner.left, key_range_t::open, shard.left);
                new_map.set(hash_region_t<key_range_t>(new_shard), i->second);
            }
            new_map.set(hash_region_t<key_range_t>(shard), value);
            if (i->first.inner.right.key != shard.right.key) {
                // TODO: what if the shard we're looking for staggers the right bound
                key_range_t new_shard;
                new_shard.left = shard.right.key;
                new_shard.right = i->first.inner.right;
                new_map.set(hash_region_t<key_range_t>(new_shard), i->second);
            }
            shard_done = true;
        } else {
            // just copy over the shard
            new_map.set(i->first, i->second);
        }
    }

    if (!shard_done) {
        throw admin_cluster_exc_t("unexpected error, did not find the specified shard");
    }

    region_map = new_map;
}

// TODO: templatize on protocol
void admin_cluster_link_t::do_admin_split_shard(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    const std::vector<std::string> ns_path(get_info_from_id(guarantee_param_0(data.params, "table"))->path);
    const std::vector<std::string> split_points = guarantee_param_vec(data.params, "split-points");
    std::string error;

    if (ns_path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        error = admin_split_shard_internal(change.get(),
                                           str_to_uuid(ns_path[1]),
                                           split_points);
    } else if (ns_path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        error = admin_split_shard_internal(change.get(),
                                           str_to_uuid(ns_path[1]),
                                           split_points);
    } else if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("splitting not supported for dummy tables");
    } else {
        throw admin_cluster_exc_t("invalid object type");
    }

    do_metadata_update(&cluster_metadata, &change_request, true);

    if (!error.empty()) {
        if (split_points.size() > 1) {
            throw admin_cluster_exc_t(error + "not all split points were successfully added");
        } else {
            throw admin_cluster_exc_t(error.substr(0, error.length() - 1));
        }
    }
}

template <class protocol_t>
std::string admin_cluster_link_t::admin_split_shard_internal(namespaces_semilattice_metadata_t<protocol_t> *ns_map,
                                                             const namespace_id_t &ns_id,
                                                             const std::vector<std::string> &split_points) {
    std::string error;
    typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator ns_it = ns_map->namespaces.find(ns_id);

    if (ns_it == ns_map->namespaces.end() || ns_it->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error when looking up table: " + uuid_to_str(ns_id));
    }

    namespace_semilattice_metadata_t<protocol_t> *ns = ns_it->second.get_mutable();

    if (ns->shards.in_conflict()) {
        throw admin_cluster_exc_t("table shards are in conflict, run 'help resolve' for more information");
    }

    error = split_shards(&ns->shards, split_points);

    // Any time shards are changed, we destroy existing pinnings
    // Use 'resolve' because they should be cleared even if in conflict
    // ID sent to the vector clock doesn't matter here since we're setting the metadata through HTTP (TODO: change this if that is no longer true)
    region_map_t<protocol_t, machine_id_t> new_primaries(protocol_t::region_t::universe(), nil_uuid());
    region_map_t<protocol_t, std::set<machine_id_t> > new_secondaries(protocol_t::region_t::universe(), std::set<machine_id_t>());

    ns->primary_pinnings = ns->primary_pinnings.make_resolving_version(new_primaries, change_request_id);
    ns->secondary_pinnings = ns->secondary_pinnings.make_resolving_version(new_secondaries, change_request_id);

    return error;
}

template <class protocol_t>
std::string admin_cluster_link_t::split_shards(vclock_t<nonoverlapping_regions_t<protocol_t> > *shards_vclock,
                                               const std::vector<std::string> &split_points) {
    // TODO: Non-const reference.
    nonoverlapping_regions_t<protocol_t> &shards = shards_vclock->get_mutable();
    std::string error;

    for (size_t i = 0; i < split_points.size(); ++i) {
        try {
            store_key_t key;
            if (!cli_str_to_key(split_points[i], &key)) {
                throw admin_cluster_exc_t("split point could not be parsed: " + split_points[i]);
            }

            guarantee(!shards.empty());

            // TODO: use a better search than linear
            std::set<hash_region_t<key_range_t> >::iterator shard = shards.begin();
            while (true) {
                // TODO: This assertion is too low-level, there should be a function for hash_region_t that computes the expression.
                guarantee(shard->beg == 0 && shard->end == HASH_REGION_HASH_SIZE);

                if (shard == shards.end()) {
                    throw admin_cluster_exc_t("split point could not be placed: " + split_points[i]);
                } else if (shard->inner.contains_key(key)) {
                    break;
                }
                ++shard;
            }

            // Don't split if this key is already the split point
            if (shard->inner.left == key) {
                throw admin_cluster_exc_t("split point already exists: " + split_points[i]);
            }

            // Create the two new shards to be inserted
            key_range_t left;
            left.left = shard->inner.left;
            left.right = key_range_t::right_bound_t(key);
            key_range_t right;
            right.left = key;
            right.right = shard->inner.right;

            shards.remove_region(shard);
            bool add_success = shards.add_region(hash_region_t<key_range_t>(left));
            guarantee(add_success);
            add_success = shards.add_region(hash_region_t<key_range_t>(right));
            guarantee(add_success);

            shards_vclock->upgrade_version(change_request_id);
        } catch (const std::exception& ex) {
            error += ex.what();
            error += "\n";
        }
    }
    return error;
}

void admin_cluster_link_t::do_admin_merge_shard(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    metadata_info_t *info = get_info_from_id(guarantee_param_0(data.params, "table"));
    const std::vector<std::string> split_points(guarantee_param_vec(data.params, "split-points"));
    std::string error;

    if (info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        admin_merge_shard_internal(change.get(), str_to_uuid(info->path[1]), split_points);
    } else if (info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        admin_merge_shard_internal(change.get(), str_to_uuid(info->path[1]), split_points);
    } else if (info->path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("merging not supported for dummy tables");
    } else {
        throw admin_cluster_exc_t("invalid object type");
    }

    do_metadata_update(&cluster_metadata, &change_request, true);

    if (!error.empty()) {
        if (split_points.size() > 1) {
            throw admin_cluster_exc_t(error + "not all split points were successfully removed");
        } else {
            throw admin_cluster_exc_t(error.substr(0, error.length() - 1));
        }
    }
}

template <class protocol_t>
std::string admin_cluster_link_t::admin_merge_shard_internal(namespaces_semilattice_metadata_t<protocol_t> *ns_map,
                                                             const namespace_id_t &ns_id,
                                                             const std::vector<std::string> &split_points) {
    std::string error;
    typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator ns_it =
        ns_map->namespaces.find(ns_id);

    if (ns_it == ns_map->namespaces.end() || ns_it->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error when looking up table: " + uuid_to_str(ns_id));
    }

    namespace_semilattice_metadata_t<protocol_t> *ns = ns_it->second.get_mutable();

    if (ns->shards.in_conflict()) {
        throw admin_cluster_exc_t("table shards are in conflict, run 'help resolve' for more information");
    }

    error = merge_shards(&ns->shards, split_points);

    // Any time shards are changed, we destroy existing pinnings
    // Use 'resolve' because they should be cleared even if in conflict
    // ID sent to the vector clock doesn't matter here since we're setting the metadata through HTTP (TODO: change this if that is no longer true)
    region_map_t<protocol_t, machine_id_t> new_primaries(protocol_t::region_t::universe(), nil_uuid());
    region_map_t<protocol_t, std::set<machine_id_t> > new_secondaries(protocol_t::region_t::universe(), std::set<machine_id_t>());

    ns->primary_pinnings = ns->primary_pinnings.make_resolving_version(new_primaries, change_request_id);
    ns->secondary_pinnings = ns->secondary_pinnings.make_resolving_version(new_secondaries, change_request_id);

    return error;
}

template <class protocol_t>
std::string admin_cluster_link_t::merge_shards(vclock_t<nonoverlapping_regions_t<protocol_t> > *shards_vclock,
                                               const std::vector<std::string> &split_points) {
    nonoverlapping_regions_t<protocol_t> &shards = shards_vclock->get_mutable();
    std::string error;
    for (size_t i = 0; i < split_points.size(); ++i) {
        try {
            store_key_t key;
            if (!cli_str_to_key(split_points[i], &key)) {
                throw admin_cluster_exc_t("split point could not be parsed: " + split_points[i]);
            }

            std::set< hash_region_t<key_range_t> >::iterator shard = shards.begin();
            if (shard == shards.end()) {
                throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);
            }

            std::set< hash_region_t<key_range_t> >::iterator prev = shard;
            // TODO: This assertion's expression is too low-level, there should be a function for it.
            guarantee(shard->beg == 0 && shard->end == HASH_REGION_HASH_SIZE);

            ++shard;
            while (true) {
                // TODO: This assertion's expression is too low-level, there should be a function for it.
                guarantee(shard->beg == 0 && shard->end == HASH_REGION_HASH_SIZE);

                if (shard == shards.end()) {
                    throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);
                } else if (shard->inner.contains_key(key)) {
                    break;
                }
                prev = shard;
                ++shard;
            }

            if (shard->inner.left != store_key_t(key)) {
                throw admin_cluster_exc_t("split point does not exist: " + split_points[i]);
            }

            // Create the new shard to be inserted
            key_range_t merged;
            merged.left = prev->inner.left;
            merged.right = shard->inner.right;

            shards.remove_region(shard);
            shards.remove_region(prev);
            bool add_success = shards.add_region(hash_region_t<key_range_t>(merged));
            guarantee(add_success);

            shards_vclock->upgrade_version(change_request_id);
        } catch (const std::exception& ex) {
            error += ex.what();
            error += "\n";
        }
    }
    return error;
}

void admin_cluster_link_t::do_admin_list(const admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::map<std::string, std::vector<std::string> >::const_iterator obj_it = data.params.find("object");
    std::string obj_str = (obj_it == data.params.end() ? "" : obj_it->second[0]);
    bool long_format = (data.params.count("long") == 1);

    if (obj_str.empty()) {
        list_all(long_format, cluster_metadata);
    } else {
        metadata_info_t *info = get_info_from_id(obj_str);
        uuid_t obj_id = info->uuid;
        if (info->path[0] == "datacenters") {
            datacenters_semilattice_metadata_t::datacenter_map_t::iterator i = cluster_metadata.datacenters.datacenters.find(obj_id);
            if (i == cluster_metadata.datacenters.datacenters.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_datacenter(obj_id, i->second.get_ref(), cluster_metadata);
        } else if (info->path[0] == "databases") {
            databases_semilattice_metadata_t::database_map_t::iterator i = cluster_metadata.databases.databases.find(obj_id);
            if (i == cluster_metadata.databases.databases.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_database(obj_id, i->second.get_ref(), cluster_metadata);
        } else if (info->path[0] == "rdb_namespaces") {
            cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
            namespaces_semilattice_metadata_t<rdb_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_id);
            if (i == change.get()->namespaces.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_namespace(obj_id, i->second.get_ref(), cluster_metadata, "rdb");
        } else if (info->path[0] == "dummy_namespaces") {
            cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
            namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_id);
            if (i == change.get()->namespaces.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_namespace(obj_id, i->second.get_ref(), cluster_metadata, "dummy");
        } else if (info->path[0] == "memcached_namespaces") {
            cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
            namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_id);
            if (i == change.get()->namespaces.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_namespace(obj_id, i->second.get_ref(), cluster_metadata, "memcached");
        } else if (info->path[0] == "machines") {
            machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.find(obj_id);
            if (i == cluster_metadata.machines.machines.end() || i->second.is_deleted()) {
                throw admin_cluster_exc_t("object not found: " + obj_str);
            }
            list_single_machine(obj_id, i->second.get_ref(), cluster_metadata);
        } else {
            throw admin_cluster_exc_t("unexpected error, object found, but type not recognized: " + info->path[0]);
        }
    }
}

template <class protocol_t>
void admin_cluster_link_t::list_pinnings(const namespace_semilattice_metadata_t<protocol_t>& ns, const shard_input_t& shard_in, const cluster_semilattice_metadata_t& cluster_metadata) {
    if (ns.blueprint.in_conflict()) {
        throw admin_cluster_exc_t("table blueprint is in conflict");
    } else if (ns.shards.in_conflict()) {
        throw admin_cluster_exc_t("table shards are in conflict");
    }

    // Search through for the shard
    typename protocol_t::region_t shard = find_shard_in_namespace(ns, shard_in);

    // TODO: this is a low-level assertion.
    guarantee(shard.beg == 0 && shard.end == HASH_REGION_HASH_SIZE);

    list_pinnings_internal(ns.blueprint.get(), shard.inner, cluster_metadata);
}

template <class protocol_t>
void admin_cluster_link_t::list_pinnings_internal(const persistable_blueprint_t<protocol_t>& bp,
                                                  const key_range_t& shard,
                                                  const cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;

    {
        std::vector<std::string> delta;
        delta.push_back("type");
        delta.push_back("machine");
        delta.push_back("name");
        delta.push_back("datacenter");
        delta.push_back("name");
        table.push_back(delta);
    }

    for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator i = bp.machines_roles.begin(); i != bp.machines_roles.end(); ++i) {
        typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator j = i->second.find(hash_region_t<key_range_t>(shard));
        if (j != i->second.end() && j->second != blueprint_role_nothing) {
            std::vector<std::string> delta;

            if (j->second == blueprint_role_primary) {
                delta.push_back("master");
            } else if (j->second == blueprint_role_secondary) {
                delta.push_back("replica");
            } else {
                throw admin_cluster_exc_t("unexpected error, unrecognized role type encountered");
            }

            delta.push_back(truncate_uuid(i->first));

            // Find the machine to get the datacenter and name
            machines_semilattice_metadata_t::machine_map_t::const_iterator m = cluster_metadata.machines.machines.find(i->first);
            if (m == cluster_metadata.machines.machines.end() || m->second.is_deleted()) {
                throw admin_cluster_exc_t("unexpected error, blueprint invalid");
            }

            if (m->second.get().name.in_conflict()) {
                delta.push_back("<conflict>");
            } else {
                delta.push_back(m->second.get().name.get().str());
            }

            if (m->second.get().datacenter.in_conflict()) {
                delta.push_back("<conflict>");
                delta.push_back("");
            } else {
                delta.push_back(truncate_uuid(m->second.get().datacenter.get()));

                // Find the datacenter to get the name
                datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator dc = cluster_metadata.datacenters.datacenters.find(m->second.get().datacenter.get());
                if (dc == cluster_metadata.datacenters.datacenters.end() || dc->second.is_deleted()) {
                    throw admin_cluster_exc_t("unexpected error, blueprint invalid");
                }

                if (dc->second.get().name.in_conflict()) {
                    delta.push_back("<conflict>");
                } else {
                    delta.push_back(dc->second.get().name.get().str());
                }
            }

            table.push_back(delta);
        }
    }

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

struct admin_stats_request_t {
    explicit admin_stats_request_t(mailbox_manager_t *mailbox_manager) :
        response_mailbox(mailbox_manager,
                         boost::bind(&promise_t<perfmon_result_t>::pulse, &stats_promise, _1),
                         mailbox_callback_mode_inline) { }
    promise_t<perfmon_result_t> stats_promise;
    mailbox_t<void(perfmon_result_t)> response_mailbox;
};

void admin_cluster_link_t::do_admin_list_stats(const admin_command_parser_t::command_data& data) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    boost::ptr_map<machine_id_t, admin_stats_request_t> request_map;
    std::set<machine_id_t> machine_filters;
    std::set<namespace_id_t> namespace_filters;
    std::string stat_filter;
    signal_timer_t timer(5000); // 5 second timeout to get all stats

    // Check command params for namespace or machine filter
    std::map<std::string, std::vector<std::string> >::const_iterator id_filter_it = data.params.find("id-filter");
    if (id_filter_it != data.params.end()) {
        for (size_t i = 0; i < id_filter_it->second.size(); ++i) {
            std::string temp = id_filter_it->second[i];
            metadata_info_t *info = get_info_from_id(temp);
            if (info->path[0] == "machines") {
                machine_filters.insert(info->uuid);
            } else if (info->path[0] == "dummy_namespaces" ||
                       info->path[0] == "memcached_namespaces" ||
                       info->path[0] == "rdb_namespaces") {
                namespace_filters.insert(info->uuid);
            } else {
                throw admin_parse_exc_t("object filter is not a machine or table: " + temp);
            }
        }
    }

    // Get the set of machines to request stats from and construct mailboxes for the responses
    if (machine_filters.empty()) {
        for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.begin();
             i != cluster_metadata.machines.machines.end(); ++i) {
            if (!i->second.is_deleted()) {
                machine_id_t target = i->first;
                request_map.insert(target, new admin_stats_request_t(&mailbox_manager));
            }
        }
    } else {
        for (std::set<machine_id_t>::iterator i = machine_filters.begin(); i != machine_filters.end(); ++i) {
            machine_id_t id = *i;
            request_map.insert(id, new admin_stats_request_t(&mailbox_manager));
        }
    }

    if (request_map.empty()) {
        throw admin_cluster_exc_t("no machines to query stats from");
    }

    // Send the requests
    std::set<stat_manager_t::stat_id_t> requested_stats;
    requested_stats.insert(".*");
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
                     requested_stats);
            }
        }

        if (!found) {
            throw admin_cluster_exc_t("Could not locate machine in directory: " + uuid_to_str(i->first));
        }
    }

    std::vector<std::vector<std::string> > stats_table;
    std::vector<std::string> header;

    header.push_back("machine");
    header.push_back("category");
    header.push_back("stat");
    header.push_back("value");
    stats_table.push_back(header);

    // Wait for responses and output them, filtering as necessary
    for (boost::ptr_map<machine_id_t, admin_stats_request_t>::iterator i = request_map.begin(); i != request_map.end(); ++i) {
        const signal_t *stats_ready = i->second->stats_promise.get_ready_signal();
        wait_any_t waiter(&timer, stats_ready);
        waiter.wait();

        if (stats_ready->is_pulsed()) {
            perfmon_result_t stats = i->second->stats_promise.wait();
            std::string machine_name = get_info_from_id(uuid_to_str(i->first))->name;

            // If namespaces were selected, only list stats belonging to those namespaces
            if (!namespace_filters.empty()) {
                for (perfmon_result_t::const_iterator j = stats.begin(); j != stats.end(); ++j) {
                    if (is_uuid(j->first) && namespace_filters.count(str_to_uuid(j->first)) == 1) {
                        // Try to convert the uuid to a (unique) name
                        std::string id = j->first;
                        try {
                            uuid_t temp = str_to_uuid(id);
                            id = get_info_from_id(uuid_to_str(temp))->name;
                        } catch (...) {
                        }
                        admin_stats_to_table(machine_name, id, *j->second, &stats_table);
                    }
                }
            } else {
                admin_stats_to_table(machine_name, std::string(), stats, &stats_table);
            }
        }
    }

    if (stats_table.size() > 1) {
        admin_print_table(stats_table);
    }
}

void admin_cluster_link_t::do_admin_list_directory(const admin_command_parser_t::command_data& data) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    bool long_format = data.params.count("long");
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("type");
    delta.push_back("name");
    delta.push_back("uuid");
    delta.push_back("ips");
    table.push_back(delta);

    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); i++) {
        delta.clear();

        switch (i->second.peer_type) {
          case ADMIN_PEER: delta.push_back("admin"); break;
          case SERVER_PEER: delta.push_back("server"); break;
          case PROXY_PEER: delta.push_back("proxy"); break;
          default: unreachable();
        }

        machines_semilattice_metadata_t::machine_map_t::iterator m = cluster_metadata.machines.machines.find(i->second.machine_id);
        if (m != cluster_metadata.machines.machines.end()) {
            if (m->second.is_deleted()) {
                delta.push_back("<deleted>");
            } else if (m->second.get().name.in_conflict()) {
                delta.push_back("<conflict>");
            } else {
                delta.push_back(m->second.get().name.get().str());
            }
        } else {
            delta.push_back("");
        }

        if (long_format) {
            delta.push_back(uuid_to_str(i->second.machine_id));
        } else {
            delta.push_back(uuid_to_str(i->second.machine_id).substr(0, uuid_output_length));
        }

        std::string ips;
        for (size_t j = 0; j != i->second.ips.size(); ++j) {
            ips += (j == 0 ? "" : " ") + i->second.ips[j];
        }
        delta.push_back(ips);

        table.push_back(delta);
    }

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

void admin_cluster_link_t::do_admin_list_issues(const admin_command_parser_t::command_data& data UNUSED) {
    std::list<clone_ptr_t<global_issue_t> > issues = admin_tracker.issue_aggregator.get_issues();
    for (std::list<clone_ptr_t<global_issue_t> >::iterator i = issues.begin(); i != issues.end(); ++i) {
        puts((*i)->get_description().c_str());
    }
}

template <class map_type>
void admin_cluster_link_t::list_all_internal(const std::string& type, bool long_format, const map_type& obj_map, std::vector<std::vector<std::string> > *table) {
    for (typename map_type::const_iterator i = obj_map.begin(); i != obj_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::vector<std::string> delta;

            delta.push_back(type);

            if (long_format) {
                delta.push_back(uuid_to_str(i->first));
            } else {
                delta.push_back(truncate_uuid(i->first));
            }

            if (i->second.get().name.in_conflict()) {
                delta.push_back("<conflict>");
            } else {
                delta.push_back(i->second.get().name.get().str());
            }

            table->push_back(delta);
        }
    }
}

void admin_cluster_link_t::list_all(bool long_format, const cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("type");
    delta.push_back("uuid");
    delta.push_back("name");
    table.push_back(delta);

    list_all_internal("machine", long_format, cluster_metadata.machines.machines, &table);
    list_all_internal("datacenter", long_format, cluster_metadata.datacenters.datacenters, &table);
    list_all_internal("database", long_format, cluster_metadata.databases.databases, &table);
    // TODO: better differentiation between table types
    list_all_internal("table", long_format, cluster_metadata.rdb_namespaces->namespaces, &table);
    list_all_internal("table (d)", long_format, cluster_metadata.dummy_namespaces->namespaces, &table);
    list_all_internal("table (m)", long_format, cluster_metadata.memcached_namespaces->namespaces, &table);

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

std::map<datacenter_id_t, admin_cluster_link_t::datacenter_info_t> admin_cluster_link_t::build_datacenter_info(const cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<datacenter_id_t, datacenter_info_t> results;
    std::map<machine_id_t, machine_info_t> machine_data = build_machine_info(cluster_metadata);

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
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
    add_datacenter_affinities(cluster_metadata.rdb_namespaces->namespaces, &results);
    add_datacenter_affinities(cluster_metadata.dummy_namespaces->namespaces, &results);
    add_datacenter_affinities(cluster_metadata.memcached_namespaces->namespaces, &results);

    return results;
}

std::map<database_id_t, admin_cluster_link_t::database_info_t> admin_cluster_link_t::build_database_info(const cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<database_id_t, database_info_t> results;
    add_database_tables(cluster_metadata.rdb_namespaces->namespaces, &results);
    add_database_tables(cluster_metadata.dummy_namespaces->namespaces, &results);
    add_database_tables(cluster_metadata.memcached_namespaces->namespaces, &results);
    return results;
}

template <class map_type>
void admin_cluster_link_t::add_database_tables(const map_type& ns_map, std::map<database_id_t, database_info_t> *results) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted() &&
            !i->second.get().database.in_conflict()) {
            ++(*results)[i->second.get().database.get()].tables;
        }
    }
}

template <class map_type>
void admin_cluster_link_t::add_datacenter_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t> *results) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            if (!i->second.get().primary_datacenter.in_conflict()) {
                // TODO: Is this correct?  Are we creating a new value
                // here?  Do maps of ints initialize the value to 0?
                ++(*results)[i->second.get().primary_datacenter.get()].tables;
            }

            if (!i->second.get().replica_affinities.in_conflict()) {
                std::map<datacenter_id_t, int> affinities = i->second.get().replica_affinities.get();
                for (std::map<datacenter_id_t, int>::iterator j = affinities.begin(); j != affinities.end(); ++j) {
                    if (j->second > 0) {
                        ++(*results)[j->first].tables;
                    }
                }
            }
        }
    }
}

void admin_cluster_link_t::do_admin_list_databases(const admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    bool long_format = (data.params.find("long") != data.params.end());

    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;
    std::map<database_id_t, database_info_t> long_info;

    delta.push_back("uuid");
    delta.push_back("name");
    if (long_format) {
        delta.push_back("tables");
    }

    table.push_back(delta);

    if (long_format) {
        long_info = build_database_info(cluster_metadata);
    }

    for (databases_semilattice_metadata_t::database_map_t::const_iterator i = cluster_metadata.databases.databases.begin();
         i != cluster_metadata.databases.databases.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            if (long_format) {
                delta.push_back(uuid_to_str(i->first));
            } else {
                delta.push_back(truncate_uuid(i->first));
            }

            if (i->second.get().name.in_conflict()) {
                delta.push_back("<conflict>");
            } else {
                delta.push_back(i->second.get().name.get().str());
            }

            if (long_format) {
                database_info_t info = long_info[i->first];
                delta.push_back(strprintf("%zu", info.tables));
            }
            table.push_back(delta);
        }
    }

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

void admin_cluster_link_t::do_admin_list_datacenters(const admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    bool long_format = (data.params.find("long") != data.params.end());

    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;
    std::map<datacenter_id_t, datacenter_info_t> long_info;

    delta.push_back("uuid");
    delta.push_back("name");
    if (long_format) {
        delta.push_back("machines");
        delta.push_back("tables");
        delta.push_back("primaries");
        delta.push_back("secondaries");
    }

    table.push_back(delta);

    if (long_format) {
        long_info = build_datacenter_info(cluster_metadata);
    }

    for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator i = cluster_metadata.datacenters.datacenters.begin(); i != cluster_metadata.datacenters.datacenters.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            if (long_format) {
                delta.push_back(uuid_to_str(i->first));
            } else {
                delta.push_back(truncate_uuid(i->first));
            }

            if (i->second.get().name.in_conflict()) {
                delta.push_back("<conflict>");
            } else {
                delta.push_back(i->second.get().name.get().str());
            }

            if (long_format) {
                char buffer[64];
                datacenter_info_t info = long_info[i->first];
                snprintf(buffer, sizeof(buffer), "%zu", info.machines);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%zu", info.tables);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%zu", info.primaries);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%zu", info.secondaries);
                delta.push_back(buffer);
            }
            table.push_back(delta);
        }
    }

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

template <class ns_type>
admin_cluster_link_t::namespace_info_t admin_cluster_link_t::get_namespace_info(const ns_type& ns) {
    namespace_info_t result;

    if (ns.shards.in_conflict()) {
        result.shards = -1;
    } else {
        result.shards = ns.shards.get().size();
    }

    // For replicas, go through the blueprint and sum up all roles
    if (ns.blueprint.in_conflict()) {
        result.replicas = -1;
    } else {
        result.replicas = get_replica_count_from_blueprint(ns.blueprint.get());
    }

    if (ns.primary_datacenter.in_conflict()) {
        result.primary.assign("<conflict>");
    } else {
        result.primary.assign(uuid_to_str(ns.primary_datacenter.get()));
    }

    if (ns.database.in_conflict()) {
        result.database.assign("<conflict>");
    } else {
        result.database.assign(uuid_to_str(ns.database.get()));
    }

    return result;
}

template <class protocol_t>
size_t admin_cluster_link_t::get_replica_count_from_blueprint(const persistable_blueprint_t<protocol_t>& bp) {
    size_t count = 0;

    for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator j = bp.machines_roles.begin();
         j != bp.machines_roles.end(); ++j) {
        for (typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator k = j->second.begin();
             k != j->second.end(); ++k) {
            if (k->second == blueprint_role_primary) {
                ++count;
            } else if (k->second == blueprint_role_secondary) {
                ++count;
            }
        }
    }
    return count;
}

void admin_cluster_link_t::do_admin_list_tables(const admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::map<std::string, std::vector<std::string> >::const_iterator type_it = data.params.find("protocol");
    std::string type = (type_it == data.params.end() ? "" : type_it->second[0]);
    bool long_format = (data.params.count("long") == 1);

    std::vector<std::vector<std::string> > table;
    std::vector<std::string> header;

    header.push_back("uuid");
    header.push_back("name");
    // TODO: fix this once multiple protocols are supported again
    // header.push_back("protocol");
    if (long_format) {
        header.push_back("shards");
        header.push_back("replicas");
        header.push_back("primary");
        header.push_back("database");
    }

    table.push_back(header);

    if (type.empty()) {
        add_namespaces("rdb", long_format, cluster_metadata.rdb_namespaces->namespaces, &table);
        add_namespaces("dummy", long_format, cluster_metadata.dummy_namespaces->namespaces, &table);
        add_namespaces("memcached", long_format, cluster_metadata.memcached_namespaces->namespaces, &table);
    } else if (type == "rdb") {
        add_namespaces(type, long_format, cluster_metadata.rdb_namespaces->namespaces, &table);
#ifndef NO_DUMMY
    } else if (type == "dummy") {
        add_namespaces(type, long_format, cluster_metadata.dummy_namespaces->namespaces, &table);
#endif
    } else if (type == "memcached") {
        add_namespaces(type, long_format, cluster_metadata.memcached_namespaces->namespaces, &table);
    } else {
        throw admin_parse_exc_t("unrecognized protocol: " + type);
    }

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

template <class map_type>
void admin_cluster_link_t::add_namespaces(UNUSED const std::string& protocol,
                                          bool long_format,
                                          const map_type& namespaces,
                                          std::vector<std::vector<std::string> > *table) {
    for (typename map_type::const_iterator i = namespaces.begin(); i != namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::vector<std::string> delta;

            if (long_format) {
                delta.push_back(uuid_to_str(i->first));
            } else {
                delta.push_back(truncate_uuid(i->first));
            }

            if (!i->second.get().name.in_conflict()) {
                delta.push_back(i->second.get().name.get().str());
            } else {
                delta.push_back("<conflict>");
            }

            // TODO: fix this once multiple protocols are supported again
            // delta.push_back(protocol);

            if (long_format) {
                char buffer[64];
                namespace_info_t info = get_namespace_info(i->second.get());

                if (info.shards != -1) {
                    snprintf(buffer, sizeof(buffer), "%i", info.shards);
                    delta.push_back(buffer);
                } else {
                    delta.push_back("<conflict>");
                }

                if (info.replicas != -1) {
                    snprintf(buffer, sizeof(buffer), "%i", info.replicas);
                    delta.push_back(buffer);
                } else {
                    delta.push_back("<conflict>");
                }

                delta.push_back(info.primary);
                delta.push_back(info.database);
            }

            table->push_back(delta);
        }
    }
}

std::map<machine_id_t, admin_cluster_link_t::machine_info_t> admin_cluster_link_t::build_machine_info(const cluster_semilattice_metadata_t& cluster_metadata) {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();
    std::map<machine_id_t, machine_info_t> results;

    // Initialize each machine, reachable machines are in the directory
    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); ++i) {
        results.insert(std::make_pair(i->second.machine_id, machine_info_t()));
        results[i->second.machine_id].status.assign("reach");
    }

    // Unreachable machines will be found in the metadata but not the directory
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            if (results.count(i->first) == 0) {
                results.insert(std::make_pair(i->first, machine_info_t()));
                results[i->first].status.assign("unreach");
            }
        }
    }

    // Go through namespaces
    build_machine_info_internal(cluster_metadata.rdb_namespaces->namespaces, &results);
    build_machine_info_internal(cluster_metadata.dummy_namespaces->namespaces, &results);
    build_machine_info_internal(cluster_metadata.memcached_namespaces->namespaces, &results);

    return results;
}

template <class map_type>
void admin_cluster_link_t::build_machine_info_internal(const map_type& ns_map, std::map<machine_id_t, machine_info_t> *results) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted() && !i->second.get().blueprint.in_conflict()) {
            add_machine_info_from_blueprint(i->second.get().blueprint.get_mutable(), results);
        }
    }
}

template <class protocol_t>
void admin_cluster_link_t::add_machine_info_from_blueprint(const persistable_blueprint_t<protocol_t>& bp, std::map<machine_id_t, machine_info_t> *results) {
    for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator j = bp.machines_roles.begin();
         j != bp.machines_roles.end(); ++j) {
        std::map<machine_id_t, machine_info_t>::iterator it = results->find(j->first);
        if (it != results->end()) {
            bool machine_used = false;

            for (typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator k = j->second.begin();
                 k != j->second.end(); ++k) {
                if (k->second == blueprint_role_primary) {
                    ++it->second.primaries;
                    machine_used = true;
                } else if (k->second == blueprint_role_secondary) {
                    ++it->second.secondaries;
                    machine_used = true;
                }
            }

            if (machine_used) {
                ++it->second.tables;
            }
        }
    }
}

void admin_cluster_link_t::do_admin_list_machines(const admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    bool long_format = (data.params.count("long") == 1);

    std::map<machine_id_t, machine_info_t> long_info;
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> delta;

    delta.push_back("uuid");
    delta.push_back("name");
    delta.push_back("datacenter");
    if (long_format) {
        delta.push_back("status");
        delta.push_back("tables");
        delta.push_back("primaries");
        delta.push_back("secondaries");
    }

    table.push_back(delta);

    if (long_format) {
        long_info = build_machine_info(cluster_metadata);
    }

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            delta.clear();

            if (long_format) {
                delta.push_back(uuid_to_str(i->first));
            } else {
                delta.push_back(truncate_uuid(i->first));
            }

            if (!i->second.get().name.in_conflict()) {
                delta.push_back(i->second.get().name.get().str());
            } else {
                delta.push_back("<conflict>");
            }

            if (!i->second.get().datacenter.in_conflict()) {
                if (i->second.get().datacenter.get().is_nil()) {
                    delta.push_back("none");
                } else if (long_format) {
                    delta.push_back(uuid_to_str(i->second.get().datacenter.get()));
                } else {
                    delta.push_back(truncate_uuid(i->second.get().datacenter.get()));
                }
            } else {
                delta.push_back("<conflict>");
            }

            if (long_format) {
                char buffer[64];
                machine_info_t info = long_info[i->first];
                delta.push_back(info.status);
                snprintf(buffer, sizeof(buffer), "%zu", info.tables);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%zu", info.primaries);
                delta.push_back(buffer);
                snprintf(buffer, sizeof(buffer), "%zu", info.secondaries);
                delta.push_back(buffer);
            }

            table.push_back(delta);
        }
    }

    // TODO: sort by datacenter and name

    if (table.size() > 1) {
        admin_print_table(table);
    }
}

void admin_cluster_link_t::do_admin_create_database(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    database_id_t new_id = generate_uuid();
    database_semilattice_metadata_t *database = cluster_metadata.databases.databases[new_id].get_mutable();

    if (!database->name.get_mutable().assign_value(guarantee_param_0(data.params, "name"))) {
        throw admin_parse_exc_t(strprintf("invalid database name.  (%s)", name_string_t::valid_char_msg));
    }
    database->name.upgrade_version(change_request_id);

    do_metadata_update(&cluster_metadata, &change_request, false);

    printf("uuid: %s\n", uuid_to_str(new_id).c_str());
}

void admin_cluster_link_t::do_admin_create_datacenter(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    datacenter_id_t new_id = generate_uuid();
    datacenter_semilattice_metadata_t *datacenter = cluster_metadata.datacenters.datacenters[new_id].get_mutable();

    if (!datacenter->name.get_mutable().assign_value(guarantee_param_0(data.params, "name"))) {
        throw admin_parse_exc_t(strprintf("invalid datacenter name.  (%s)", name_string_t::valid_char_msg));
    }
    datacenter->name.upgrade_version(change_request_id);

    do_metadata_update(&cluster_metadata, &change_request, false);

    printf("uuid: %s\n", uuid_to_str(new_id).c_str());
}

void admin_cluster_link_t::do_admin_create_table(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string name_str = guarantee_param_0(data.params, "name");
    name_string_t name;
    if (!name.assign_value(name_str)) {
        throw admin_parse_exc_t(strprintf("table name invalid. (%s): %s", name_string_t::valid_char_msg, name_str.c_str()));
    }

    std::string database_id = guarantee_param_0(data.params, "database");
    metadata_info_t *database_info = get_info_from_id(database_id);
    database_id_t database = str_to_uuid(database_info->path[1]);
    datacenter_id_t primary = nil_uuid();
    namespace_id_t new_id;
    std::string protocol;
    std::string primary_key;
    uint64_t port;

    // If primary is specified, use it and verify its validity
    if (data.params.find("primary") != data.params.end()) {
        std::string datacenter_id = guarantee_param_0(data.params, "primary");
        metadata_info_t *datacenter_info = get_info_from_id(datacenter_id);

        if (datacenter_info->path[0] != "datacenters") {
            throw admin_parse_exc_t("specified primary is not a datacenter: " + datacenter_id);
        }

        // Verify that the datacenter has at least one machine in it
        if (get_machine_count_in_datacenter(cluster_metadata, datacenter_info->uuid) < 1) {
            throw admin_cluster_exc_t("primary datacenter must have at least one machine, run 'help set datacenter' for more information");
        }

        primary = str_to_uuid(datacenter_info->path[1]);
    }

    // TODO: fix this once multiple protocols are supported again
    if (data.params.find("protocol") != data.params.end()) {
        protocol = guarantee_param_0(data.params, "protocol");
    } else {
        protocol = "rdb";
    }

    // Get the primary key
    if (data.params.find("primary-key") != data.params.end()) {
        if (protocol != "rdb") {
            throw admin_parse_exc_t("primary-key is only valid for the rdb protocol");
        }
        primary_key = guarantee_param_0(data.params, "primary-key");
    } else if (protocol == "rdb") {
        // TODO: get this value from somewhere, rather than hard-code it
        primary_key = "id";
    }

    // Make sure port is valid if required, or not specified if not needed
    if (protocol == "memcached") {
        if (data.params.find("port") == data.params.end()) {
            throw admin_parse_exc_t("port is required for the memcached protocol");
        }
        std::string port_str = guarantee_param_0(data.params, "port");
        if (!strtou64_strict(port_str, 10, &port)) {
            throw admin_parse_exc_t("port is not a number");
        }
        if (port > 65536) {
            throw admin_parse_exc_t("port is too large: " + port_str);
        }
    } else {
        if (data.params.find("port") != data.params.end()) {
            throw admin_parse_exc_t("port is only vald for the memcached protocol");
        }
        port = 0;
    }

    if (database_info->path[0] != "databases") {
        throw admin_parse_exc_t("specified database is not a database: " + database_id);
    }

    if (protocol == "rdb") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        new_id = do_admin_create_table_internal(name, port, primary, primary_key, database, change.get());
    } else if (protocol == "memcached") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        new_id = do_admin_create_table_internal(name, port, primary, primary_key, database, change.get());
#ifndef NO_DUMMY
    } else if (protocol == "dummy") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        new_id = do_admin_create_table_internal(name, port, primary, primary_key, database, change.get());
#endif
    } else {
        throw admin_parse_exc_t("unrecognized protocol: " + protocol);
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
    printf("uuid: %s\n", uuid_to_str(new_id).c_str());
}

// TODO: This is mostly redundant with the new_namespace function?  Or just outdated?
template <class protocol_t>
namespace_id_t admin_cluster_link_t::do_admin_create_table_internal(const name_string_t& name,
                                                                    int port,
                                                                    const datacenter_id_t& primary,
                                                                    const std::string& primary_key,
                                                                    const database_id_t& database,
                                                                    namespaces_semilattice_metadata_t<protocol_t> *ns) {
    namespace_id_t id = generate_uuid();
    namespace_semilattice_metadata_t<protocol_t> *obj = ns->namespaces[id].get_mutable();

    obj->name.get_mutable() = name;
    obj->name.upgrade_version(change_request_id);

    obj->primary_key.get_mutable() = primary_key;
    obj->primary_key.upgrade_version(change_request_id);

    obj->primary_datacenter.get_mutable() = primary;
    obj->primary_datacenter.upgrade_version(change_request_id);

    obj->port.get_mutable() = port;
    obj->port.upgrade_version(change_request_id);

    nonoverlapping_regions_t<protocol_t> shards;
    bool add_success = shards.add_region(protocol_t::region_t::universe());
    guarantee(add_success);
    obj->shards.get_mutable() = shards;
    obj->shards.upgrade_version(change_request_id);

    obj->ack_expectations.get_mutable()[primary] = 1;
    obj->ack_expectations.upgrade_version(change_request_id);

    /* It's important to initialize this because otherwise it will be
    initialized with a default-constructed UUID, which doesn't initialize its
    contents, so Valgrind will complain. */
    region_map_t<protocol_t, machine_id_t> default_primary_pinnings(protocol_t::region_t::universe(), nil_uuid());
    obj->primary_pinnings.get_mutable() = default_primary_pinnings;
    obj->primary_pinnings.upgrade_version(change_request_id);
    obj->database.get_mutable() = database;
    obj->database.upgrade_version(change_request_id);

    return id;
}

void admin_cluster_link_t::do_admin_set_primary(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "table");
    metadata_info_t *obj_info = get_info_from_id(obj_id);
    std::string datacenter_id = guarantee_param_0(data.params, "datacenter");
    metadata_info_t *datacenter_info = get_info_from_id(datacenter_id);
    datacenter_id_t datacenter_uuid = datacenter_info->uuid;

    // Target must be a datacenter in all existing use cases
    if (datacenter_info->path[0] != "datacenters") {
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);
    }

    // TODO: check this against the number of replicas as well?
    if (get_machine_count_in_datacenter(cluster_metadata, datacenter_uuid) < 1) {
        throw admin_cluster_exc_t("insufficient machines in the selected datacenter to host the table");
    }

    if (obj_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else if (obj_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else if (obj_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else {
        throw admin_cluster_exc_t("target object is not a table");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

void admin_cluster_link_t::do_admin_unset_primary(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "table");
    metadata_info_t *obj_info = get_info_from_id(obj_id);
    datacenter_id_t datacenter_uuid = nil_uuid();

    if (obj_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else if (obj_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else if (obj_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        do_admin_set_datacenter_namespace(obj_info->uuid, datacenter_uuid, &change.get()->namespaces);
    } else {
        throw admin_cluster_exc_t("target object is not a table");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

void admin_cluster_link_t::do_admin_set_datacenter(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "machine");
    metadata_info_t *obj_info = get_info_from_id(obj_id);
    std::string datacenter_id = guarantee_param_0(data.params, "datacenter");
    metadata_info_t *datacenter_info = get_info_from_id(datacenter_id);
    datacenter_id_t datacenter_uuid = datacenter_info->uuid;

    // Target must be a datacenter in all existing use cases
    if (datacenter_info->path[0] != "datacenters") {
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);
    }

    if (obj_info->path[0] == "machines") {
        do_admin_set_datacenter_machine(obj_info->uuid, datacenter_uuid, &cluster_metadata.machines.machines, &cluster_metadata);
    } else {
        throw admin_cluster_exc_t("target object is not a machine");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

void admin_cluster_link_t::do_admin_unset_datacenter(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "machine");
    metadata_info_t *obj_info = get_info_from_id(obj_id);
    datacenter_id_t datacenter_uuid = nil_uuid();

    if (obj_info->path[0] == "machines") {
        do_admin_set_datacenter_machine(obj_info->uuid, datacenter_uuid, &cluster_metadata.machines.machines, &cluster_metadata);
    } else {
        throw admin_cluster_exc_t("target object is not a machine");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

void admin_cluster_link_t::do_admin_set_database(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "table");
    metadata_info_t *obj_info = get_info_from_id(obj_id);
    std::string database_id = guarantee_param_0(data.params, "database");
    metadata_info_t *database_info = get_info_from_id(database_id);
    database_id_t database_uuid = database_info->uuid;

    // Target must be a database in all existing use cases
    if (database_info->path[0] != "databases") {
        throw admin_parse_exc_t("destination is not a database: " + database_id);
    }

    if (obj_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t rdb_change(&cluster_metadata.rdb_namespaces);
        do_admin_set_database_table(obj_info->uuid, database_uuid, &rdb_change.get()->namespaces);
    } else if (obj_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t memcached_change(&cluster_metadata.memcached_namespaces);
        do_admin_set_database_table(obj_info->uuid, database_uuid, &memcached_change.get()->namespaces);
    } else if (obj_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t dummy_change(&cluster_metadata.dummy_namespaces);
        do_admin_set_database_table(obj_info->uuid, database_uuid, &dummy_change.get()->namespaces);
    } else {
        throw admin_cluster_exc_t("target object is not a machine");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

template <class obj_map>
void admin_cluster_link_t::do_admin_set_datacenter_namespace(const uuid_t &obj_uuid,
                                                             const datacenter_id_t &dc,
                                                             obj_map *metadata) {
    typename obj_map::iterator i = metadata->find(obj_uuid);
    if (i == metadata->end() || i->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error when looking up object: " + uuid_to_str(obj_uuid));
    } else if (i->second.get_ref().primary_datacenter.in_conflict()) {
        throw admin_cluster_exc_t("table's primary datacenter is in conflict, run 'help resolve' for more information");
    }

    // TODO: check that changing the primary will not result in invalid number of acks or replicas

    i->second.get_mutable()->primary_datacenter.get_mutable() = dc;
    i->second.get_mutable()->primary_datacenter.upgrade_version(change_request_id);
}

void admin_cluster_link_t::do_admin_set_datacenter_machine(const uuid_t obj_uuid,
                                                           const datacenter_id_t dc,
                                                           machines_semilattice_metadata_t::machine_map_t *metadata,
                                                           cluster_semilattice_metadata_t *cluster_metadata) {
    machines_semilattice_metadata_t::machine_map_t::iterator i = metadata->find(obj_uuid);
    if (i == metadata->end() || i->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error when looking up object: " + uuid_to_str(obj_uuid));
    } else if (i->second.get_ref().datacenter.in_conflict()) {
        throw admin_cluster_exc_t("machine's datacenter is in conflict, run 'help resolve' for more information");
    }

    // TODO: check that changing the datacenter won't violate goals (not enough machines to host n replicas)

    datacenter_id_t old_datacenter(i->second.get_ref().datacenter.get());
    i->second.get_mutable()->datacenter.get_mutable() = dc;
    i->second.get_mutable()->datacenter.upgrade_version(change_request_id);

    // If the datacenter has changed (or we couldn't determine the old datacenter uuid), clear pinnings
    if (old_datacenter != dc) {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t rdb_change(&cluster_metadata->rdb_namespaces);
        remove_machine_pinnings(obj_uuid, &rdb_change.get()->namespaces);
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t memcached_change(&cluster_metadata->memcached_namespaces);
        remove_machine_pinnings(obj_uuid, &memcached_change.get()->namespaces);
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t dummy_change(&cluster_metadata->dummy_namespaces);
        remove_machine_pinnings(obj_uuid, &dummy_change.get()->namespaces);
    }
}

template <class obj_map>
void admin_cluster_link_t::do_admin_set_database_table(const namespace_id_t &table_uuid,
                                                       const database_id_t &db,
                                                       obj_map *metadata) {
    typename obj_map::iterator i = metadata->find(table_uuid);
    if (i == metadata->end() || i->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error when looking up object: " + uuid_to_str(table_uuid));
    } else if (i->second.get_ref().database.in_conflict()) {
        throw admin_cluster_exc_t("table's database is in conflict, run 'help resolve' for more information");
    }

    i->second.get_mutable()->database.get_mutable() = db;
    i->second.get_mutable()->database.upgrade_version(change_request_id);
}

template <class protocol_t>
void admin_cluster_link_t::remove_machine_pinnings(const machine_id_t& machine,
                                                   std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map) {
    // TODO: what if a pinning is in conflict with this machine specified, but is resolved later?
    // perhaps when a resolve is issued, check to make sure it is still valid

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::iterator i = ns_map->begin(); i != ns_map->end(); ++i) {
        if (i->second.is_deleted()) {
            continue;
        }

        namespace_semilattice_metadata_t<protocol_t> *ns = i->second.get_mutable();

        // Check for and remove the machine in primary pinnings
        if (!ns->primary_pinnings.in_conflict()) {
            bool do_upgrade = false;
            for (typename region_map_t<protocol_t, machine_id_t>::iterator j = ns->primary_pinnings.get_mutable().begin();
                 j != ns->primary_pinnings.get_mutable().end(); ++j) {
                if (j->second == machine) {
                    j->second = nil_uuid();
                    do_upgrade = true;
                }
            }

            if (do_upgrade) {
                ns->primary_pinnings.upgrade_version(change_request_id);
            }
        }

        // Check for and remove the machine in secondary pinnings
        if (!ns->secondary_pinnings.in_conflict()) {
            bool do_upgrade = false;
            for (typename region_map_t<protocol_t, std::set<machine_id_t> >::iterator j = ns->secondary_pinnings.get_mutable().begin();
                 j != ns->secondary_pinnings.get_mutable().end(); ++j) {
                if (j->second.erase(machine) == 1) {
                    do_upgrade = true;
                }
            }

            if (do_upgrade) {
                ns->secondary_pinnings.upgrade_version(change_request_id);
            }
        }
    }
}

void admin_cluster_link_t::do_admin_set_name(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();

    metadata_info_t *info = get_info_from_id(guarantee_param_0(data.params, "id"));

    std::string name = guarantee_param_0(data.params, "new-name");

    // TODO: make sure names aren't silly things like uuids or reserved strings

    if (info->path[0] == "machines") {
        do_admin_set_name_internal(info->uuid, name, &cluster_metadata.machines.machines);
    } else if (info->path[0] == "databases") {
        do_admin_set_name_internal(info->uuid, name, &cluster_metadata.databases.databases);
    } else if (info->path[0] == "datacenters") {
        do_admin_set_name_internal(info->uuid, name, &cluster_metadata.datacenters.datacenters);
    } else if (info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        do_admin_set_name_internal(info->uuid, name, &change.get()->namespaces);
    } else if (info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        do_admin_set_name_internal(info->uuid, name, &change.get()->namespaces);
    } else if (info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        do_admin_set_name_internal(info->uuid, name, &change.get()->namespaces);
    } else {
        throw admin_cluster_exc_t("unrecognized object type");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

void do_assign_string_to_name(name_string_t &assignee, const std::string& s) THROWS_ONLY(admin_cluster_exc_t) {
    name_string_t ret;
    if (!ret.assign_value(s)) {
        throw admin_cluster_exc_t("invalid name: " + s);
    }
    assignee = ret;
}


template <class map_type>
void admin_cluster_link_t::do_admin_set_name_internal(const uuid_t& id, const std::string& name, map_type *obj_map) {
    typename map_type::iterator i = obj_map->find(id);
    if (i != obj_map->end() && !i->second.is_deleted() && !i->second.get().name.in_conflict()) {
        do_assign_string_to_name(i->second.get_mutable()->name.get_mutable(), name);
        i->second.get_mutable()->name.upgrade_version(change_request_id);
    } else {
        throw admin_cluster_exc_t("unexpected error, object not found: " + uuid_to_str(id));
    }
}

void admin_cluster_link_t::do_admin_set_acks(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    metadata_info_t *ns_info(get_info_from_id(guarantee_param_0(data.params, "table")));
    datacenter_id_t dc_id = nil_uuid();
    std::string acks_str = guarantee_param_0(data.params, "num-acks").c_str();
    uint64_t acks_num;

    // Make sure num-acks is a number
    if (!strtou64_strict(acks_str, 10, &acks_num)) {
        throw admin_parse_exc_t("num-acks is not a number");
    }

    if (data.params.count("datacenter") != 0) {
        metadata_info_t *dc_info(get_info_from_id(guarantee_param_0(data.params, "datacenter")));
        dc_id = dc_info->uuid;
        if (dc_info->path[0] != "datacenters") {
            throw admin_parse_exc_t(guarantee_param_0(data.params, "datacenter") + " is not a datacenter");
        }
    }

    if (ns_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        namespaces_semilattice_metadata_t<rdb_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(ns_info->uuid);
        if (i == cluster_metadata.rdb_namespaces->namespaces.end()) {
            throw admin_parse_exc_t("unexpected error, table not found");
        } else if (i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected error, table has been deleted");
        }
        do_admin_set_acks_internal(dc_id, acks_num, i->second.get_mutable());

    } else if (ns_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(ns_info->uuid);
        if (i == cluster_metadata.dummy_namespaces->namespaces.end()) {
            throw admin_parse_exc_t("unexpected error, table not found");
        } else if (i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected error, table has been deleted");
        }
        do_admin_set_acks_internal(dc_id, acks_num, i->second.get_mutable());

    } else if (ns_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(ns_info->uuid);
        if (i == cluster_metadata.memcached_namespaces->namespaces.end()) {
            throw admin_parse_exc_t("unexpected error, table not found");
        } else if (i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected error, table has been deleted");
        }
        do_admin_set_acks_internal(dc_id, acks_num, i->second.get_mutable());

    } else {
        throw admin_parse_exc_t(guarantee_param_0(data.params, "table") + " is not a table");
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_set_acks_internal(const datacenter_id_t& datacenter, int num_acks, namespace_semilattice_metadata_t<protocol_t> *ns) {
    if (ns->primary_datacenter.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's primary datacenter is in conflict, run 'help resolve' for more information");
    }

    if (ns->replica_affinities.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's replica affinities are in conflict, run 'help resolve' for more information");
    }

    if (ns->ack_expectations.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's ack expectations are in conflict, run 'help resolve' for more information");
    }

    // Make sure the selected datacenter is assigned to the namespace and that the number of replicas is less than or equal to the number of acks
    const std::map<datacenter_id_t, int> replica_affinities = ns->replica_affinities.get();
    std::map<datacenter_id_t, int>::const_iterator i = replica_affinities.find(datacenter);
    bool is_primary = (datacenter == ns->primary_datacenter.get());
    int replicas = (is_primary ? 1 : 0);
    if (i == replica_affinities.end() || i->second == 0) {
        if (!is_primary) {
            throw admin_cluster_exc_t("the specified datacenter has no replica affinities with the given table");
        }
    }

    if (i != replica_affinities.end()) {
        replicas += i->second;
    }

    if (num_acks > replicas) {
        throw admin_cluster_exc_t("cannot assign more ack expectations than replicas in a datacenter");
    }

    if (num_acks == 0) {
        ns->ack_expectations.get_mutable().erase(datacenter);
    } else {
        ns->ack_expectations.get_mutable()[datacenter] = num_acks;
    }
    ns->ack_expectations.upgrade_version(change_request_id);
}

void admin_cluster_link_t::do_admin_set_replicas(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    metadata_info_t *ns_info = get_info_from_id(guarantee_param_0(data.params, "table"));
    datacenter_id_t dc_id = nil_uuid();
    std::string replicas_str = guarantee_param_0(data.params, "num-replicas").c_str();
    uint64_t num_replicas;

    // Make sure num-replicas is a number
    if (!strtou64_strict(replicas_str, 10, &num_replicas)) {
        throw admin_parse_exc_t("num-replicas is not a number");
    }

    if (data.params.count("datacenter") != 0) {
        metadata_info_t *dc_info(get_info_from_id(guarantee_param_0(data.params, "datacenter")));
        dc_id = dc_info->uuid;
        if (dc_info->path[0] != "datacenters") {
            throw admin_parse_exc_t(guarantee_param_0(data.params, "datacenter") + " is not a datacenter");
        }
    }

    if (get_machine_count_in_datacenter(cluster_metadata, dc_id) < (size_t)num_replicas) {
        throw admin_cluster_exc_t("the number of replicas cannot be more than the number of machines in the datacenter");
    }

    if (ns_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        do_admin_set_replicas_internal(ns_info->uuid, dc_id, num_replicas, change.get()->namespaces);

    } else if (ns_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        do_admin_set_replicas_internal(ns_info->uuid, dc_id, num_replicas, change.get()->namespaces);

    } else if (ns_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        do_admin_set_replicas_internal(ns_info->uuid, dc_id, num_replicas, change.get()->namespaces);

    } else {
        throw admin_parse_exc_t(guarantee_param_0(data.params, "table") + " is not a table");  // TODO(sam): Check if this function body is copy/paste'd.
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

template <class map_type>
void admin_cluster_link_t::do_admin_set_replicas_internal(const namespace_id_t& ns_id,
                                                          const datacenter_id_t& dc_id,
                                                          int num_replicas,
                                                          map_type &ns_map) {
    typename map_type::iterator ns_iter = ns_map.find(ns_id);

    if (ns_iter == ns_map.end()) {
        throw admin_parse_exc_t("unexpected error, table not found");
    } else if (ns_iter->second.is_deleted()) {
        throw admin_cluster_exc_t("unexpected error, table has been deleted");
    }

    typename map_type::mapped_type::value_t *ns = ns_iter->second.get_mutable();

    if (ns->primary_datacenter.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's primary datacenter is in conflict, run 'help resolve' for more information");
    }

    if (ns->replica_affinities.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's replica affinities are in conflict, run 'help resolve' for more information");
    }

    if (ns->ack_expectations.in_conflict()) {
        throw admin_cluster_exc_t("the specified table's ack expectations are in conflict, run 'help resolve' for more information");
    }

    bool is_primary = (dc_id == ns->primary_datacenter.get());
    if (is_primary && num_replicas == 0) {
        throw admin_cluster_exc_t("the number of replicas for the primary datacenter cannot be 0");
    }

    std::map<datacenter_id_t, int>::iterator ack_iter = ns->ack_expectations.get_mutable().find(dc_id);
    if (ack_iter != ns->ack_expectations.get_mutable().end() && ack_iter->second > num_replicas) {
        throw admin_cluster_exc_t("the number of replicas for this datacenter cannot be less than the number of acks, run 'help set acks' for more information");
    }

    ns->replica_affinities.get_mutable()[dc_id] = num_replicas - (is_primary ? 1 : 0);
    if (ns->replica_affinities.get_mutable()[dc_id] == 0) {
        ns->replica_affinities.get_mutable().erase(dc_id);
    }
    ns->replica_affinities.upgrade_version(change_request_id);
}

void admin_cluster_link_t::do_admin_remove_machine(const admin_command_parser_t::command_data& data) {
    std::vector<std::string> ids = guarantee_param_vec(data.params, "id");
    do_admin_remove_internal("machines", ids);
}

void admin_cluster_link_t::do_admin_remove_table(const admin_command_parser_t::command_data& data) {
    std::vector<std::string> ids = guarantee_param_vec(data.params, "id");
    do_admin_remove_internal("namespaces", ids);
}

void admin_cluster_link_t::do_admin_remove_datacenter(const admin_command_parser_t::command_data& data) {
    std::vector<std::string> ids = guarantee_param_vec(data.params, "id");
    do_admin_remove_internal("datacenters", ids);
}

void admin_cluster_link_t::do_admin_remove_database(const admin_command_parser_t::command_data& data) {
    std::vector<std::string> ids = guarantee_param_vec(data.params, "id");
    do_admin_remove_internal("databases", ids);
}

void admin_cluster_link_t::do_admin_remove_internal(const std::string& obj_type, const std::vector<std::string>& ids) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string error;
    bool do_update = false;

    for (size_t i = 0; i < ids.size(); ++i) {
        try {
            metadata_info_t *obj_info = get_info_from_id(ids[i]);

            // TODO: in case of machine, check if it's up and ask for confirmation if it is
            // TODO: in case of a database with tables assigned, ask for confirmation

            // Remove the object from the metadata
            if (obj_info->path[0] == "machines" && obj_type == "machines") {
                do_admin_remove_internal_internal(obj_info->uuid, &cluster_metadata.machines.machines);
            } else if (obj_info->path[0] == "databases" && obj_type == "databases") {
                do_admin_remove_internal_internal(obj_info->uuid, &cluster_metadata.databases.databases);
            } else if (obj_info->path[0] == "datacenters" && obj_type == "datacenters") {
                do_admin_remove_internal_internal(obj_info->uuid, &cluster_metadata.datacenters.datacenters);
            } else if (obj_info->path[0] == "rdb_namespaces" && obj_type == "namespaces") {
                cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
                do_admin_remove_internal_internal(obj_info->uuid, &change.get()->namespaces);
            } else if (obj_info->path[0] == "dummy_namespaces" && obj_type == "namespaces") {
                cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
                do_admin_remove_internal_internal(obj_info->uuid, &change.get()->namespaces);
            } else if (obj_info->path[0] == "memcached_namespaces" && obj_type == "namespaces") {
                cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
                do_admin_remove_internal_internal(obj_info->uuid, &change.get()->namespaces);
            } else {
                throw admin_cluster_exc_t("invalid object type: " + obj_info->path[0]);
            }

            do_update = true;

            // Clean up any hanging references
            if (obj_info->path[0] == "machines") {
                machine_id_t machine(obj_info->uuid);
                cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t memcached_change(&cluster_metadata.memcached_namespaces);
                remove_machine_pinnings(machine, &memcached_change.get()->namespaces);
                cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t dummy_change(&cluster_metadata.dummy_namespaces);
                remove_machine_pinnings(machine, &dummy_change.get()->namespaces);
                cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t rdb_change(&cluster_metadata.rdb_namespaces);
                remove_machine_pinnings(machine, &rdb_change.get()->namespaces);
            } else if (obj_info->path[0] == "datacenters") {
                datacenter_id_t datacenter(obj_info->uuid);
                remove_datacenter_references(datacenter, &cluster_metadata);
            } else if (obj_info->path[0] == "databases") {
                database_id_t database(obj_info->uuid);
                remove_database_tables(database, &cluster_metadata);
            }
        } catch (const std::exception& ex) {
            error += ex.what();
            error += "\n";
        }
    }

    if (do_update) {
        do_metadata_update(&cluster_metadata, &change_request, false);
    }

    if (!error.empty()) {
        if (ids.size() > 1) {
            throw admin_cluster_exc_t(error + "not all removes were successful");
        } else {
            throw admin_cluster_exc_t(error.substr(0, error.length() - 1));
        }
    }
}

template <class T>
void admin_cluster_link_t::do_admin_remove_internal_internal(const uuid_t& key, std::map<uuid_t, T> *obj_map) {
    typename std::map<uuid_t, T>::iterator i = obj_map->find(key);

    if (i == obj_map->end() || i->second.is_deleted()) {
        throw admin_cluster_exc_t("object not found");
    }

    i->second.mark_deleted();
}

void admin_cluster_link_t::remove_datacenter_references(const datacenter_id_t& datacenter, cluster_semilattice_metadata_t *cluster_metadata) {
    datacenter_id_t nil_id(nil_uuid());

    // Go through machines
    for (machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata->machines.machines.begin();
         i != cluster_metadata->machines.machines.end(); ++i) {
        if (i->second.is_deleted()) {
            continue;
        }

        machine_semilattice_metadata_t *machine = i->second.get_mutable();
        if (!machine->datacenter.in_conflict() && machine->datacenter.get() == datacenter) {

            machine->datacenter.get_mutable() = nil_id;
            machine->datacenter.upgrade_version(change_request_id);
        }
    }

    cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t memcached_change(&cluster_metadata->memcached_namespaces);
    remove_datacenter_references_from_namespaces(datacenter, &memcached_change.get()->namespaces);
    cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t dummy_change(&cluster_metadata->dummy_namespaces);
    remove_datacenter_references_from_namespaces(datacenter, &dummy_change.get()->namespaces);
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t rdb_change(&cluster_metadata->rdb_namespaces);
    remove_datacenter_references_from_namespaces(datacenter, &rdb_change.get()->namespaces);
}

template <class protocol_t>
void admin_cluster_link_t::remove_datacenter_references_from_namespaces(const datacenter_id_t& datacenter,
                                                                        std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map) {
    datacenter_id_t nil_id(nil_uuid());

    for (typename std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >::iterator i = ns_map->begin();
         i != ns_map->end(); ++i) {
        if (i->second.is_deleted()) {
            continue;
        }

        namespace_semilattice_metadata_t<protocol_t> *ns = i->second.get_mutable();

        if (!ns->primary_datacenter.in_conflict() && ns->primary_datacenter.get_mutable() == datacenter) {
            ns->primary_datacenter.get_mutable() = nil_id;
            ns->primary_datacenter.upgrade_version(change_request_id);
        }

        if (!ns->replica_affinities.in_conflict()) {
            ns->replica_affinities.get_mutable().erase(datacenter);
            ns->replica_affinities.upgrade_version(change_request_id);
        }

        if (!ns->ack_expectations.in_conflict()) {
            ns->ack_expectations.get_mutable().erase(datacenter);
            ns->ack_expectations.upgrade_version(change_request_id);
        }
    }
}

void admin_cluster_link_t::remove_database_tables(const database_id_t& database, cluster_semilattice_metadata_t *cluster_metadata) {
    cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t memcached_change(&cluster_metadata->memcached_namespaces);
    remove_database_tables_internal(database, &memcached_change.get()->namespaces);
    cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t dummy_change(&cluster_metadata->dummy_namespaces);
    remove_database_tables_internal(database, &dummy_change.get()->namespaces);
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t rdb_change(&cluster_metadata->rdb_namespaces);
    remove_database_tables_internal(database, &rdb_change.get()->namespaces);
}

template <class protocol_t>
void admin_cluster_link_t::remove_database_tables_internal(const database_id_t& database,
                                                           std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map) {
    for (typename std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >::iterator i = ns_map->begin();
         i != ns_map->end(); ++i) {
        if (!i->second.is_deleted()) {
            namespace_semilattice_metadata_t<protocol_t> *ns = i->second.get_mutable();
            if (!ns->database.in_conflict() && ns->database.get() == database) {
                // We delete all namespaces associated with the database
                i->second.mark_deleted();
            }
        }
    }
}

void admin_cluster_link_t::do_admin_touch(UNUSED const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();

    do_metadata_update(&cluster_metadata, &change_request, false);
}

template <class protocol_t>
void admin_cluster_link_t::list_single_namespace(const namespace_id_t& ns_id,
                                                 const namespace_semilattice_metadata_t<protocol_t>& ns,
                                                 const cluster_semilattice_metadata_t& cluster_metadata,
                                                 UNUSED const std::string& protocol) {
    if (ns.name.in_conflict() || ns.name.get().empty()) {
        printf("table %s\n", uuid_to_str(ns_id).c_str());
    } else {
        printf("table '%s' %s\n", ns.name.get().c_str(), uuid_to_str(ns_id).c_str());
    }

    // Print primary datacenter
    if (!ns.primary_datacenter.in_conflict()) {
        datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator dc = cluster_metadata.datacenters.datacenters.find(ns.primary_datacenter.get());
        if (dc == cluster_metadata.datacenters.datacenters.end() ||
            dc->second.is_deleted() ||
            dc->second.get().name.in_conflict() ||
            dc->second.get().name.get().empty()) {
            printf("primary datacenter %s\n", uuid_to_str(ns.primary_datacenter.get()).c_str());
        } else {
            printf("primary datacenter '%s' %s\n", dc->second.get().name.get().c_str(), uuid_to_str(ns.primary_datacenter.get()).c_str());
        }
    } else {
        printf("primary datacenter <conflict>\n");
    }

    // Print database
    if (!ns.database.in_conflict()) {
        databases_semilattice_metadata_t::database_map_t::const_iterator db = cluster_metadata.databases.databases.find(ns.database.get());
        if (db == cluster_metadata.databases.databases.end() ||
            db->second.is_deleted() ||
            db->second.get().name.in_conflict() ||
            db->second.get().name.get().empty()) {
            printf("in database %s\n", uuid_to_str(ns.database.get()).c_str());
        } else {
            printf("in database '%s' %s\n", db->second.get().name.get().c_str(), uuid_to_str(ns.database.get()).c_str());
        }
    } else {
        printf ("in database <conflict>\n");
    }

    // Print primary key
    if (!ns.primary_key.in_conflict()) {
        printf("with primary key '%s'\n", ns.primary_key.get().c_str());
    } else {
        printf ("with primary_key <conflict>\n");
    }

    // TODO: fix this once multiple protocols are supported again
    // if (ns.port.in_conflict()) {
    //     printf("running %s protocol on port <conflict>\n", protocol.c_str());
    // } else {
    //     printf("running %s protocol on port %i\n", protocol.c_str(), ns.port.get());
    // }
    printf("\n");

    {
        std::vector<std::vector<std::string> > table;
        {
            std::vector<std::string> delta;

            // Print configured affinities and ack expectations
            delta.push_back("uuid");
            delta.push_back("name");
            delta.push_back("replicas");
            delta.push_back("acks");
            table.push_back(delta);
        }

        const std::map<datacenter_id_t, int> replica_affinities = ns.replica_affinities.get();
        const std::map<datacenter_id_t, int> ack_expectations = ns.ack_expectations.get();

        for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator i = cluster_metadata.datacenters.datacenters.begin();
             i != cluster_metadata.datacenters.datacenters.end(); ++i) {
            if (!i->second.is_deleted()) {
                std::vector<std::string> delta;

                delta.push_back(uuid_to_str(i->first));
                delta.push_back(i->second.get().name.in_conflict() ? "<conflict>" : i->second.get().name.get().str());

                std::map<datacenter_id_t, int>::const_iterator replica_it = replica_affinities.find(i->first);
                int replicas = 0;
                if (!ns.primary_datacenter.in_conflict() && ns.primary_datacenter.get() == i->first) {
                    replicas = 1 + (replica_it != replica_affinities.end() ? replica_it->second : 0);
                } else if (replica_it != replica_affinities.end()) {
                    replicas = replica_it->second;
                }
                delta.push_back(strprintf("%d", replicas));

                std::map<datacenter_id_t, int>::const_iterator ack_it = ack_expectations.find(i->first);
                int acks = 0;
                if (ack_it != ack_expectations.end()) {
                    acks = ack_it->second;
                }
                delta.push_back(strprintf("%d", acks));

                if (replicas != 0 || acks != 0) {
                    table.push_back(delta);
                }
            }
        }

        // Special case: nil affinity replicas
        bool nil_is_primary = (!ns.primary_datacenter.in_conflict() && ns.primary_datacenter.get() == nil_uuid());
        if (nil_is_primary || replica_affinities.find(nil_uuid()) != replica_affinities.end()) {
            std::vector<std::string> delta;
            delta.push_back(uuid_to_str(nil_uuid()));
            delta.push_back("universe");

            std::map<datacenter_id_t, int>::const_iterator replica_it = replica_affinities.find(nil_uuid());
            int replicas = (nil_is_primary ? 1 : 0);
            if (replica_it != replica_affinities.end()) {
                replicas += replica_it->second;
            }
            delta.push_back(strprintf("%d", replicas));

            std::map<datacenter_id_t, int>::const_iterator ack_it = ack_expectations.find(nil_uuid());
            int acks = 0;
            if (ack_it != ack_expectations.end()) {
                acks = ack_it->second;
            }
            delta.push_back(strprintf("%d", acks));

            table.push_back(delta);
        }

        printf("affinity with %zu datacenter%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
        if (table.size() > 1) {
            admin_print_table(table);
        }
        printf("\n");
    }

    if (ns.shards.in_conflict()) {
        printf("shards in conflict\n");
    } else if (ns.blueprint.in_conflict()) {
        printf("cluster blueprint in conflict\n");
    } else {
        // Print shard hosting
        std::vector<std::vector<std::string> > table;

        {
            std::vector<std::string> delta;
            delta.push_back("shard");
            delta.push_back("machine uuid");
            delta.push_back("name");
            delta.push_back("primary");
            table.push_back(delta);
        }

        add_single_namespace_replicas(ns.shards.get(),
                                      ns.blueprint.get(),
                                      cluster_metadata.machines.machines,
                                      &table);

        printf("%zu replica%s for %zu shard%s\n",
               table.size() - 1, table.size() == 2 ? "" : "s",
               ns.shards.get().size(), ns.shards.get().size() == 1 ? "" : "s");
        if (table.size() > 1) {
            admin_print_table(table);
        }
    }
}

template <class protocol_t>
void admin_cluster_link_t::add_single_namespace_replicas(const nonoverlapping_regions_t<protocol_t>& shards,
                                                         const persistable_blueprint_t<protocol_t>& blueprint,
                                                         const machines_semilattice_metadata_t::machine_map_t& machine_map,
                                                         std::vector<std::vector<std::string> > *table) {
    for (typename std::set<typename protocol_t::region_t>::iterator s = shards.begin(); s != shards.end(); ++s) {
        std::string shard_str = admin_value_to_string(*s);

        // First add the primary host
        for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator i = blueprint.machines_roles.begin();
             i != blueprint.machines_roles.end(); ++i) {
            typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator j = i->second.find(*s);
            if (j != i->second.end() && j->second == blueprint_role_primary) {
                std::vector<std::string> delta;

                delta.push_back(shard_str);
                delta.push_back(uuid_to_str(i->first));

                machines_semilattice_metadata_t::machine_map_t::const_iterator m = machine_map.find(i->first);
                if (m == machine_map.end() || m->second.is_deleted()) {
                    // This shouldn't really happen, but oh well
                    delta.push_back(std::string());
                } else if (m->second.get().name.in_conflict()) {
                    delta.push_back("<conflict>");
                } else {
                    delta.push_back(m->second.get().name.get().str());
                }

                delta.push_back("yes");
                table->push_back(delta);
            }
        }

        // Then add all the secondaries
        for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator i = blueprint.machines_roles.begin();
             i != blueprint.machines_roles.end(); ++i) {
            typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator j = i->second.find(*s);
            if (j != i->second.end() && j->second == blueprint_role_secondary) {
                std::vector<std::string> delta;

                delta.push_back(shard_str);
                delta.push_back(uuid_to_str(i->first));

                machines_semilattice_metadata_t::machine_map_t::const_iterator m = machine_map.find(i->first);
                if (m == machine_map.end() || m->second.is_deleted()) {
                    // This shouldn't really happen, but oh well
                    delta.push_back(std::string());
                } else if (m->second.get().name.in_conflict()) {
                    delta.push_back("<conflict>");
                } else {
                    delta.push_back(m->second.get().name.get().str());
                }

                delta.push_back("no");
                table->push_back(delta);
            }
        }
    }
}

void admin_cluster_link_t::list_single_datacenter(const datacenter_id_t& dc_id,
                                                  const datacenter_semilattice_metadata_t& dc,
                                                  const cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    if (dc.name.in_conflict() || dc.name.get().empty()) {
        printf("datacenter %s\n", uuid_to_str(dc_id).c_str());
    } else {
        printf("datacenter '%s' %s\n", dc.name.get().c_str(), uuid_to_str(dc_id).c_str());
    }
    printf("\n");

    // Get a list of machines in the datacenter
    {
        std::vector<std::string> delta;
        delta.push_back("uuid");
        delta.push_back("name");
        table.push_back(delta);
    }

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted() &&
            !i->second.get().datacenter.in_conflict() &&
            i->second.get().datacenter.get() == dc_id) {
            std::vector<std::string> delta;
            delta.push_back(uuid_to_str(i->first));
            delta.push_back(i->second.get().name.in_conflict() ? "<conflict>" : i->second.get().name.get().str());
            table.push_back(delta);
        }
    }

    printf("%zu machine%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1) {
        admin_print_table(table);
    }
    printf("\n");

    // Get a list of namespaces hosted by the datacenter
    table.clear();
    {
        std::vector<std::string> delta;
        delta.push_back("uuid");
        delta.push_back("name");
        // TODO: fix this once multiple protocols are supported again
        // delta.push_back("protocol");
        delta.push_back("primary");
        delta.push_back("replicas");
        table.push_back(delta);
    }

    add_single_datacenter_affinities(dc_id, cluster_metadata.rdb_namespaces->namespaces, "rdb", &table);
    add_single_datacenter_affinities(dc_id, cluster_metadata.dummy_namespaces->namespaces, "dummy", &table);
    add_single_datacenter_affinities(dc_id, cluster_metadata.memcached_namespaces->namespaces, "memcached", &table);

    printf("%zu table%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1) {
        admin_print_table(table);
    }
}

template <class map_type>
void admin_cluster_link_t::add_single_datacenter_affinities(const datacenter_id_t& dc_id,
                                                            const map_type& ns_map,
                                                            UNUSED const std::string& protocol,
                                                            std::vector<std::vector<std::string> > *table) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            const typename map_type::mapped_type::value_t ns = i->second.get();
            size_t replicas = 0;

            std::vector<std::string> delta;

            delta.push_back(uuid_to_str(i->first));
            delta.push_back(ns.name.in_conflict() ? "<conflict>" : ns.name.get().str());
            // TODO: fix this once multiple protocols are supported again
            // delta.push_back(protocol);

            // TODO: this will only list the replicas required by the user, not the actual replicas (in case of impossible requirements)
            if (!ns.primary_datacenter.in_conflict() &&
                ns.primary_datacenter.get() == dc_id) {
                delta.push_back("yes");
                ++replicas;
            } else {
                delta.push_back("no");
            }

            if (!ns.replica_affinities.in_conflict()) {
                const std::map<datacenter_id_t, int> replica_affinities = ns.replica_affinities.get();

                std::map<datacenter_id_t, int>::const_iterator jt = replica_affinities.find(dc_id);
                if (jt != replica_affinities.end()) {
                    replicas += jt->second;
                }
            }

            delta.push_back(strprintf("%zu", replicas));

            if (replicas > 0) {
                table->push_back(delta);
            }
        }
    }
}

void admin_cluster_link_t::list_single_database(const database_id_t& db_id,
                                                const database_semilattice_metadata_t& db,
                                                const cluster_semilattice_metadata_t& cluster_metadata) {
    std::vector<std::vector<std::string> > table;
    if (db.name.in_conflict() || db.name.get().empty()) {
        printf("database %s\n", uuid_to_str(db_id).c_str());
    } else {
        printf("database '%s' %s\n", db.name.get().c_str(), uuid_to_str(db_id).c_str());
    }
    printf("\n");

    // Get a list of namespaces in the database
    table.clear();
    {
        std::vector<std::string> delta;
        delta.push_back("uuid");
        delta.push_back("name");
        // TODO: fix this once multiple protocols are supported again
        // delta.push_back("protocol");
        delta.push_back("primary");
        table.push_back(delta);
    }

    add_single_database_affinities(db_id, cluster_metadata.rdb_namespaces->namespaces, "rdb", &table);
    add_single_database_affinities(db_id, cluster_metadata.dummy_namespaces->namespaces, "dummy", &table);
    add_single_database_affinities(db_id, cluster_metadata.memcached_namespaces->namespaces, "memcached", &table);

    printf("%zu table%s\n", table.size() - 1, table.size() == 2 ? "" : "s");
    if (table.size() > 1) {
        admin_print_table(table);
    }
}

template <class map_type>
void admin_cluster_link_t::add_single_database_affinities(const datacenter_id_t& db_id,
                                                            const map_type& ns_map,
                                                            UNUSED const std::string& protocol,
                                                            std::vector<std::vector<std::string> > *table) {
    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        if (!i->second.is_deleted()) {
            const typename map_type::mapped_type::value_t ns = i->second.get();

            if (!ns.database.in_conflict() && ns.database.get() == db_id) {
                std::vector<std::string> delta;

                delta.push_back(uuid_to_str(i->first));
                delta.push_back(ns.name.in_conflict() ? "<conflict>" : ns.name.get().str());
                // TODO: fix this once multiple protocols are supported again
                // delta.push_back(protocol);

                if (!ns.primary_datacenter.in_conflict()) {
                    delta.push_back(uuid_to_str(ns.primary_datacenter.get()));
                } else {
                    delta.push_back("<conflict>");
                }

                table->push_back(delta);
            }
        }
    }
}

void admin_cluster_link_t::list_single_machine(const machine_id_t& machine_id,
                                               const machine_semilattice_metadata_t& machine,
                                               const cluster_semilattice_metadata_t& cluster_metadata) {
    if (machine.name.in_conflict() || machine.name.get().empty()) {
        printf("machine %s\n", uuid_to_str(machine_id).c_str());
    } else {
        printf("machine '%s' %s\n", machine.name.get().c_str(), uuid_to_str(machine_id).c_str());
    }

    // Print datacenter
    if (!machine.datacenter.in_conflict()) {
        datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator dc = cluster_metadata.datacenters.datacenters.find(machine.datacenter.get());
        if (dc == cluster_metadata.datacenters.datacenters.end() ||
            dc->second.is_deleted() ||
            dc->second.get().name.in_conflict() ||
            dc->second.get().name.get().empty()) {
            printf("in datacenter %s\n", uuid_to_str(machine.datacenter.get()).c_str());
        } else {
            printf("in datacenter '%s' %s\n", dc->second.get().name.get().c_str(), uuid_to_str(machine.datacenter.get()).c_str());
        }
    } else {
        printf("in datacenter <conflict>\n");
    }
    printf("\n");

    // Print hosted replicas
    std::vector<std::vector<std::string> > table;
    std::vector<std::string> header;

    header.push_back("table");
    header.push_back("name");
    header.push_back("shard");
    header.push_back("primary");
    table.push_back(header);

    size_t namespace_count = 0;
    namespace_count += add_single_machine_replicas(machine_id, cluster_metadata.rdb_namespaces->namespaces, &table);
    namespace_count += add_single_machine_replicas(machine_id, cluster_metadata.dummy_namespaces->namespaces, &table);
    namespace_count += add_single_machine_replicas(machine_id, cluster_metadata.memcached_namespaces->namespaces, &table);

    printf("hosting %zu replica%s from %zu table%s\n", table.size() - 1, table.size() == 2 ? "" : "s", namespace_count, namespace_count == 1 ? "" : "s");
    if (table.size() > 1) {
        admin_print_table(table);
    }
}

template <class map_type>
size_t admin_cluster_link_t::add_single_machine_replicas(const machine_id_t& machine_id,
                                                         const map_type& ns_map,
                                                         std::vector<std::vector<std::string> > *table) {
    size_t matches = 0;

    for (typename map_type::const_iterator i = ns_map.begin(); i != ns_map.end(); ++i) {
        // TODO: if there is a blueprint conflict, the values won't be accurate
        if (!i->second.is_deleted() && !i->second.get().blueprint.in_conflict()) {
            typename map_type::mapped_type::value_t ns = i->second.get();
            std::string uuid = uuid_to_str(i->first);
            std::string name = ns.name.in_conflict() ? "<conflict>" : ns.name.get().str();
            matches += add_single_machine_blueprint(machine_id, ns.blueprint.get(), uuid, name, table);
        }
    }

    return matches;
}

template <class protocol_t>
bool admin_cluster_link_t::add_single_machine_blueprint(const machine_id_t& machine_id,
                                                        const persistable_blueprint_t<protocol_t>& blueprint,
                                                        const std::string& ns_uuid,
                                                        const std::string& ns_name,
                                                        std::vector<std::vector<std::string> > *table) {
    bool match = false;

    typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator machine_entry = blueprint.machines_roles.find(machine_id);
    if (machine_entry == blueprint.machines_roles.end()) {
        return false;
    }

    for (typename persistable_blueprint_t<protocol_t>::region_to_role_map_t::const_iterator i = machine_entry->second.begin();
         i != machine_entry->second.end(); ++i) {
        if (i->second == blueprint_role_primary || i->second == blueprint_role_secondary) {
            std::vector<std::string> delta;

            delta.push_back(ns_uuid);
            delta.push_back(ns_name);

            // Build a string for the shard
            delta.push_back(admin_value_to_string(i->first));

            if (i->second == blueprint_role_primary) {
                delta.push_back("yes");
            } else {
                delta.push_back("no");
            }

            table->push_back(delta);
            match = true;
        } else {
            continue;
        }
    }


    return match;
}

void admin_cluster_link_t::do_admin_resolve(const admin_command_parser_t::command_data& data) {
    metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t
        change_request(&mailbox_manager, choose_sync_peer());
    cluster_semilattice_metadata_t cluster_metadata = change_request.get();
    std::string obj_id = guarantee_param_0(data.params, "id");
    std::string field = guarantee_param_0(data.params, "field");
    metadata_info_t *obj_info = get_info_from_id(obj_id);

    if (obj_info->path[0] == "machines") {
        machines_semilattice_metadata_t::machine_map_t::iterator i = cluster_metadata.machines.machines.find(obj_info->uuid);
        if (i == cluster_metadata.machines.machines.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_machine_value(i->second.get_mutable(), field);
    } else if (obj_info->path[0] == "databases") {
        databases_semilattice_metadata_t::database_map_t::iterator i = cluster_metadata.databases.databases.find(obj_info->uuid);
        if (i == cluster_metadata.databases.databases.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_database_value(i->second.get_mutable(), field);
    } else if (obj_info->path[0] == "datacenters") {
        datacenters_semilattice_metadata_t::datacenter_map_t::iterator i = cluster_metadata.datacenters.datacenters.find(obj_info->uuid);
        if (i == cluster_metadata.datacenters.datacenters.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_datacenter_value(i->second.get_mutable(), field);
    } else if (obj_info->path[0] == "rdb_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata.rdb_namespaces);
        namespaces_semilattice_metadata_t<rdb_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_info->uuid);
        if (i == cluster_metadata.rdb_namespaces->namespaces.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_namespace_value(i->second.get_mutable(), field);
    } else if (obj_info->path[0] == "dummy_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<mock::dummy_protocol_t> >::change_t change(&cluster_metadata.dummy_namespaces);
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_info->uuid);
        if (i == cluster_metadata.dummy_namespaces->namespaces.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_namespace_value(i->second.get_mutable(), field);
    } else if (obj_info->path[0] == "memcached_namespaces") {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata.memcached_namespaces);
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = change.get()->namespaces.find(obj_info->uuid);
        if (i == cluster_metadata.memcached_namespaces->namespaces.end() || i->second.is_deleted()) {
            throw admin_cluster_exc_t("unexpected exception when looking up object: " + obj_id);
        }
        resolve_namespace_value(i->second.get_mutable(), field);
    } else {
        throw admin_cluster_exc_t("unexpected object type encountered: " + obj_info->path[0]);
    }

    do_metadata_update(&cluster_metadata, &change_request, false);
}

// Reads from stream until a newline is occurred.  Reads the newline
// but does not store it in out.  Returns true upon success.
bool getline(FILE *stream, std::string *out) {
    out->clear();

    const int size = 1024;
    char buf[size];

    for (;;) {
        char *res = fgets(buf, size, stream);

        if (!res) {
            return false;
        }

        if (res) {
            int len = strlen(buf);
            guarantee(len < size);
            guarantee(len > 0);

            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
                out->append(buf);
                return true;
            } else {
                out->append(buf);
            }
        }
    }
}

template <class T>
void admin_cluster_link_t::resolve_value(vclock_t<T> *field) {
    if (!field->in_conflict()) {
        throw admin_cluster_exc_t("value is not in conflict");
    }

    std::vector<T> values = field->get_all_values();

    printf("%zu values\n", values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        printf(" %zu: %s\n", i + 1, admin_value_to_string(values[i]).c_str());
    }
    printf(" 0: cancel\n");
    printf("select: ");

    std::string selection;
    bool getline_res = getline(stdin, &selection);
    if (!getline_res) {
        throw admin_cluster_exc_t("could not read from stdin");
    }

    uint64_t index;
    if (!strtou64_strict(selection, 10, &index) || index > values.size()) {
        throw admin_cluster_exc_t("invalid selection");
    } else if (index == 0) {
        throw admin_cluster_exc_t("cancelled");
    } else if (index != 0) {
        *field = field->make_resolving_version(values[index - 1], change_request_id);
    }
}

void admin_cluster_link_t::resolve_machine_value(machine_semilattice_metadata_t *machine,
                                                 const std::string& field) {
    if (field == "name") {
        resolve_value(&machine->name);
    } else if (field == "datacenter") {
        resolve_value(&machine->datacenter);
    } else {
        throw admin_cluster_exc_t("unknown machine field: " + field);
    }
}

void admin_cluster_link_t::resolve_datacenter_value(datacenter_semilattice_metadata_t *dc,
                                                    const std::string& field) {
    if (field == "name") {
        resolve_value(&dc->name);
    } else {
        throw admin_cluster_exc_t("unknown datacenter field: " + field);
    }
}

void admin_cluster_link_t::resolve_database_value(database_semilattice_metadata_t *db,
                                                  const std::string& field) {
    if (field == "name") {
        resolve_value(&db->name);
    } else {
        throw admin_cluster_exc_t("unknown database field: " + field);
    }
}

template <class protocol_t>
void admin_cluster_link_t::resolve_namespace_value(namespace_semilattice_metadata_t<protocol_t> *ns,
                                                   const std::string& field) {
    if (field == "name") {
        resolve_value(&ns->name);
    } else if (field == "database") {
        resolve_value(&ns->database);
    } else if (field == "datacenter") {
        resolve_value(&ns->primary_datacenter);
    } else if (field == "replicas") {
        resolve_value(&ns->replica_affinities);
    } else if (field == "acks") {
        resolve_value(&ns->ack_expectations);
    } else if (field == "shards") {
        resolve_value(&ns->shards);
    } else if (field == "primary_key") {
        resolve_value(&ns->primary_key);
    } else if (field == "port") {
        resolve_value(&ns->port);
    } else if (field == "primary_pinnings") {
        resolve_value(&ns->primary_pinnings);
    } else if (field == "secondary_pinnings") {
        resolve_value(&ns->secondary_pinnings);
    } else {
        throw admin_cluster_exc_t("unknown table field: " + field);
    }
}

size_t admin_cluster_link_t::machine_count() const {
    size_t count = 0;
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            ++count;
        }
    }

    return count;
}

size_t admin_cluster_link_t::get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter) {
    // If this is the nil datacenter, we instead want the count of all machines in the cluster
    if (datacenter.is_nil()) {
        return machine_count();
    }

    size_t count = 0;
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin();
         i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted() &&
            !i->second.get().datacenter.in_conflict() &&
            i->second.get().datacenter.get() == datacenter) {
            ++count;
        }
    }

    return count;
}

size_t admin_cluster_link_t::available_machine_count() {
    std::map<peer_id_t, cluster_directory_metadata_t> directory = directory_read_manager->get_root_view()->get();
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    size_t count = 0;

    for (std::map<peer_id_t, cluster_directory_metadata_t>::iterator i = directory.begin(); i != directory.end(); i++) {
        // Check uuids vs machines in cluster
        machines_semilattice_metadata_t::machine_map_t::const_iterator machine = cluster_metadata.machines.machines.find(i->second.machine_id);
        if (machine != cluster_metadata.machines.machines.end() && !machine->second.is_deleted()) {
            ++count;
        }
    }

    return count;
}

size_t admin_cluster_link_t::issue_count() {
    return admin_tracker.issue_aggregator.get_issues().size();
}

std::string admin_cluster_link_t::path_to_str(const std::vector<std::string>& path) {
    if (path.size() == 0) {
        return std::string();
    }

    std::string result(path[0]);
    for (size_t i = 1; i < path.size(); ++i) {
        result += "/" + path[i];
    }

    return result;
}

