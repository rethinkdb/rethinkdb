#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_
#define CLUSTERING_ADMINISTRATION_CLU_ADMIN_CLUSTER_LINK_HPP_

#include <vector>
#include <string>

#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/linenoise.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/local_to_global.hpp"
#include "clustering/administration/issues/machine_down.hpp"
#include "clustering/administration/issues/name_conflict.hpp"
#include "clustering/administration/issues/pinnings_shards_mismatch.hpp"
#include "clustering/administration/issues/unsatisfiable_goals.hpp"
#include "clustering/administration/issues/vector_clock_conflict.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/suggester.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/semilattice/view.hpp"

struct admin_cluster_exc_t : public std::exception {
public:
    explicit admin_cluster_exc_t(const std::string& data) : info(data) { }
    ~admin_cluster_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class admin_cluster_link_t {
public:
    admin_cluster_link_t(const std::set<peer_address_t> &joins, int client_port, signal_t *interruptor);

    // A way for the parser to do completions and parsing verification
    std::vector<std::string> get_ids(const std::string& base);

    // Commands that may be run by the parser
    void do_admin_list(admin_command_parser_t::command_data& data);
    void do_admin_resolve(admin_command_parser_t::command_data& data);
    void do_admin_split_shard(admin_command_parser_t::command_data& data);
    void do_admin_merge_shard(admin_command_parser_t::command_data& data);
    void do_admin_set_name(admin_command_parser_t::command_data& data);
    void do_admin_set_acks(admin_command_parser_t::command_data& data);
    void do_admin_set_replicas(admin_command_parser_t::command_data& data);
    void do_admin_set_datacenter(admin_command_parser_t::command_data& data);
    void do_admin_create_datacenter(admin_command_parser_t::command_data& data);
    void do_admin_create_namespace(admin_command_parser_t::command_data& data);
    void do_admin_remove(admin_command_parser_t::command_data& data);

    void sync_from();
    void sync_to();

    size_t machine_count() const;
    size_t available_machine_count();
    size_t issue_count();

private:

    static std::string truncate_uuid(const boost::uuids::uuid& uuid);

    static const size_t minimum_uuid_substring = 4;
    static const size_t uuid_output_length = 8;

    void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata);

    void set_metadata_value(const std::vector<std::string>& path, const std::string& value);

    size_t get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter);

    template <class protocol_t>
    void do_admin_set_acks_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_acks);
    template <class protocol_t>
    void do_admin_set_replicas_internal(namespace_semilattice_metadata_t<protocol_t>& ns, const datacenter_id_t& datacenter, int num_replicas);

    void list_issues(bool long_format);
    void list_directory(bool long_format);
    void list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);

    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata);

    void init_uuid_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata);
    void init_name_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata);

    template <class T>
    void add_subset_to_uuid_path_map(const std::string& base, T& data_map);
    template <class T>
    void add_subset_to_name_path_map(const std::string& base, T& data_map);

    std::vector<std::string> get_path_from_id(const std::string& id);

    local_issue_tracker_t local_issue_tracker;
    log_writer_t log_writer;
    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;
    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    stat_manager_t stat_manager;
    log_server_t log_server;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;
    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;
    message_multiplexer_t::client_t directory_manager_client;
    watchable_variable_t<cluster_directory_metadata_t> our_directory_metadata;
    directory_read_manager_t<cluster_directory_metadata_t> directory_read_manager;
    directory_write_manager_t<cluster_directory_metadata_t> directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;
    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;

    // Issue tracking
    global_issue_aggregator_t issue_aggregator;
    remote_issue_collector_t remote_issue_tracker;
    global_issue_aggregator_t::source_t remote_issue_tracker_feed;
    machine_down_issue_tracker_t machine_down_issue_tracker;
    global_issue_aggregator_t::source_t machine_down_issue_tracker_feed;
    name_conflict_issue_tracker_t name_conflict_issue_tracker;
    global_issue_aggregator_t::source_t name_conflict_issue_tracker_feed;
    vector_clock_conflict_issue_tracker_t vector_clock_conflict_issue_tracker;
    global_issue_aggregator_t::source_t vector_clock_issue_tracker_feed;
    pinnings_shards_mismatch_issue_tracker_t<memcached_protocol_t> mc_pinnings_shards_mismatch_issue_tracker;
    global_issue_aggregator_t::source_t mc_pinnings_shards_mismatch_issue_tracker_feed;
    pinnings_shards_mismatch_issue_tracker_t<mock::dummy_protocol_t> dummy_pinnings_shards_mismatch_issue_tracker;
    global_issue_aggregator_t::source_t dummy_pinnings_shards_mismatch_issue_tracker_feed;
    unsatisfiable_goals_issue_tracker_t unsatisfiable_goals_issue_tracker;
    global_issue_aggregator_t::source_t unsatisfiable_goals_issue_tracker_feed;

    // Initial join
    initial_joiner_t initial_joiner;

    std::map<std::string, std::vector<std::string> > uuid_to_path;
    std::multimap<std::string, std::vector<std::string> > name_to_path;

    peer_id_t sync_peer;

    DISABLE_COPYING(admin_cluster_link_t);
};

#endif
