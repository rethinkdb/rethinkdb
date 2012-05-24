#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_
#define CLUSTERING_ADMINISTRATION_CLU_ADMIN_CLUSTER_LINK_HPP_

#include <curl/curl.h>

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
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/suggester.hpp"
#include "rpc/connectivity/cluster.hpp"
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

struct admin_retry_exc_t : public std::exception {
public:
    explicit admin_retry_exc_t(const std::string& data) : info(data) { }
    ~admin_retry_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class admin_cluster_link_t {
public:
    admin_cluster_link_t(const std::set<peer_address_t> &joins, int client_port, signal_t *interruptor);
    ~admin_cluster_link_t();

    // A way for the parser to do completions and parsing verification
    std::vector<std::string> get_ids(const std::string& base);
    std::vector<std::string> get_machine_ids(const std::string& base);
    std::vector<std::string> get_namespace_ids(const std::string& base);
    std::vector<std::string> get_datacenter_ids(const std::string& base);
    std::vector<std::string> get_conflicted_ids(const std::string& base);

    // Commands that may be run by the parser
    void do_admin_list(admin_command_parser_t::command_data& data);
    void do_admin_resolve(admin_command_parser_t::command_data& data);
    void do_admin_pin_shard(admin_command_parser_t::command_data& data);
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

    size_t machine_count() const;
    size_t available_machine_count();
    size_t issue_count();

private:

    static std::string truncate_uuid(const boost::uuids::uuid& uuid);

    static const size_t minimum_uuid_substring = 4;
    static const size_t uuid_output_length = 8;

    std::vector<std::string> get_ids_internal(const std::string& base, const std::string& path);

    size_t get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter);

    template <class protocol_t>
    void do_admin_set_acks_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                    const datacenter_id_t& datacenter,
                                    int num_acks,
                                    const std::string& post_path);
    template <class protocol_t>
    void do_admin_set_replicas_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                        const datacenter_id_t& datacenter,
                                        int num_replicas,
                                        const std::string& post_path);

    template <class obj_map>
    void do_admin_set_name_internal(obj_map& metadata,
                                    const boost::uuids::uuid& uuid,
                                    const std::string& new_name,
                                    bool resolve);

    template <class protocol_t>
    void remove_machine_pinnings(const machine_id_t& machine,
                                 const std::string& post_path,
                                 std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map);

    template <class protocol_t>
    void do_admin_create_namespace_internal(std::string& name,
                                            int port,
                                            datacenter_id_t& primary,
                                            const std::string& path);

    void remove_datacenter_references(const datacenter_id_t& datacenter, cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void remove_datacenter_references_from_namespaces(const datacenter_id_t& datacenter,
                                                      const std::string& post_path,
                                                      std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > >& ns_map);

    template <class map_type>
    void list_all_internal(const std::string& type, bool long_format, map_type& obj_map, std::vector<std::vector<std::string> >& table);

    void list_stats(bool long_format);
    void list_issues(bool long_format);
    void list_directory(bool long_format);
    void list_all(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);

    void list_namespaces(const std::string& type, bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    template <class map_type>
    void add_namespaces(const std::string& protocol, bool long_format, map_type& namespaces, std::vector<std::vector<std::string> >& table);

    struct shard_input_t {
        struct {
            bool inf;
            std::string key;
        } left, right;
    };

    template <class protocol_t>
    void do_admin_pin_shard_internal(namespace_semilattice_metadata_t<protocol_t>& ns,
                                     const shard_input_t& shard_in,
                                     const std::string& primary_str,
                                     const std::vector<std::string>& secondary_strs,
                                     cluster_semilattice_metadata_t& cluster_metadata,
                                     const std::string& post_path);

    template <class protocol_t>
    typename protocol_t::region_t find_shard_in_namespace(namespace_semilattice_metadata_t<protocol_t>& ns,
                                                          const shard_input_t& shard_in);

    template <class protocol_t>
    void list_pinnings(namespace_semilattice_metadata_t<protocol_t>& ns,
                       const shard_input_t& shard_in,
                       cluster_semilattice_metadata_t& cluster_metadata);

    template <class bp_type>
    void list_pinnings_internal(const bp_type& bp,
                                const key_range_t& shard,
                                cluster_semilattice_metadata_t& cluster_metadata);

    template <class map_type, class value_type>
    void insert_pinning(map_type& region_map, const key_range_t& shard, value_type& value);

    struct machine_info_t {
        machine_info_t() : status(), primaries(0), secondaries(0), namespaces(0) { }

        std::string status;
        size_t primaries;
        size_t secondaries;
        size_t namespaces;
    };

    template <class bp_type>
    void add_machine_info_from_blueprint(const bp_type& bp, std::map<machine_id_t, machine_info_t>& results);

    template <class map_type>
    void build_machine_info_internal(const map_type& ns_map, std::map<machine_id_t, machine_info_t>& results);

    std::map<machine_id_t, machine_info_t> build_machine_info(cluster_semilattice_metadata_t& cluster_metadata);

    struct namespace_info_t {
        namespace_info_t() : shards(0), replicas(0), primary() { }

        // These may be set to -1 in the case of a conflict
        int shards;
        int replicas;
        std::string primary;
    };

    template <class ns_type>
    namespace_info_t get_namespace_info(ns_type& ns);

    template <class bp_type>
    size_t get_replica_count_from_blueprint(const bp_type& bp);

    struct datacenter_info_t {
        datacenter_info_t() : machines(0), primaries(0), secondaries(0), namespaces(0) { }

        size_t machines;
        size_t primaries;
        size_t secondaries;
        size_t namespaces;
    };

    std::map<datacenter_id_t, datacenter_info_t> build_datacenter_info(cluster_semilattice_metadata_t& cluster_metadata);

    template <class map_type>
    void add_datacenter_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t>& results);

    void list_single_datacenter(const datacenter_id_t& dc_id,
                                datacenter_semilattice_metadata_t& dc,
                                cluster_semilattice_metadata_t& cluster_metadata);

    void list_single_machine(const machine_id_t& machine_id,
                             machine_semilattice_metadata_t& machine,
                             cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void list_single_namespace(const namespace_id_t& ns_id,
                               namespace_semilattice_metadata_t<protocol_t>& ns,
                               cluster_semilattice_metadata_t& cluster_metadata,
                               const std::string& protocol);

    template <class map_type>
    void add_single_datacenter_affinities(const datacenter_id_t& dc_id,
                                          map_type& ns_map,
                                          std::vector<std::vector<std::string> >& table,
                                          const std::string& protocol);

    template <class map_type>
    size_t add_single_machine_replicas(const machine_id_t& machine_id,
                                       map_type& ns_map,
                                       std::vector<std::vector<std::string> >& table);

    template <class protocol_t>
    bool add_single_machine_blueprint(const machine_id_t& machine_id,
                                      persistable_blueprint_t<protocol_t>& blueprint,
                                      std::vector<std::vector<std::string> >& table,
                                      const std::string& ns_uuid,
                                      const std::string& ns_name);

    template <class protocol_t>
    void add_single_namespace_replicas(std::set<typename protocol_t::region_t>& shards,
                                       persistable_blueprint_t<protocol_t>& blueprint,
                                       machines_semilattice_metadata_t::machine_map_t& machine_map,
                                       std::vector<std::vector<std::string> >& table);

    template <class T>
    void resolve_value(const vclock_t<T>& field,
                       const std::string& field_name,
                       const std::string& post_path);

    void resolve_machine_value(machine_semilattice_metadata_t& machine,
                               const std::string& field,
                               const std::string& post_path);

    void resolve_datacenter_value(datacenter_semilattice_metadata_t& dc,
                                  const std::string& field,
                                  const std::string& post_path);

    template <class protocol_t>
    void resolve_namespace_value(namespace_semilattice_metadata_t<protocol_t>& ns,
                                 const std::string& field,
                                 const std::string& post_path);

    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata);

    std::string path_to_str(const std::vector<std::string>& path);

    static size_t handle_post_result(char *ptr, size_t size, size_t nmemb, void *param);
    std::string post_result;

    template <class T>
    void post_metadata(std::string path, T& metadata);
    std::string create_metadata(const std::string& path);
    void delete_metadata(const std::string& path);
    void post_internal(std::string path, std::string data);

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

    struct metadata_info_t {
        std::string uuid;
        std::string name;
        std::vector<std::string> path;
    };

    std::map<std::string, metadata_info_t*> uuid_map;
    std::multimap<std::string, metadata_info_t*> name_map;

    void clear_metadata_maps();
    void update_metadata_maps();
    template <class T>
    void add_subset_to_maps(const std::string& base, T& data_map);
    metadata_info_t* get_info_from_id(const std::string& id);

    CURL *curl_handle;
    struct curl_slist *curl_header_list;
    std::string sync_peer;
    peer_id_t sync_peer_id;

    DISABLE_COPYING(admin_cluster_link_t);
};

#endif
