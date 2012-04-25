#ifndef CLUSTERING_ADMINISTRATION_MAIN_ADMIN_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_ADMIN_HPP_

#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/manager.hpp"
#include "clustering/administration/logger.hpp"
#include "clustering/administration/suggester.hpp"
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include "clustering/administration/main/linenoise.hpp"

struct admin_parse_exc_t : public std::exception {
public:
    admin_parse_exc_t(const std::string& data) : info(data) { }
    ~admin_parse_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class rethinkdb_admin_app_t {
public:
    struct command_data;

private:

    struct param_options {
        param_options(const std::string& _name, int _count, bool _required) :
            name(_name), count(_count), required(_required) { }

        void add_option(const char *term);
        void add_options(size_t count, ...);

        const std::string name;
        const size_t count; // -1: unlimited, 0: flag only, n: n params expected
        const bool required;
        std::set<std::string> valid_options; // !uuid or !name or literal
    };

    struct command_info {
        command_info(std::string cmd,
                     std::string use,
                     bool sync,
                     void (rethinkdb_admin_app_t::* const fn)(command_data&)) :
            command(cmd), usage(use), post_sync(sync), do_function(fn) { }

        ~command_info();

        param_options * add_flag(const std::string& name, int count, bool required);
        param_options * add_positional(const std::string& name, int count, bool required);
        void add_subcommand(command_info *info);

        std::string command;
        std::string usage;
        const bool post_sync;
        void (rethinkdb_admin_app_t::* const do_function)(command_data&);

        std::vector<param_options *> positionals; // TODO: it is an error to have both positionals and subcommands
        std::map<std::string, param_options *> flags;
        std::map<std::string, command_info *> subcommands;
    };

public:

    struct command_data {
        command_data(const command_info *cmd_info) : info(cmd_info) { }
        const command_info * const info;
        std::map<std::string, std::vector<std::string> > params;
    };

    rethinkdb_admin_app_t(const std::set<peer_address_t> &joins, int client_port);
    ~rethinkdb_admin_app_t();
    command_data parse_command(const std::vector<std::string>& command_args);
    void run_command(command_data& data);
    void run_console();

private:

    void build_command_descriptions();

    void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata);

    void do_admin_set(command_data& data);
    void do_admin_list(command_data& data);
    void do_admin_move(command_data& data);
    void do_admin_make_datacenter(command_data& data);
    void do_admin_make_namespace(command_data& data);
    void do_admin_rename(command_data& data);
    void do_admin_remove(command_data& data);
    void do_admin_help(command_data& data);

    void set_metadata_value(const std::vector<std::string>& path, const std::string& value);

    void list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);
    void list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata);

    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata);

    void init_uuid_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata);
    void init_name_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata);

    void sync_from();
    void sync_to();

    std::map<std::string, command_info *>::const_iterator find_command(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches, const char *base_str);
    void add_option_matches(const param_options *option, const std::string& partial, linenoiseCompletions *completions, const char *base_str);
    void add_positional_matches(const command_info *info, size_t offset, const std::string& partial, linenoiseCompletions *completions, const char *base_str);
    void get_id_completions(const std::string& base, linenoiseCompletions *completions, const char *base_str);
    static void completion_generator_hook(const char *partial_raw, linenoiseCompletions *completions);
    void completion_generator(const char *partial_raw, linenoiseCompletions *completions);

    template <class T>
    void add_subset_to_uuid_path_map(const std::string& base, T& data_map);
    template <class T>
    void add_subset_to_name_path_map(const std::string& base, T& data_map, std::set<std::string>& collisions);

    std::vector<std::string> get_path_from_id(const std::string& id);

    local_issue_tracker_t local_issue_tracker;
    log_writer_t log_writer;
    connectivity_cluster_t connectivity_cluster;
    message_multiplexer_t message_multiplexer;
    message_multiplexer_t::client_t mailbox_manager_client;
    mailbox_manager_t mailbox_manager;
    stat_manager_t stat_manager;
    log_server_t log_server;
    cluster_directory_metadata_t cluster_directory_metadata;
    message_multiplexer_t::client_t::run_t mailbox_manager_client_run;
    message_multiplexer_t::client_t semilattice_manager_client;
    semilattice_manager_t<cluster_semilattice_metadata_t> semilattice_manager_cluster;
    message_multiplexer_t::client_t::run_t semilattice_manager_client_run;
    message_multiplexer_t::client_t directory_manager_client;
    directory_readwrite_manager_t<cluster_directory_metadata_t> directory_manager;
    message_multiplexer_t::client_t::run_t directory_manager_client_run;
    message_multiplexer_t::run_t message_multiplexer_run;
    connectivity_cluster_t::run_t connectivity_cluster_run;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;

    std::map<std::string, std::vector<std::string> > uuid_to_path;
    std::map<std::string, std::vector<std::string> > name_to_path;

    peer_id_t sync_peer;

    std::map<std::string, command_info *> command_descriptions;

    static rethinkdb_admin_app_t *instance;
};

#endif
