#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <stdarg.h>
#include <stdio.h>

#include <map>
#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/suggester.hpp"
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
    initial_joiner(&connectivity_cluster, &connectivity_cluster_run, joins, 5000)
{
    wait_interruptible(initial_joiner.get_ready_signal(), interruptor);
    if (!initial_joiner.get_success())
            throw admin_cluster_exc_t("failed to join cluster");

    // Pick a peer to do cluster operations through
    // TODO: pick a peer more intelligently (latency? peer specified at command line?)
    std::set<peer_id_t> peers = connectivity_cluster.get_peers_list();
    for (std::set<peer_id_t>::iterator i = peers.begin(); i != peers.end(); ++i)
        if (*i != connectivity_cluster.get_me())
            sync_peer = *i;
    rassert(!sync_peer.is_nil());
}

admin_cluster_link_t::~admin_cluster_link_t() {
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
    for (std::map<std::string, metadata_info*>::iterator i = uuid_map.begin(); i != uuid_map.end(); ++i)
        delete i->second;

    name_map.clear();
    uuid_map.clear();
}

template <class T>
void admin_cluster_link_t::add_subset_to_maps(const std::string& base, T& data_map) {
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        if (i->second.is_deleted())
            continue;

        metadata_info* info = new metadata_info;
        info->uuid = uuid_to_str(i->first);
        info->path.push_back(base);
        info->path.push_back(info->uuid);

        if (!i->second.get().name.in_conflict()) {
            info->name = i->second.get().name.get();
            name_map.insert(std::make_pair<std::string, metadata_info*>(info->name, info));
        }
        uuid_map.insert(std::make_pair<std::string, metadata_info*>(info->uuid, info));
    }
}

void admin_cluster_link_t::sync_from() {
    // cond_t interruptor;
    // semilattice_metadata->sync_from(sync_peer, &interruptor);
    semilattice_metadata = semilattice_manager_cluster.get_root_view();
    update_metadata_maps();
}

void admin_cluster_link_t::sync_to() {
    // cond_t interruptor;
    // semilattice_metadata->sync_to(sync_peer, &interruptor);
}

std::vector<std::string> admin_cluster_link_t::get_ids_internal(const std::string& base, const std::string& path) {
    std::vector<std::string> results;
    sync_from();

    // TODO: check for uuid collisions, give longer completions
    // Build completion values
    for (std::map<std::string, metadata_info*>::iterator i = uuid_map.lower_bound(base); i != uuid_map.end() && i->first.find(base) == 0; ++i)
        if (path.empty() || i->second->path[0] == path)
            results.push_back(i->first.substr(0, uuid_output_length));

    for (std::map<std::string, metadata_info*>::iterator i = name_map.lower_bound(base); i != name_map.end() && i->first.find(base) == 0; ++i)
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
    sync_from();

    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > conflicts = vector_clock_conflict_issue_tracker.get_vector_clock_issues();

    for (std::list<clone_ptr_t<vector_clock_conflict_issue_t> >::iterator i = conflicts.begin(); i != conflicts.end(); ++i)
        unique_set.insert(uuid_to_str(i->get()->object_id));

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i)
        if (i->find(base) == 0)
            results.push_back(i->substr(0, uuid_output_length));

    for (std::set<std::string>::iterator i = unique_set.begin(); i != unique_set.end(); ++i) {
        std::map<std::string, metadata_info*>::iterator info = uuid_map.find(*i);
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

admin_cluster_link_t::metadata_info* admin_cluster_link_t::get_info_from_id(const std::string& id) {
    // Names must be an exact match, but uuids can be prefix substrings
    if (name_map.count(id) == 0) {
        std::map<std::string, metadata_info*>::iterator item = uuid_map.lower_bound(id);

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
        throw admin_cluster_exc_t("name not unique: " + id);
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
    shard_input_t shard_in;
    shard_in.left.key = shard_str.substr(0, split);
    shard_in.left.inf = shard_in.left.key == "%inf";
    shard_in.right.key = shard_str.substr(split + 1);
    shard_in.right.inf = shard_in.right.key == "%inf";

    printf("key: %s\n", shard_str.c_str());

    if (ns_path[0] == "memcached_namespaces") {
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator i = cluster_metadata.memcached_namespaces.namespaces.find(str_to_uuid(ns_path[1]));
        if (i == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_cluster_exc_t("unexpected error, could not find namespace: " + ns);

        // If no primaries or secondaries are given, we list the current machine assignments
        if (primary.empty() && secondaries.empty())
            list_pinnings(i->second.get_mutable(), shard_in);
        else
            do_admin_pin_shard_internal(i->second.get_mutable(), shard_in, primary, secondaries, cluster_metadata);
    } else
        throw admin_cluster_exc_t("unexpected error, unknown namespace protocol");
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_pin_shard_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const shard_input_t& shard_in UNUSED, const std::string& primary_str, const std::vector<std::string>& secondary_strs, cluster_semilattice_metadata_t& cluster_metadata) {
    machine_id_t primary;
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
    if (!primary_str.empty()) {
        insert_pinning(ns.primary_pinnings.get_mutable(), shard, primary);
        ns.primary_pinnings.upgrade_version(sync_peer.get_uuid());
    }

    if (!secondary_strs.empty()) {
        insert_pinning(ns.secondary_pinnings.get_mutable(), shard, secondaries);
        ns.secondary_pinnings.upgrade_version(sync_peer.get_uuid());
    }

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
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

void admin_cluster_link_t::do_admin_split_shard(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> split_points(data.params["split-points"]);
    bool errored = false;

    if (ns_path[0] == "memcached_namespaces") {
        namespace_id_t ns_id(str_to_uuid(ns_path[1]));
        namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::iterator ns_it =
            cluster_metadata.memcached_namespaces.namespaces.find(ns_id);

        if (ns_it == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_cluster_exc_t("unexpected error when looking up namespace: " + ns_path[1]);

        namespace_semilattice_metadata_t<memcached_protocol_t>& ns = ns_it->second.get_mutable();

        if (ns.shards.in_conflict())
            throw admin_cluster_exc_t("namespace shards are in conflict, run 'help resolve' for more information");

        for (size_t i = 0; i < split_points.size(); ++i) {
            store_key_t key(split_points[i]);
            if (ns.shards.get().empty()) {
                // this should never happen, but try to handle it anyway
                key_range_t left(key_range_t::none, store_key_t(), key_range_t::open, store_key_t(key));
                key_range_t right(key_range_t::closed, store_key_t(key), key_range_t::none, store_key_t());
                ns.shards.get_mutable().insert(left);
                ns.shards.get_mutable().insert(right);
                ns.shards.upgrade_version(sync_peer.get_uuid());
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
                    ns.shards.upgrade_version(sync_peer.get_uuid());
                } catch (std::exception& ex) {
                    printf("%s\n", ex.what());
                    errored = true;
                }
            }
        }

        // Any time shards are changed, we destroy existing pinnings
        region_map_t<memcached_protocol_t, machine_id_t> primary;
        primary.set(memcached_protocol_t::region_t::universe(), machine_id_t());
        ns.primary_pinnings = ns.primary_pinnings.make_resolving_version(primary, sync_peer.get_uuid());
        region_map_t<memcached_protocol_t, std::set<machine_id_t> > secondary;
        secondary.set(memcached_protocol_t::region_t::universe(), std::set<machine_id_t>());
        ns.secondary_pinnings = ns.secondary_pinnings.make_resolving_version(secondary, sync_peer.get_uuid());

    } else if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("splitting not supported for dummy namespaces");
    } else
        throw admin_cluster_exc_t("invalid object type");


    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);

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

        if (ns_it == cluster_metadata.memcached_namespaces.namespaces.end())
            throw admin_cluster_exc_t("unexpected error when looking up namespace: " + ns_path[1]);

        namespace_semilattice_metadata_t<memcached_protocol_t>& ns = ns_it->second.get_mutable();

        if (ns.shards.in_conflict())
            throw admin_cluster_exc_t("namespace shards are in conflict, run 'help resolve' for more information");

        for (size_t i = 0; i < split_points.size(); ++i) {
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
                ns.shards.upgrade_version(sync_peer.get_uuid());
            } catch (std::exception& ex) {
                printf("%s\n", ex.what());
                errored = true;
            }
        }

        // Any time shards are changed, we destroy existing pinnings
        region_map_t<memcached_protocol_t, machine_id_t> primary;
        primary.set(memcached_protocol_t::region_t::universe(), machine_id_t());
        ns.primary_pinnings = ns.primary_pinnings.make_resolving_version(primary, sync_peer.get_uuid());
        region_map_t<memcached_protocol_t, std::set<machine_id_t> > secondary;
        secondary.set(memcached_protocol_t::region_t::universe(), std::set<machine_id_t>());
        ns.secondary_pinnings = ns.secondary_pinnings.make_resolving_version(secondary, sync_peer.get_uuid());
    } else if (ns_path[0] == "dummy_namespaces") {
        throw admin_cluster_exc_t("merging not supported for dummy namespaces");
    } else
        throw admin_cluster_exc_t("invalid object type");

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);

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
        list_stats(long_format);
    } else if (id.empty()) {
        list_all(long_format, cluster_metadata);
    } else {
        // TODO: special formatting for each object type, instead of JSON
        namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
        std::vector<std::string> obj_path(get_info_from_id(id)->path);
        puts(cJSON_print_std_string(scoped_cJSON_t(traverse_directory(obj_path, json_ctx, cluster_metadata)->render(json_ctx)).get()).c_str());
    }
}

template <class protocol_t>
void admin_cluster_link_t::list_pinnings(namespace_semilattice_metadata_t<protocol_t>& ns UNUSED, const shard_input_t& shard_in UNUSED) {
    printf("NYI\n");
}

void admin_cluster_link_t::list_stats(bool long_format UNUSED) {
    printf("NYI\n");
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

void admin_print_table(const std::vector<std::vector<std::string> >& table) {
    std::vector<int> column_widths;

    if (table.size() == 0)
        return;

    for (size_t i = 1; i < table.size(); ++i)
        if (table[i].size() != table[0].size())
            throw admin_cluster_exc_t("unexpected error when printing table");

    for (size_t i = 0; i < table[0].size(); ++i) {
        int max = table[0][i].length();

        for (size_t j = 1; j < table.size(); ++j)
            if ((int)table[j][i].length() > max)
                max = table[j][i].length();

        column_widths.push_back(max);
    }

    for (size_t i = 0; i < table.size(); ++i) {
        for (size_t j = 0; j < table[i].size(); ++j)
            printf("%-*s", column_widths[j] + 2, table[i][j].c_str());
        printf("\n");
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
                delta.push_back(uuid_to_str(i->first).substr(0, uuid_output_length));

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

    add_datacenter_info_affinities(cluster_metadata.dummy_namespaces.namespaces, results);
    add_datacenter_info_affinities(cluster_metadata.memcached_namespaces.namespaces, results);

    return results;
}

template <class map_type>
void admin_cluster_link_t::add_datacenter_info_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t>& results) {
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
                delta.push_back(uuid_to_str(i->first).substr(0, uuid_output_length));
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
                delta.push_back(uuid_to_str(i->first).substr(0, uuid_output_length));

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
                delta.push_back(uuid_to_str(i->first).substr(0, uuid_output_length));

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
                    delta.push_back(uuid_to_str(i->second.get().datacenter.get()).substr(0, uuid_output_length));
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
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    datacenter_id_t id = generate_uuid();
    datacenter_semilattice_metadata_t& datacenter = cluster_metadata.datacenters.datacenters[id].get_mutable();

    if (data.params.count("name") == 1) {
        datacenter.name.get_mutable() = data.params["name"][0];
        datacenter.name.upgrade_version(sync_peer.get_uuid());
    }

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}


void admin_cluster_link_t::do_admin_create_namespace(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string protocol(data.params["protocol"][0]);
    std::string port_str(data.params["port"][0]);
    datacenter_id_t primary;
    std::string name;
    int port;

    if (data.params.count("name") == 1)
        name.assign(data.params["name"][0]);

    // Make sure port is a number
    for (size_t i = 0; i < port_str.length(); ++i)
        if (port_str[i] < '0' || port_str[i] > '9')
            throw admin_parse_exc_t("port is not a number");

    port = atoi(port_str.c_str());
    if (port > 65536)
        throw admin_parse_exc_t("port is too large: " + port_str);

    if (data.params.count("primary") == 1) {

        std::vector<std::string> datacenter_path(get_info_from_id(data.params["primary"][0])->path);

        if (datacenter_path[0] != "datacenters")
            throw admin_parse_exc_t("namespace primary is not a datacenter: " + data.params["primary"][0]);

        primary = str_to_uuid(datacenter_path[datacenter_path.size() - 1]);
    }

    if (protocol == "memcached")
        do_admin_create_namespace_internal(cluster_metadata.memcached_namespaces, name, port, primary);
    else if (protocol == "dummy")
        do_admin_create_namespace_internal(cluster_metadata.dummy_namespaces, name, port, primary);
    else
        throw admin_parse_exc_t("unrecognized protocol: " + protocol);

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

template <class protocol_t>
void admin_cluster_link_t::do_admin_create_namespace_internal(namespaces_semilattice_metadata_t<protocol_t>& ns,
                                                              const std::string& name,
                                                              int port,
                                                              const datacenter_id_t& primary) {
    namespace_id_t id = generate_uuid();
    namespace_semilattice_metadata_t<protocol_t>& obj = ns.namespaces[id].get_mutable();

    if (!name.empty()) {
        obj.name.get_mutable() = name;
        obj.name.upgrade_version(sync_peer.get_uuid());
    }

    if (!primary.is_nil()) {
        obj.primary_datacenter.get_mutable() = primary;
        obj.primary_datacenter.upgrade_version(sync_peer.get_uuid());
    }

    obj.port.get_mutable() = port;
    obj.port.upgrade_version(sync_peer.get_uuid());

    std::set<typename protocol_t::region_t> shards;
    shards.insert(protocol_t::region_t::universe());
    obj.shards.get_mutable() = shards;
    obj.shards.upgrade_version(sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_set_datacenter(admin_command_parser_t::command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string obj_id(data.params["id"][0]);
    std::vector<std::string> obj_path(get_info_from_id(obj_id)->path);
    std::string datacenter_id(data.params["datacenter"][0]);
    std::vector<std::string> datacenter_path(get_info_from_id(datacenter_id)->path);
    boost::uuids::uuid datacenter_uuid(str_to_uuid(datacenter_path[1]));
    boost::uuids::uuid obj_uuid(str_to_uuid(obj_path[1]));
    bool resolve(false);

    if (data.params.count("resolve") == 1)
        resolve = true;

    // Target must be a datacenter in all existing use cases
    if (datacenter_path[0] != "datacenters")
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);

    if (obj_path[0] == "memcached_namespaces") {
        do_admin_set_datacenter_namespace(cluster_metadata.memcached_namespaces.namespaces, obj_uuid, datacenter_uuid, resolve);
    } else if (obj_path[0] == "dummy_namespaces") {
        do_admin_set_datacenter_namespace(cluster_metadata.dummy_namespaces.namespaces, obj_uuid, datacenter_uuid, resolve);
    } else if (obj_path[0] == "machines") {
        do_admin_set_datacenter_machine(cluster_metadata.machines.machines, obj_uuid, datacenter_uuid, resolve);
    } else
        throw admin_cluster_exc_t("target objects is not a namespace or machine");

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

template <class obj_map>
void admin_cluster_link_t::do_admin_set_datacenter_namespace(obj_map& metadata,
                                                             const boost::uuids::uuid obj_uuid,
                                                             const datacenter_id_t dc,
                                                             bool resolve) {
    typename obj_map::iterator i = metadata.find(obj_uuid);
    if (i == metadata.end())
        throw admin_cluster_exc_t("unexpected error when looking up object: " + uuid_to_str(obj_uuid));

    if (i->second.get_mutable().primary_datacenter.in_conflict()) {
        if (!resolve)
            throw admin_cluster_exc_t("namespace's primary datacenter is in conflict, run 'help resolve' for more information");
        i->second.get_mutable().primary_datacenter = i->second.get_mutable().primary_datacenter.make_resolving_version(dc, sync_peer.get_uuid());
    } else if (resolve)
        throw admin_cluster_exc_t("namespace's primary datacenter is not in conflict, command ignored");

    i->second.get_mutable().primary_datacenter = i->second.get_mutable().primary_datacenter.make_new_version(dc, sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_set_datacenter_machine(machines_semilattice_metadata_t::machine_map_t& metadata,
                                                           const boost::uuids::uuid obj_uuid,
                                                           const datacenter_id_t dc,
                                                           bool resolve) {
    machines_semilattice_metadata_t::machine_map_t::iterator i = metadata.find(obj_uuid);
    if (i == metadata.end())
        throw admin_cluster_exc_t("unexpected error when looking up object: " + uuid_to_str(obj_uuid));

    if (i->second.get_mutable().datacenter.in_conflict()) {
        if (!resolve)
            throw admin_cluster_exc_t("machine's datacenter is in conflict, run 'help resolve' for more information");
        i->second.get_mutable().datacenter = i->second.get_mutable().datacenter.make_resolving_version(dc, sync_peer.get_uuid());
    } else if (resolve)
        throw admin_cluster_exc_t("machine's datacenter is not in conflict, command ignored");

    i->second.get_mutable().datacenter = i->second.get_mutable().datacenter.make_new_version(dc, sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_set_name(admin_command_parser_t::command_data& data) {
    // TODO: make sure names aren't silly things like uuids or reserved strings
    std::string new_name(data.params["new-name"][0]);
    std::string id(data.params["id"][0]);
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> path(get_info_from_id(id)->path);
    boost::uuids::uuid obj_uuid = str_to_uuid(path[1]);
    bool resolve(false);

    if (data.params.count("resolve") == 1)
        resolve = true;

    if (path[0] == "datacenters") {
        do_admin_set_name_internal(cluster_metadata.datacenters.datacenters, obj_uuid, new_name, resolve);
    } else if (path[0] == "machines") {
        do_admin_set_name_internal(cluster_metadata.machines.machines, obj_uuid, new_name, resolve);
    } else if (path[0] == "dummy_namespaces") {
        do_admin_set_name_internal(cluster_metadata.dummy_namespaces.namespaces, obj_uuid, new_name, resolve);
    } else if (path[0] == "memcached_namespaces") {
        do_admin_set_name_internal(cluster_metadata.memcached_namespaces.namespaces, obj_uuid, new_name, resolve);
    } else
        throw admin_cluster_exc_t("specified object does not have a name");

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

template <class obj_map>
void admin_cluster_link_t::do_admin_set_name_internal(obj_map& metadata,
                                                      const boost::uuids::uuid& uuid,
                                                      const std::string& new_name,
                                                      bool resolve) {
    typename obj_map::iterator i = metadata.find(uuid);

    if (i == metadata.end())
        throw admin_cluster_exc_t("no object found with id: " + uuid_to_str(uuid));

    if (i->second.get().name.in_conflict()) {
        if (!resolve)
            throw admin_cluster_exc_t("name is in conflict, use --resolve, or run 'help resolve' for more information");

        // Resolve the conflict
        i->second.get_mutable().name = i->second.get_mutable().name.make_resolving_version(new_name, sync_peer.get_uuid());
    } else if (resolve)
        throw admin_cluster_exc_t("name is not in conflict, command ignored");

    i->second.get_mutable().name = i->second.get_mutable().name.make_new_version(new_name, sync_peer.get_uuid());
}

void admin_cluster_link_t::do_admin_set_acks(admin_command_parser_t::command_data& data) {
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> dc_path(get_info_from_id(data.params["datacenter"][0])->path);
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string acks_str = data.params["num-acks"][0].c_str();

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
    std::vector<std::string> ns_path(get_info_from_id(data.params["namespace"][0])->path);
    std::vector<std::string> dc_path(get_info_from_id(data.params["datacenter"][0])->path);
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::string replicas_str = data.params["num-replicas"][0].c_str();
    int num_replicas = atoi(replicas_str.c_str());

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
    std::vector<std::string> ids = data.params["id"];
    bool errored = false;

    for (size_t i = 0; i < ids.size(); ++i) {
        try {
            std::vector<std::string> path(get_info_from_id(ids[i])->path);
            boost::uuids::uuid obj_uuid(str_to_uuid(path[1]));

            if (path[0] == "datacenters") {
                do_admin_remove_internal(cluster_metadata.datacenters.datacenters, obj_uuid);
            } else if (path[0] == "dummy_namespaces") {
                do_admin_remove_internal(cluster_metadata.dummy_namespaces.namespaces, obj_uuid);
            } else if (path[0] == "memcached_namespaces") {
                do_admin_remove_internal(cluster_metadata.memcached_namespaces.namespaces, obj_uuid);
            } else if (path[0] == "machines") {
                // TODO: check if the machine is up, if so, ask for confirmation
                do_admin_remove_internal(cluster_metadata.machines.machines, obj_uuid);
            } else
                throw admin_cluster_exc_t("unexpected error when looking up object: " + ids[i]);

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

template <class obj_map>
void admin_cluster_link_t::do_admin_remove_internal(obj_map& metadata, const boost::uuids::uuid& obj_uuid) {
    typename obj_map::iterator i = metadata.find(obj_uuid);

    if (i == metadata.end())
        throw admin_cluster_exc_t("object not found: " + uuid_to_str(obj_uuid));

    i->second.mark_deleted();
}

void admin_cluster_link_t::do_admin_resolve(admin_command_parser_t::command_data& data UNUSED) {
    printf("NYI\n");
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

