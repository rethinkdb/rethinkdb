#include "clustering/administration/cli/admin.hpp"

#include <cstdarg>
#include <iostream>
#include <map>
#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

#include "clustering/administration/suggester.hpp"
#include "clustering/administration/main/initial_join.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"

// TODO: make a useful prompt
const char *prompt = " > ";

rethinkdb_admin_app_t *rethinkdb_admin_app_t::instance = NULL;

const char *rethinkdb_admin_app_t::set_command = "set";
const char *rethinkdb_admin_app_t::list_command = "ls";
const char *rethinkdb_admin_app_t::make_command = "mk";
const char *rethinkdb_admin_app_t::move_command = "mv";
const char *rethinkdb_admin_app_t::help_command = "help";
const char *rethinkdb_admin_app_t::rename_command = "rename";
const char *rethinkdb_admin_app_t::remove_command = "remove";
const char *rethinkdb_admin_app_t::complete_command = "complete";

const char *rethinkdb_admin_app_t::set_usage = "( <uuid> | <name> ) <field> ... [--resolve] [--value <value>]";
const char *rethinkdb_admin_app_t::list_usage = "[ datacenters | namespaces [--protocol <protocol>] | machines | issues | <name> | <uuid> ] [--long]";
const char *rethinkdb_admin_app_t::make_usage = "( datacenter | namespace --port <port> --protocol ( memcached | dummy ) [--primary <primary datacenter uuid>] ) [--name <name>]";
const char *rethinkdb_admin_app_t::make_namespace_usage = "[--port <port>] [--protocol ( memcached | dummy ) ] [--primary <primary datacenter uuid>] [--name <name>]";
const char *rethinkdb_admin_app_t::make_datacenter_usage = "[--name <name>]";
const char *rethinkdb_admin_app_t::move_usage = "( <target-uuid> | <target-name> ) ( <datacenter-uuid> | <datacenter-name> ) [--resolve]";
const char *rethinkdb_admin_app_t::help_usage = "[ ls | mk | mv | rm | set | rename | help ]";
const char *rethinkdb_admin_app_t::rename_usage = "( <uuid> | <name> ) <new name> [--resolve]";
const char *rethinkdb_admin_app_t::remove_usage = "( <uuid> | <name> )";

namespace po = boost::program_options;

void rethinkdb_admin_app_t::param_options::add_option(const char *term) {
    valid_options.insert(term);
}

void rethinkdb_admin_app_t::param_options::add_options(const char *term, ...) {
    va_list terms;
    va_start(terms, term);
    while (term != NULL) {
        add_option(term);
        term = va_arg(terms, const char *);
    }
    va_end(terms);
}

rethinkdb_admin_app_t::command_info::~command_info() {
    for(std::map<std::string, param_options *>::iterator i = flags.begin(); i != flags.end(); ++i)
        delete i->second;

    for(std::vector<param_options *>::iterator i = positionals.begin(); i != positionals.end(); ++i)
        delete *i;

    for(std::map<std::string, command_info *>::iterator i = subcommands.begin(); i != subcommands.end(); ++i)
        delete i->second;
}

rethinkdb_admin_app_t::param_options * rethinkdb_admin_app_t::command_info::add_flag(const std::string& name, int count, bool required)
{
    param_options *option = new param_options(name, count, required);
    flags.insert(std::make_pair(name, option));
    return option;
}

rethinkdb_admin_app_t::param_options * rethinkdb_admin_app_t::command_info::add_positional(const std::string& name, int count, bool required)
{
    param_options *option = new param_options(name, count, required);
    positionals.push_back(option);
    return option;
}

void rethinkdb_admin_app_t::command_info::add_subcommand(command_info *info)
{
    rassert(subcommands.count(info->command) == 0);
    subcommands.insert(std::make_pair(info->command, info));
}

// TODO: this was copied from semilattice_http_app, unify them
void rethinkdb_admin_app_t::fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata) {
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

rethinkdb_admin_app_t::rethinkdb_admin_app_t(const std::set<peer_address_t> &joins, int client_port) :
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
    dummy_pinnings_shards_mismatch_issue_tracker_feed(&issue_aggregator, &dummy_pinnings_shards_mismatch_issue_tracker)
{
    rassert(instance == NULL);
    instance = this;

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

    build_command_descriptions();
}

rethinkdb_admin_app_t::~rethinkdb_admin_app_t() {
    rassert(instance == this);
    instance = NULL;
}

template <class T>
void rethinkdb_admin_app_t::add_subset_to_uuid_path_map(const std::string& base, T& data_map) {
    std::vector<std::string> base_path;
    base_path.push_back(base);
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        std::string uuid(uuid_to_str(i->first));
        std::vector<std::string> current_path(base_path);
        current_path.push_back(uuid);
        uuid_to_path.insert(std::make_pair<std::string, std::vector<std::string> >(uuid, current_path));
    }
}

void rethinkdb_admin_app_t::init_uuid_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata) {
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
    uuid_to_path.clear();
    add_subset_to_uuid_path_map("machines", cluster_metadata.machines.machines);
    add_subset_to_uuid_path_map("datacenters", cluster_metadata.datacenters.datacenters);
    add_subset_to_uuid_path_map("dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces);
    add_subset_to_uuid_path_map("memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces);
}

template <class T>
void rethinkdb_admin_app_t::add_subset_to_name_path_map(const std::string& base, T& data_map, std::set<std::string>& collisions) {
    std::vector<std::string> base_path;
    base_path.push_back(base);
    for (typename T::const_iterator i = data_map.begin(); i != data_map.end(); ++i) {
        std::string name(i->second.get().name.get());
        std::string uuid(uuid_to_str(i->first));
        if (!name.empty() && collisions.count(name) == 0) {
            if (name_to_path.find(name) != name_to_path.end()) {
                collisions.insert(name);
                name_to_path.erase(name);
            } else {
                std::vector<std::string> current_path(base_path);
                current_path.push_back(uuid);
                name_to_path.insert(std::make_pair<std::string, std::vector<std::string> >(name, current_path));
            }
        }
    }
}

void rethinkdb_admin_app_t::init_name_to_path_map(const cluster_semilattice_metadata_t& cluster_metadata) {
    std::set<std::string> collisions;
    name_to_path.clear();
    add_subset_to_name_path_map("machines", cluster_metadata.machines.machines, collisions);
    add_subset_to_name_path_map("datacenters", cluster_metadata.datacenters.datacenters, collisions);
    add_subset_to_name_path_map("dummy_namespaces", cluster_metadata.dummy_namespaces.namespaces, collisions);
    add_subset_to_name_path_map("memcached_namespaces", cluster_metadata.memcached_namespaces.namespaces, collisions);
}

void rethinkdb_admin_app_t::build_command_descriptions() {
    command_descriptions.clear();

    {
        command_info *info = new command_info(set_command, set_usage, true, &rethinkdb_admin_app_t::do_admin_set);
        info->add_flag("resolve", 0, false);
        info->add_flag("value", 1, true);
        info->add_positional("path", 1, true)->add_option("!id");
        info->add_positional("fields", -1, true);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(list_command, list_usage, false, &rethinkdb_admin_app_t::do_admin_list);
        info->add_positional("filter", 1, false)->add_options("issues", "machines", "namespaces", "datacenters", "!id", NULL); // TODO: how to handle --protocol: for now, just error if specified in the wrong situation
        info->add_flag("protocol", 1, false)->add_options("memcached", "dummy", NULL);
        info->add_flag("long", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(make_command, make_usage, true, NULL);
        command_info *namespace_command = new command_info("namespace", make_namespace_usage, true, &rethinkdb_admin_app_t::do_admin_make_namespace);
        command_info *datacenter_command = new command_info("datacenter", make_datacenter_usage, true, &rethinkdb_admin_app_t::do_admin_make_datacenter);

        info->add_subcommand(namespace_command);
        info->add_subcommand(datacenter_command);

        datacenter_command->add_flag("name", 1, false);

        namespace_command->add_flag("name", 1, false);
        namespace_command->add_flag("protocol", 1, true)->add_options("memcached", "dummy", NULL);
        namespace_command->add_flag("primary", 1, false)->add_option("!id");
        namespace_command->add_flag("port", 1, true);

        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(move_command, move_usage, true, &rethinkdb_admin_app_t::do_admin_move);
        info->add_positional("id", 1, true)->add_option("!id");
        info->add_positional("datacenter", 1, true)->add_option("!id"); // TODO: restrict this to datacenters only
        info->add_flag("resolve", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(remove_command, remove_usage, true, &rethinkdb_admin_app_t::do_admin_remove);
        info->add_positional("id", 1, true)->add_option("!id");
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(rename_command, rename_usage, true, &rethinkdb_admin_app_t::do_admin_rename);
        info->add_positional("id", 1, true)->add_option("!id");
        info->add_positional("new-name", 1, true);
        info->add_flag("resolve", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(help_command, help_usage, false, &rethinkdb_admin_app_t::do_admin_help);
        info->add_positional("type", -1, false)->add_options("ls", "mk", "mv", "rm", "set", "help", "rename", NULL);
        command_descriptions.insert(std::make_pair(info->command, info));
    }
}

rethinkdb_admin_app_t::command_data rethinkdb_admin_app_t::parse_command(const std::vector<std::string>& line)
{
    std::map<std::string, command_info *>::iterator i = command_descriptions.find(line[0]);
    size_t index;

    if (i == command_descriptions.end())
        throw admin_parse_exc_t("unrecognized command: " + line[0]);
    else
        // If any subcommands exist, one must be selected by the next substring
        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info *>::iterator temp = i->second->subcommands.find(line[index]);
            if (temp == i->second->subcommands.end())
                throw admin_parse_exc_t("unrecognized subcommand: " + line[index]);
            i = temp;
        }

    if (!i->second->subcommands.empty())
        throw admin_parse_exc_t("incomplete command");

    command_info *info = i->second;
    command_data data(info);
    size_t positional_index = 0;
    size_t positional_count = 0;

    // Now we have arrived at the correct command info, parse the rest of the command line
    for (; index < line.size(); ++index) {
        if (line[index].find("--") == 0) { // If the argument starts with a "--", it must be a flag option
            std::map<std::string, param_options *>::iterator opt_iter = info->flags.find(line[index].substr(2));
            if (opt_iter != info->flags.end()) {
                param_options *option = opt_iter->second;
                if (option->count == 0) // Flag only
                    data.params[option->name] = std::vector<std::string>();
                else {
                    size_t remaining = option->count;
                    if (index + remaining >= line.size())
                        throw admin_parse_exc_t("not enough arguments provided for flag: " + line[index]);

                    while(remaining > 0) {
                        ++index;
                        --remaining;
                        if (line[index].find("--") == 0)
                            throw admin_parse_exc_t("not enough arguments provided for flag: " + line[index]);
                        data.params[option->name].push_back(line[index]);
                    }
                }
            } else
                throw admin_parse_exc_t("unrecognized flag: " + line[index]);
        } else { // Otherwise, it must be a positional
            if (positional_index > info->positionals.size() - 1)
                throw admin_parse_exc_t("too many positional arguments");
            data.params[info->positionals[positional_index]->name].push_back(line[index]);
            if (++positional_count >= info->positionals[positional_index]->count) {
                positional_count = 0;
                ++positional_index;
            }
        }
    }

    // Make sure all required options have been provided
    for (size_t j = 0; j < info->positionals.size(); ++j) {
        if (info->positionals[j]->required && data.params.find(info->positionals[j]->name) == data.params.end())
            throw admin_parse_exc_t("insufficient positional parameters provided");
    }

    for (std::map<std::string, param_options *>::iterator j = info->flags.begin(); j != info->flags.end(); ++j) {
        if (j->second->required && data.params.find(j->second->name) == data.params.end())
            throw admin_parse_exc_t("required flag not specified: " + j->second->name);
    }

    return data;
}

void rethinkdb_admin_app_t::sync_from() {
    cond_t interruptor;
    semilattice_metadata->sync_from(sync_peer, &interruptor);
    init_uuid_to_path_map(semilattice_metadata->get());
    init_name_to_path_map(semilattice_metadata->get());
}

void rethinkdb_admin_app_t::sync_to() {
    cond_t interruptor;
    semilattice_metadata->sync_to(sync_peer, &interruptor);
}

void rethinkdb_admin_app_t::run_command(command_data& data)
{
    sync_from();

    if (data.info->do_function == NULL)
        throw admin_parse_exc_t("incomplete command (should have been caught earlier, though)");

    (this->*(data.info->do_function))(data);

    if (data.info->post_sync)
        sync_to();
}

void rethinkdb_admin_app_t::completion_generator_hook(const char *raw, linenoiseCompletions *completions) {
    if (instance == NULL)
        logERR("linenoise completion generator called without an instance of rethinkdb_admin_app_t");
    else {
        char last_char = raw[strlen(raw) - 1];
        bool partial = (last_char != ' ') && (last_char != '\t') && (last_char != '\r') && (last_char != '\n');
        std::vector<std::string> line = po::split_unix(raw);
        instance->completion_generator(line, completions, partial);
    }
}

void rethinkdb_admin_app_t::get_id_completions(const std::string& base, linenoiseCompletions *completions) {
    sync_from();

    // Build completion values
    for (std::map<std::string, std::vector<std::string> >::iterator i = uuid_to_path.lower_bound(base); i != uuid_to_path.end() && i->first.find(base) == 0; ++i)
        linenoiseAddCompletion(completions, i->first.c_str());

    for (std::map<std::string, std::vector<std::string> >::iterator i = name_to_path.lower_bound(base); i != name_to_path.end() && i->first.find(base) == 0; ++i)
        linenoiseAddCompletion(completions, i->first.c_str());
}

std::map<std::string, rethinkdb_admin_app_t::command_info *>::const_iterator rethinkdb_admin_app_t::find_command(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches) {
    std::map<std::string, command_info *>::const_iterator i = commands.find(str);
    if (add_matches) {
        if (i == commands.end())
            for (i = commands.lower_bound(str); i != commands.end() && i->first.find(str) == 0; ++i)
                linenoiseAddCompletion(completions, i->first.c_str());
        else
            linenoiseAddCompletion(completions, i->first.c_str());

        return commands.end();
    }
    return i;
}

void rethinkdb_admin_app_t::add_option_matches(const param_options *option, const std::string& partial, linenoiseCompletions *completions) {

    if (partial.find("!") == 0) // Don't allow completions beginning with '!', as we use it as a special case
        return;

    for (std::set<std::string>::iterator i = option->valid_options.lower_bound(partial); i != option->valid_options.end() && i->find(partial) == 0; ++i) {
        if (i->find("!") != 0) // skip special cases
            linenoiseAddCompletion(completions, i->c_str());
    }

    if (option->valid_options.count("!id") > 0) {
        get_id_completions(partial, completions);
    }
}

void rethinkdb_admin_app_t::add_positional_matches(const command_info *info, size_t offset, const std::string& partial, linenoiseCompletions *completions) {
    if (info->positionals.size() > offset)
        add_option_matches(info->positionals[offset], partial, completions);
}

void rethinkdb_admin_app_t::completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial) {
    // TODO: this function is too long, too complicated, and is probably redundant in some cases
    //   I'm sure it can be simplified, but for now it seems to work

    if (line.empty()) { // No command specified, available completions are all basic commands
        for (std::map<std::string, command_info *>::iterator i = command_descriptions.begin(); i != command_descriptions.end(); ++i)
            linenoiseAddCompletion(completions, (i->first + " ").c_str());
    } else { // Command specified, find a matching command/subcommand
        size_t index = 0;
        std::map<std::string, command_info *>::const_iterator i = command_descriptions.find(line[0]);
        std::map<std::string, command_info *>::const_iterator end = command_descriptions.end();

        i = find_command(command_descriptions, line[index], completions, partial && line.size() == 1);
        if (i == end)
            return;

        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            end = i->second->subcommands.end();
            i = find_command(i->second->subcommands, line[index], completions, partial && line.size() == index + 1);
            if (i == end)
                return;
        }

        command_info *cmd = i->second;

        if (!cmd->subcommands.empty()) {
            // We're at the end of the line, and there are still more subcommands
            find_command(cmd->subcommands, std::string(), completions, !partial);
            return;
        }

        if (index == line.size()) {
            if (!partial) // Only the command specified, show positionals
                add_positional_matches(cmd, 0, std::string(), completions);
            else // command specified with no space at the end, throw one on to show completion
                linenoiseAddCompletion(completions, line[index - 1].c_str());
            return;
        }

        // We are now at a terminal command, figure out which parameter we're on
        size_t positional_index = 0;
        size_t positional_count = 0;

        for (; index < line.size(); ++index) {
            if (line[index].find("--") == 0) { // This is a flag, skip any completed options
                std::map<std::string, param_options *>::iterator opt_iter = cmd->flags.find(line[index].substr(2));
                if (opt_iter != cmd->flags.end()) {
                    if (line.size() <= index + opt_iter->second->count) { // Not enough params for this flag
                        if (!partial) // print valid_options
                            add_option_matches(opt_iter->second, std::string(), completions);
                        else // find matches of last word with valid_options
                            add_option_matches(opt_iter->second, line[line.size() - 1], completions);
                        return;
                    } else if (line.size() == index + opt_iter->second->count + 1) {
                        if (!partial)
                            add_positional_matches(cmd, positional_index, std::string(), completions);
                        else // find matches of last word with valid_options
                            add_option_matches(opt_iter->second, line[line.size() - 1], completions);
                        return;
                    } else
                        index += opt_iter->second->count;
                } else if (line.size() == index + 1) {
                    if (partial)
                        for (opt_iter = cmd->flags.lower_bound(line[index].substr(2)); opt_iter != cmd->flags.end() && opt_iter->first.find(line[index].substr(2)) == 0; ++opt_iter)
                            linenoiseAddCompletion(completions, ("--" + opt_iter->first).c_str());
                    else
                        add_positional_matches(cmd, positional_index, std::string(), completions);
                    return;
                }

            } else { // this is a positional, attempt to advance
                // We're at the end of the line, handle using the current positional
                if (index == line.size() - 1) {
                    if (!partial) {
                        add_positional_matches(cmd, positional_index + 1, std::string(), completions);
                    } else
                        add_positional_matches(cmd, positional_index, line[line.size() - 1], completions);
                    return;
                }

                if (++positional_count == cmd->positionals[positional_index]->count) {
                    ++positional_index;
                    positional_count = 0;
                }
            }
        }
    }
}

void rethinkdb_admin_app_t::run_complete(const std::vector<std::string>& command_args) {
    linenoiseCompletions completions = { 0, NULL };

    if (command_args.size() < 2 || (command_args[1] != "partial" && command_args[1] != "full"))
        throw admin_parse_exc_t("usage: rethinkdb admin complete ( partial | full ) [...]");

    completion_generator(std::vector<std::string>(command_args.begin() + 2, command_args.end()), &completions, command_args[1] == "partial");

    for (size_t i = 0; i < completions.len; ++i)
        printf("%s\n", completions.cvec[i]);

    linenoiseFreeCompletions(&completions);
}

void rethinkdb_admin_app_t::run_console() {
    linenoiseSetCompletionCallback(completion_generator_hook);
    char *raw_line = linenoise(prompt);
    std::string line;

    while(raw_line != NULL) {
        line.assign(raw_line);

        if (line == "exit")
            break;

        if (!line.empty()) {
            try {
                std::vector<std::string> cmd_line = po::split_unix(line);
                command_data data(parse_command(cmd_line));
                run_command(data);
            } catch (std::exception& ex) {
                std::cerr << ex.what() << std::endl;
            } catch (...) {
                std::cerr << "unknown error" << std::endl;
            }

            linenoiseHistoryAdd(raw_line);
        }

        raw_line = linenoise(prompt);
    }
    std::cout << std::endl;
}

boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > rethinkdb_admin_app_t::traverse_directory(const std::vector<std::string>& path, namespace_metadata_ctx_t& json_ctx, cluster_semilattice_metadata_t& cluster_metadata)
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

std::vector<std::string> rethinkdb_admin_app_t::get_path_from_id(const std::string& id) {
    if (is_uuid(id)) {
        std::map<std::string, std::vector<std::string> >::iterator item = uuid_to_path.find(id);
        if (item == uuid_to_path.end())
            throw admin_parse_exc_t("uuid not found: " + id);
        return item->second;
    } else {
        std::map<std::string, std::vector<std::string> >::iterator item = name_to_path.find(id);
        if (item == name_to_path.end())
            throw admin_parse_exc_t("name not found or not unique: " + id);
        return item->second;
    }
}

void rethinkdb_admin_app_t::do_admin_set(command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    std::vector<std::string> param_path(data.params["path"]);
    std::vector<std::string> fields(data.params["fields"]);
    std::vector<std::string> full_path(get_path_from_id(param_path[0]));

    std::copy(param_path.begin() + 1, param_path.end(), std::back_inserter(full_path));
    std::copy(fields.begin(), fields.end(), std::back_inserter(full_path));

    if (data.params.count("resolve") == 1)
        full_path.push_back("resolve");

    set_metadata_value(full_path, data.params["value"][0]);
}

void rethinkdb_admin_app_t::do_admin_list(command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
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
    } else if (!id.empty()) {
        // TODO: special formatting for each object type, instead of JSON
        obj_path = get_path_from_id(id);
        std::cout << cJSON_print_std_string(scoped_cJSON_t(traverse_directory(obj_path, json_ctx, cluster_metadata)->render(json_ctx)).get()) << std::endl;
    } else {
        list_machines(long_format, cluster_metadata);
        list_datacenters(long_format, cluster_metadata);
        list_dummy_namespaces(long_format, cluster_metadata);
        list_memcached_namespaces(long_format, cluster_metadata);
    }
}

void rethinkdb_admin_app_t::list_issues(bool long_format UNUSED) {
    std::list<clone_ptr_t<global_issue_t> > issues = issue_aggregator.get_issues();
    std::cout << "Issues: " << std::endl;

    for (std::list<clone_ptr_t<global_issue_t> >::iterator i = issues.begin(); i != issues.end(); ++i) {
        std::cout << (*i)->get_description() << std::endl;
    }
}

void rethinkdb_admin_app_t::list_datacenters(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::cout << "Datacenters:" << std::endl;
    for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator i = cluster_metadata.datacenters.datacenters.begin(); i != cluster_metadata.datacenters.datacenters.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::cout << " " << i->first;
            if (long_format)
                std::cout << " " << i->second.get().name.get();
            std::cout << std::endl;
        }
    }
}

void rethinkdb_admin_app_t::list_dummy_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::cout << "Dummy Namespaces:" << std::endl;
    for (namespaces_semilattice_metadata_t<mock::dummy_protocol_t>::namespace_map_t::const_iterator i = cluster_metadata.dummy_namespaces.namespaces.begin(); i != cluster_metadata.dummy_namespaces.namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::cout << " " << i->first;
            if (long_format)
                std::cout << " " << i->second.get().name.get();
            std::cout << std::endl;
        }
    }
}

void rethinkdb_admin_app_t::list_memcached_namespaces(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::cout << "Memcached Namespaces:" << std::endl;
    for (namespaces_semilattice_metadata_t<memcached_protocol_t>::namespace_map_t::const_iterator i = cluster_metadata.memcached_namespaces.namespaces.begin(); i != cluster_metadata.memcached_namespaces.namespaces.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::cout << " " << i->first;
            if (long_format)
                std::cout << " " << i->second.get().name.get();
            std::cout << std::endl;
        }
    }
}

void rethinkdb_admin_app_t::list_machines(bool long_format, cluster_semilattice_metadata_t& cluster_metadata) {
    std::cout << "Machines:" << std::endl;
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator i = cluster_metadata.machines.machines.begin(); i != cluster_metadata.machines.machines.end(); ++i) {
        if (!i->second.is_deleted()) {
            std::cout << " " << i->first;
            if (long_format)
                std::cout << " " << i->second.get().name.get();
            std::cout << std::endl;
        }
    }
}

void rethinkdb_admin_app_t::do_admin_make_datacenter(command_data& data) {
    std::vector<std::string> obj_path;
    obj_path.push_back("datacenters");
    obj_path.push_back("new");

    if (data.params.count("name") == 0)
        set_metadata_value(obj_path, "{ }");
    else
        set_metadata_value(obj_path, "{ \"name\": \"" + data.params["name"][0] + "\" }");
}


void rethinkdb_admin_app_t::do_admin_make_namespace(command_data& data) {
    std::vector<std::string> obj_path;
    std::stringstream value;

    value << "{ \"name\": \"" + (data.params.count("name") == 0 ? std::string() : data.params["name"][0]) + "\"";

    std::string protocol = data.params["protocol"][0];
    if (protocol == "memcached")
        obj_path.push_back("memcached_namespaces");
    else if (protocol == "dummy")
        obj_path.push_back("dummy_namespaces");
    else
        throw admin_parse_exc_t("unknown protocol: " + protocol);

    value << ", \"port\": " << data.params["port"][0];

    if (data.params.count("primary") == 1)
        value << ", \"primary_uuid\": \"" << data.params["primary"][0] << "\"";

    obj_path.push_back("new");
    value << " }";

    set_metadata_value(obj_path, value.str());
}

void rethinkdb_admin_app_t::do_admin_move(command_data& data) {
    std::vector<std::string> target_path(get_path_from_id(data.params["id"][0]));
    std::string datacenter_id = data.params["datacenter"][0];
    std::vector<std::string> datacenter_path(get_path_from_id(datacenter_id));

    // Make sure the path is as expected
    if (!is_uuid(target_path[1]))
        throw admin_parse_exc_t("unexpected error when looking up destination: " + datacenter_id);

    // Target must be a datacenter in all existing use cases
    if (datacenter_path[0] != "datacenters")
        throw admin_parse_exc_t("destination is not a datacenter: " + datacenter_id);

    if (target_path[0] == "memcached_namespaces" || target_path[0] == "dummy_namespaces")
        target_path.push_back("primary_uuid");
    else if (target_path[0] == "machines")
        target_path.push_back("datacenter_uuid");
    else
        throw admin_parse_exc_t("");

    if (data.params.count("resolve") == 1)
        target_path.push_back("resolve");

    // Convert target name to uuid if necessary
    set_metadata_value(target_path, "\"" + datacenter_path[1] + "\""); // TODO: adding quotes like this is kind of silly - better way to get past the json parsing?
}

void rethinkdb_admin_app_t::do_admin_rename(command_data& data) {
    // TODO: make sure names aren't silly things like uuids or reserved strings
    std::vector<std::string> path(get_path_from_id(data.params["id"][0]));
    path.push_back("name");

    if (data.params.count("resolve") == 1)
        path.push_back("resolve");

    set_metadata_value(path, "\"" + data.params["new-name"][0] + "\""); // TODO: adding quotes like this is kind of silly - better way to get past the json parsing?
}

void rethinkdb_admin_app_t::do_admin_remove(command_data& data) {
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    namespace_metadata_ctx_t json_ctx(connectivity_cluster.get_me().get_uuid());
    std::vector<std::string> path(get_path_from_id(data.params["id"][0]));
    boost::shared_ptr<json_adapter_if_t<namespace_metadata_ctx_t> > json_adapter_head(traverse_directory(path, json_ctx, cluster_metadata));

    json_adapter_head->erase(json_ctx);

    try {
        fill_in_blueprints(&cluster_metadata);
    } catch (missing_machine_exc_t &e) { }

    semilattice_metadata->join(cluster_metadata);
}

void rethinkdb_admin_app_t::do_admin_help(command_data& data) {
    if (data.params.count("type") == 0) {
        std::cout << "available commands:" << std::endl;

        for (std::map<std::string, command_info *>::iterator i = command_descriptions.begin();
             i != command_descriptions.end(); ++i) {
            std::cout << "  " << i->first << std::endl;

            // TODO: this won't be complete for multiple levels of subcommands - make this more generic
            for (std::map<std::string, command_info *>::iterator j = i->second->subcommands.begin();
                 j != i->second->subcommands.end(); ++j) {
                std::cout << "    " << j->first << std::endl;
            }
        }
    } else {
        std::map<std::string, command_info *>::iterator i = command_descriptions.find(data.params["type"][0]);
        if (i != command_descriptions.end()) {
            // TODO: this won't work for multiple levels of subcommands - make this more generic
            if (data.params["type"].size() == 2) {
                std::map<std::string, command_info *>::iterator j = i->second->subcommands.find(data.params["type"][1]);
                if (j != i->second->subcommands.end())
                    std::cout << "  usage: " << j->first << " " << j->second->usage << std::endl;
                else
                    throw admin_parse_exc_t("unknown command: " + data.params["type"][0] + " " + data.params["type"][1]);
            } else
                std::cout << "  usage: " << i->first << " " << i->second->usage << std::endl;
        } else {
            throw admin_parse_exc_t("unknown command: " + data.params["type"][0]);
        }
    }
}

void rethinkdb_admin_app_t::set_metadata_value(const std::vector<std::string>& path, const std::string& value)
{
    cluster_semilattice_metadata_t cluster_metadata = semilattice_metadata->get();
    namespace_metadata_ctx_t json_ctx(sync_peer.get_uuid()); // Use peer rather than ourselves - TODO: race condition on sync?
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
