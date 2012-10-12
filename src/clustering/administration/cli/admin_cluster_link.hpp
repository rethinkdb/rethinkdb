#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_
#define CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "clustering/administration/admin_tracker.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/linenoise.hpp"
#include "clustering/administration/issues/local.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/metadata_change_handler.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "clustering/administration/suggester.hpp"
#include "rpc/connectivity/cluster.hpp"
#include "rpc/connectivity/multiplexer.hpp"

template <class metadata_t> class directory_read_manager_t;
template <class metadata_t> class directory_write_manager_t;
template <class metadata_t> class semilattice_manager_t;
template <class metadata_t> class semilattice_readwrite_view_t;

const std::string& guarantee_param_0(const std::map<std::string, std::vector<std::string> >& params, const std::string& name);

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
    admin_retry_exc_t() { }
    ~admin_retry_exc_t() throw () { }
    const char *what() const throw () { return "metadata update to peer was rejected, try again"; }
};

class admin_cluster_link_t {
public:
    admin_cluster_link_t(const peer_address_set_t &joins, int client_port, signal_t *interruptor);
    ~admin_cluster_link_t();

    // A way for the parser to do completions and parsing verification
    std::vector<std::string> get_ids(const std::string& base);
    std::vector<std::string> get_machine_ids(const std::string& base);
    std::vector<std::string> get_database_ids(const std::string& base);
    std::vector<std::string> get_namespace_ids(const std::string& base);
    std::vector<std::string> get_datacenter_ids(const std::string& base);
    std::vector<std::string> get_conflicted_ids(const std::string& base);

    // Commands that may be run by the parser
    void do_admin_list(const admin_command_parser_t::command_data& data);
    void do_admin_list_stats(const admin_command_parser_t::command_data& data);
    void do_admin_list_issues(const admin_command_parser_t::command_data& data);
    void do_admin_list_machines(const admin_command_parser_t::command_data& data);
    void do_admin_list_directory(const admin_command_parser_t::command_data& data);
    void do_admin_list_tables(const admin_command_parser_t::command_data& data);
    void do_admin_list_datacenters(const admin_command_parser_t::command_data& data);
    void do_admin_list_databases(const admin_command_parser_t::command_data& data);
    void do_admin_resolve(const admin_command_parser_t::command_data& data);
    void do_admin_pin_shard(const admin_command_parser_t::command_data& data);
    void do_admin_split_shard(const admin_command_parser_t::command_data& data);
    void do_admin_merge_shard(const admin_command_parser_t::command_data& data);
    void do_admin_set_name(const admin_command_parser_t::command_data& data);
    void do_admin_set_acks(const admin_command_parser_t::command_data& data);
    void do_admin_set_replicas(const admin_command_parser_t::command_data& data);
    void do_admin_set_primary(const admin_command_parser_t::command_data& data);
    void do_admin_unset_primary(const admin_command_parser_t::command_data& data);
    void do_admin_set_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_unset_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_set_database(const admin_command_parser_t::command_data& data);
    void do_admin_create_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_create_database(const admin_command_parser_t::command_data& data);
    void do_admin_create_table(const admin_command_parser_t::command_data& data);
    void do_admin_remove_machine(const admin_command_parser_t::command_data& data);
    void do_admin_remove_table(const admin_command_parser_t::command_data& data);
    void do_admin_remove_datacenter(const admin_command_parser_t::command_data& data);
    void do_admin_remove_database(const admin_command_parser_t::command_data& data);
    void do_admin_touch(const admin_command_parser_t::command_data& data);

    void sync_from();

    size_t machine_count() const;
    size_t available_machine_count();
    size_t issue_count();

private:

    static std::string truncate_uuid(const uuid_t& uuid);

    static const size_t minimum_uuid_substring = 4;
    static const size_t uuid_output_length = 8;

    std::vector<std::string> get_ids_internal(const std::string& base, const std::string& path);

    std::string peer_id_to_machine_name(const std::string& peer_id);

    size_t get_machine_count_in_datacenter(const cluster_semilattice_metadata_t& cluster_metadata, const datacenter_id_t& datacenter);

    void do_metadata_update(cluster_semilattice_metadata_t *cluster_metadata,
                            metadata_change_handler_t<cluster_semilattice_metadata_t>::metadata_change_request_t *change_request,
                            bool prioritize_distribution);

    template <class protocol_t>
    std::string admin_merge_shard_internal(namespaces_semilattice_metadata_t<protocol_t> *ns_map,
                                           const namespace_id_t &ns_id,
                                           const std::vector<std::string> &split_points);

    template <class protocol_t>
    std::string merge_shards(vclock_t<nonoverlapping_regions_t<protocol_t> > *shards_vclock,
                             const std::vector<std::string> &split_points);

    template <class protocol_t>
    std::string admin_split_shard_internal(namespaces_semilattice_metadata_t<protocol_t> *ns,
                                           const namespace_id_t &ns_id,
                                           const std::vector<std::string> &split_points);

    template <class protocol_t>
    std::string split_shards(vclock_t<nonoverlapping_regions_t<protocol_t> > *shards_vclock,
                             const std::vector<std::string> &split_points);

    template <class protocol_t>
    void do_admin_set_acks_internal(const datacenter_id_t& datacenter,
                                    int num_acks,
                                    namespace_semilattice_metadata_t<protocol_t> *ns);
    template <class map_type>
    void do_admin_set_replicas_internal(const namespace_id_t& ns_id,
                                        const datacenter_id_t& dc_id,
                                        int num_replicas,
                                        map_type &ns_map);

    template <class obj_map>
    void do_admin_set_name_internal(const uuid_t& uuid,
                                    const std::string& new_name,
                                    obj_map *metadata);

    void do_admin_remove_internal(const std::string& obj_type, const std::vector<std::string>& ids);

    template <class T>
    void do_admin_remove_internal_internal(const uuid_t& key, std::map<uuid_t, T> *obj_map);

    template <class protocol_t>
    void remove_machine_pinnings(const machine_id_t& machine,
                                 std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map);

    template <class protocol_t>
    namespace_id_t do_admin_create_table_internal(const std::string& name,
                                                  int port,
                                                  const datacenter_id_t& primary,
                                                  const std::string& primary_key,
                                                  const database_id_t& database,
                                                  namespaces_semilattice_metadata_t<protocol_t> *ns);

    template <class obj_map>
    void do_admin_set_database_table(const namespace_id_t &table_uuid,
                                     const database_id_t &db,
                                     obj_map *metadata);

    template <class obj_map>
    void do_admin_set_datacenter_namespace(const uuid_t &obj_uuid,
                                           const datacenter_id_t &dc,
                                           obj_map *metadata);

    void do_admin_set_datacenter_machine(const uuid_t obj_uuid,
                                         const datacenter_id_t dc,
                                         machines_semilattice_metadata_t::machine_map_t *metadata,
                                         cluster_semilattice_metadata_t *cluster_metadata);

    void remove_database_tables(const database_id_t& database, cluster_semilattice_metadata_t *cluster_metadata);

    template <class protocol_t>
    void remove_database_tables_internal(const database_id_t& database,
                                         std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map);

    void remove_datacenter_references(const datacenter_id_t& datacenter, cluster_semilattice_metadata_t *cluster_metadata);

    template <class protocol_t>
    void remove_datacenter_references_from_namespaces(const datacenter_id_t& datacenter,
                                                      std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<protocol_t> > > *ns_map);

    template <class map_type>
    void list_all_internal(const std::string& type, bool long_format, const map_type& obj_map, std::vector<std::vector<std::string> > *table);

    void list_all(bool long_format, const cluster_semilattice_metadata_t& cluster_metadata);

    void admin_stats_to_table(const std::string& machine,
                              const std::string& prefix,
                              const perfmon_result_t& stats,
                              std::vector<std::vector<std::string> > *table);

    template <class map_type>
    void add_namespaces(const std::string& protocol, bool long_format, const map_type& namespaces, std::vector<std::vector<std::string> > *table);

    struct shard_input_t {
        struct {
            bool exists;
            bool unbounded;
            store_key_t key;
        } left, right;
    };

    template <class protocol_t>
    void do_admin_pin_shard_internal(const shard_input_t& shard_in,
                                     const std::string& primary_str,
                                     const std::vector<std::string>& secondary_strs,
                                     const cluster_semilattice_metadata_t& cluster_metadata,
                                     namespace_semilattice_metadata_t<protocol_t>* ns);

    template <class protocol_t>
    typename protocol_t::region_t find_shard_in_namespace(const namespace_semilattice_metadata_t<protocol_t>& ns,
                                                          const shard_input_t& shard_in);

    template <class protocol_t>
    void list_pinnings(const namespace_semilattice_metadata_t<protocol_t>& ns,
                       const shard_input_t& shard_in,
                       const cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void list_pinnings_internal(const persistable_blueprint_t<protocol_t>& bp,
                                const key_range_t& shard,
                                const cluster_semilattice_metadata_t& cluster_metadata);

    struct machine_info_t {
        machine_info_t() : status(), primaries(0), secondaries(0), tables(0) { }

        std::string status;
        size_t primaries;
        size_t secondaries;
        size_t tables;
    };

    template <class protocol_t>
    void add_machine_info_from_blueprint(const persistable_blueprint_t<protocol_t>& bp, std::map<machine_id_t, machine_info_t> *results);

    template <class map_type>
    void build_machine_info_internal(const map_type& ns_map, std::map<machine_id_t, machine_info_t> *results);

    std::map<machine_id_t, machine_info_t> build_machine_info(const cluster_semilattice_metadata_t& cluster_metadata);

    struct namespace_info_t {
        namespace_info_t() : shards(0), replicas(0), primary(), database() { }

        // These may be set to -1 in the case of a conflict
        int shards;
        int replicas;
        std::string primary;
        std::string database;
    };

    template <class ns_type>
    namespace_info_t get_namespace_info(const ns_type& ns);

    template <class protocol_t>
    size_t get_replica_count_from_blueprint(const persistable_blueprint_t<protocol_t>& bp);

    struct datacenter_info_t {
        datacenter_info_t() : machines(0), primaries(0), secondaries(0), tables(0) { }

        size_t machines;
        size_t primaries;
        size_t secondaries;
        size_t tables;
    };

    std::map<datacenter_id_t, datacenter_info_t> build_datacenter_info(const cluster_semilattice_metadata_t& cluster_metadata);

    template <class map_type>
    void add_datacenter_affinities(const map_type& ns_map, std::map<datacenter_id_t, datacenter_info_t> *results);

    struct database_info_t {
        database_info_t() : tables(0) { }
        size_t tables;
    };

    std::map<database_id_t, database_info_t> build_database_info(const cluster_semilattice_metadata_t& cluster_metadata);

    template <class map_type>
    void add_database_tables(const map_type& ns_map, std::map<database_id_t, database_info_t> *results);

    void list_single_database(const database_id_t& db_id,
                              const database_semilattice_metadata_t& db,
                              const cluster_semilattice_metadata_t& cluster_metadata);

    void list_single_datacenter(const datacenter_id_t& dc_id,
                                const datacenter_semilattice_metadata_t& dc,
                                const cluster_semilattice_metadata_t& cluster_metadata);

    void list_single_machine(const machine_id_t& machine_id,
                             const machine_semilattice_metadata_t& machine,
                             const cluster_semilattice_metadata_t& cluster_metadata);

    template <class protocol_t>
    void list_single_namespace(const namespace_id_t& ns_id,
                               const namespace_semilattice_metadata_t<protocol_t>& ns,
                               const cluster_semilattice_metadata_t& cluster_metadata,
                               const std::string& protocol);

    template <class map_type>
    void add_single_database_affinities(const datacenter_id_t& db_id,
                                        const map_type& ns_map,
                                        const std::string& protocol,
                                        std::vector<std::vector<std::string> > *table);

    template <class map_type>
    void add_single_datacenter_affinities(const datacenter_id_t& dc_id,
                                          const map_type& ns_map,
                                          const std::string& protocol,
                                          std::vector<std::vector<std::string> > *table);

    template <class map_type>
    size_t add_single_machine_replicas(const machine_id_t& machine_id,
                                       const map_type& ns_map,
                                       std::vector<std::vector<std::string> > *table);

    template <class protocol_t>
    bool add_single_machine_blueprint(const machine_id_t& machine_id,
                                      const persistable_blueprint_t<protocol_t>& blueprint,
                                      const std::string& ns_uuid,
                                      const std::string& ns_name,
                                      std::vector<std::vector<std::string> > *table);

    template <class protocol_t>
    void add_single_namespace_replicas(const nonoverlapping_regions_t<protocol_t>& shards,
                                       const persistable_blueprint_t<protocol_t>& blueprint,
                                       const machines_semilattice_metadata_t::machine_map_t& machine_map,
                                       std::vector<std::vector<std::string> > *table);

    template <class T>
    void resolve_value(vclock_t<T> *field);

    void resolve_machine_value(machine_semilattice_metadata_t *machine,
                               const std::string& field);

    void resolve_datacenter_value(datacenter_semilattice_metadata_t *dc,
                                  const std::string& field);

    void resolve_database_value(database_semilattice_metadata_t *db,
                                const std::string& field);

    template <class protocol_t>
    void resolve_namespace_value(namespace_semilattice_metadata_t<protocol_t> *ns,
                                 const std::string& field);

    std::string path_to_str(const std::vector<std::string>& path);

    metadata_change_handler_t<cluster_semilattice_metadata_t>::request_mailbox_t::address_t choose_sync_peer();

    struct metadata_info_t;

    void clear_metadata_maps();

    void update_metadata_maps();

    template <class T>
    void add_subset_to_maps(const std::string& base, const T& data_map);

    metadata_info_t *get_info_from_id(const std::string& id);

    local_issue_tracker_t local_issue_tracker;
    thread_pool_log_writer_t log_writer;
    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;
    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    stat_manager_t stat_manager;
    log_server_t log_server;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;
    message_multiplexer_t::client_t semilattice_manager_client;
    const scoped_ptr_t<semilattice_manager_t<cluster_semilattice_metadata_t> > semilattice_manager_cluster;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;
    metadata_change_handler_t<cluster_semilattice_metadata_t> metadata_change_handler;
    message_multiplexer_t::client_t directory_manager_client;
    watchable_variable_t<cluster_directory_metadata_t> our_directory_metadata;
    const scoped_ptr_t<directory_read_manager_t<cluster_directory_metadata_t> > directory_read_manager;
    // TODO: Do we actually need directory_write_manager?
    const scoped_ptr_t<directory_write_manager_t<cluster_directory_metadata_t> > directory_write_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;
    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    // Issue tracking etc.
    admin_tracker_t admin_tracker;

    // Initial join
    initial_joiner_t initial_joiner;

    machine_id_t change_request_id;
    peer_id_t sync_peer_id;

    struct metadata_info_t {
        uuid_t uuid;
        std::string name;
        std::vector<std::string> path;
    };

    std::map<std::string, metadata_info_t*> uuid_map;
    std::multimap<std::string, metadata_info_t*> name_map;

    DISABLE_COPYING(admin_cluster_link_t);
};

#endif /* CLUSTERING_ADMINISTRATION_CLI_ADMIN_CLUSTER_LINK_HPP_ */
