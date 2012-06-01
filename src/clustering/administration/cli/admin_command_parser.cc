#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/admin_cluster_link.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/runtime_utils.hpp"

#include <cstdarg>
#include <map>
#include <stdexcept>

#include "help.hpp"
#include "errors.hpp"

admin_command_parser_t *admin_command_parser_t::instance = NULL;
uint64_t admin_command_parser_t::cluster_join_timeout = 5000; // Give 5 seconds to connect to all machines in the cluster

const char *admin_command_parser_t::list_command = "ls";
const char *admin_command_parser_t::list_stats_command = "ls stats";
const char *admin_command_parser_t::list_issues_command = "ls issues";
const char *admin_command_parser_t::list_machines_command = "ls machines";
const char *admin_command_parser_t::list_directory_command = "ls directory";
const char *admin_command_parser_t::list_namespaces_command = "ls namespaces";
const char *admin_command_parser_t::list_datacenters_command = "ls datacenters";
const char *admin_command_parser_t::exit_command = "exit";
const char *admin_command_parser_t::help_command = "help";
const char *admin_command_parser_t::resolve_command = "resolve";
const char *admin_command_parser_t::pin_shard_command = "pin shard";
const char *admin_command_parser_t::split_shard_command = "split shard";
const char *admin_command_parser_t::merge_shard_command = "merge shard";
const char *admin_command_parser_t::set_name_command = "set name";
const char *admin_command_parser_t::set_acks_command = "set acks";
const char *admin_command_parser_t::set_replicas_command = "set replicas";
const char *admin_command_parser_t::set_datacenter_command = "set datacenter";
const char *admin_command_parser_t::create_namespace_command = "create namespace";
const char *admin_command_parser_t::create_datacenter_command = "create datacenter";
const char *admin_command_parser_t::remove_command = "rm";

// Special commands - used only in certain cases
const char *admin_command_parser_t::complete_command = "complete";

const char *admin_command_parser_t::exit_usage = "";
const char *admin_command_parser_t::list_usage = "[<id> | --long]";
const char *admin_command_parser_t::list_stats_usage = "[<machine>...] [<namespace>...]";
const char *admin_command_parser_t::list_issues_usage = "";
const char *admin_command_parser_t::list_machines_usage = "[--long]";
const char *admin_command_parser_t::list_directory_usage = "[--long]";
const char *admin_command_parser_t::list_namespaces_usage = "[--protocol <protocol>] [--long]";
const char *admin_command_parser_t::list_datacenters_usage = "[--long]";
const char *admin_command_parser_t::help_usage = "[ ls | create | rm | set | split | merge | resolve | help ]";
const char *admin_command_parser_t::resolve_usage = "<id> <field>";
const char *admin_command_parser_t::pin_shard_usage = "<namespace> <shard> [--master <machine>] [--replicas <machine>...]";
const char *admin_command_parser_t::split_shard_usage = "<namespace> <split-point>...";
const char *admin_command_parser_t::merge_shard_usage = "<namespace> <split-point>...";
const char *admin_command_parser_t::set_name_usage = "<id> <new name>";
const char *admin_command_parser_t::set_acks_usage = "<namespace> <datacenter> <num-acks>";
const char *admin_command_parser_t::set_replicas_usage = "<namespace> <datacenter> <num-replicas>";
const char *admin_command_parser_t::set_datacenter_usage = "( <namespace> | <machine> ) <datacenter>";
const char *admin_command_parser_t::create_namespace_usage = "<name> --port <port> --protocol ( memcached | dummy ) --primary <datacenter>";
const char *admin_command_parser_t::create_datacenter_usage = "<name>";
const char *admin_command_parser_t::remove_usage = "<id>...";

const char *admin_command_parser_t::list_description = "Print a list of objects in the cluster.  An individual object can be selected by name or uuid for a detailed description of the object.";
const char *admin_command_parser_t::list_stats_description = "Print a list of statistics gathered by the cluster.  Statistics will be on a per-machine and per-namespace basis, if applicable, and can be filtered by machine or namespace.";
const char *admin_command_parser_t::list_issues_description = "Print a list of issues currently detected by the cluster.";
const char *admin_command_parser_t::list_machines_description = "Print a list of machines in the cluster along with some relevant data about each machine.";
const char *admin_command_parser_t::list_directory_description = "Print a list of nodes currently connected to the running admin client, this may include data servers, proxy nodes, or other admin clients.";
const char *admin_command_parser_t::list_namespaces_description = "Print a list of namespaces in the cluster along with some relevant data about each namespace. The list may be filtered by a namespace protocol type.";
const char *admin_command_parser_t::list_datacenters_description = "Print a list of datacenters in the cluster along with some relevant data about each datacenter.";
const char *admin_command_parser_t::exit_description = "Quit the cluster administration console.";
const char *admin_command_parser_t::help_description = "Print help on a cluster administration command.";
const char *admin_command_parser_t::resolve_description = "If there are any conflicted values in the cluster, either list the possible values or resolve the conflict by selecting one of the values.";
const char *admin_command_parser_t::pin_shard_description = "Set machines to host the master and/or replicas for a given shard in a namespace.";
const char *admin_command_parser_t::split_shard_description = "Add a new shard to a namespace by creating a new split point.  This will subdivide a given shard into two shards at the specified key.";
const char *admin_command_parser_t::merge_shard_description = "Remove a shard from a namespace by deleting a split point.  This will merge the two shards on each side of the split point into one.";
const char *admin_command_parser_t::set_name_description = "Set the name of an object.  This object may be referred to by its existing name or its UUID.  An object may have only one name at a time.";
const char *admin_command_parser_t::set_acks_description = "Set how many replicas must acknowledge a write operation for it to succeed, for the given namespace and datacenter.";
const char *admin_command_parser_t::set_replicas_description = "Set the replica affinities of a namespace.  This represents the number of replicas that the namespace will have in each specified datacenter.";
const char *admin_command_parser_t::set_datacenter_description = "Set the primary datacenter of a namespace, or the datacenter that a machine belongs to.";
const char *admin_command_parser_t::create_namespace_description = "Create a new namespace with the given protocol.  The namespace's primary datacenter and listening port must be specified.";
const char *admin_command_parser_t::create_datacenter_description = "Create a new datacenter with the given name.  Machines and replicas may be assigned to the datacenter.";
const char *admin_command_parser_t::remove_description = "Remove an object from the cluster.";

std::vector<std::string> parse_line(const std::string& line) {
    std::vector<std::string> result;
    char quote = '\0';
    bool escape = false;
    std::string argument;

    for (size_t i = 0; i < line.length(); ++i) {
        if (escape) {
            // It was easier for me to understand the boolean logic like this, empty if statements intentional
            if (quote == '\0' && (line[i] == '\'' || line[i] == '\"' || line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n'));
            else if (quote != '\0' && line[i] == quote);
            else {
                argument += '\\';
            }
            argument += line[i];
            escape = false;
        } else if (line[i] == '\\') {
            escape = true;
        } else if (line[i] == '\'' || line[i] == '\"') {
            if (quote == '\0') {
                quote = line[i];
            } else if (quote == line[i]) {
                quote = '\0';
            } else {
                argument += line[i];
            }
        } else if (quote != '\0' || (line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n')) {
            argument += line[i];
        } else {
            if (!argument.empty())
                result.push_back(argument);
            argument.clear();
        }
    }

    if (quote != '\0') {
        throw admin_parse_exc_t("unmatched quote");
    }

    if (!argument.empty()) {
        result.push_back(argument);
    }

    return result;
}

void admin_command_parser_t::param_options::add_option(const char *term) {
    valid_options.insert(term);
}

void admin_command_parser_t::param_options::add_options(const char *term, ...) {
    va_list terms;
    va_start(terms, term);
    while (term != NULL) {
        add_option(term);
        term = va_arg(terms, const char *);
    }
    va_end(terms);
}

admin_command_parser_t::command_info::~command_info() {
    for(std::map<std::string, param_options *>::iterator i = flags.begin(); i != flags.end(); ++i) {
        delete i->second;
    }

    for(std::vector<param_options *>::iterator i = positionals.begin(); i != positionals.end(); ++i) {
        delete *i;
    }

    for(std::map<std::string, command_info *>::iterator i = subcommands.begin(); i != subcommands.end(); ++i) {
        delete i->second;
    }
}

admin_command_parser_t::param_options * admin_command_parser_t::command_info::add_flag(const std::string& name, int count, bool required)
{
    param_options *option = new param_options(name, count, required);
    flags.insert(std::make_pair(name, option));
    return option;
}

admin_command_parser_t::param_options * admin_command_parser_t::command_info::add_positional(const std::string& name, int count, bool required)
{
    param_options *option = new param_options(name, count, required);
    positionals.push_back(option);
    return option;
}

void admin_command_parser_t::do_usage_internal(const std::vector<admin_help_info_t>& helps,
                                               const std::vector<std::string>& options,
                                               const char * header,
                                               bool console) {
    const char *prefix = console ? "" : "rethinkdb admin [OPTIONS] ";
    Help_Pager *help = new Help_Pager();

    if (header != NULL) {
        if (console) help->pagef("%s\n\n", header);
        else         help->pagef("rethinkdb admin %s\n\n", header);
    }

    help->pagef("Usage:\n");
    for (size_t i = 0; i < helps.size(); ++i) {
        help->pagef("  %s%s %s\n", prefix, helps[i].command.c_str(), helps[i].usage.c_str());
    }

    if (!console) {
        help->pagef("\n"
                    "Clustering Options:\n"
#ifndef NDEBUG
                    "     --client-port <port>   debug only option, specify the local port to use when\n"
                    "                              connecting to the cluster\n"
#endif
                    "  -j,--join <host>:<port>   specify the host and cluster port of a node in the cluster\n"
                    "                              to join\n");
    }

    bool description_header_printed = false;
    for (size_t i = 0; i < helps.size(); ++i) {
        if (helps[i].description.c_str() == NULL) {
            continue;
        } if (description_header_printed == false) {
            help->pagef("\nDescription:\n");
            description_header_printed = true;
        }
        help->pagef("  %s%s %s\n    %s\n\n", prefix, helps[i].command.c_str(), helps[i].usage.c_str(), helps[i].description.c_str());
    }

    if (!options.empty()) {
        help->pagef("\n"
                    "Options:\n");
        for (size_t i = 0; i < options.size(); ++i) {
            help->pagef("  %s\n", options[i].c_str());
        }
    }

    delete help;
}

void admin_command_parser_t::do_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<std::string> options;
    helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
    helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
    helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
    helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
    helps.push_back(admin_help_info_t(list_command, list_usage, list_description));
    helps.push_back(admin_help_info_t(list_stats_command, list_stats_usage, list_stats_description));
    helps.push_back(admin_help_info_t(list_issues_command, list_issues_usage, list_issues_description));
    helps.push_back(admin_help_info_t(list_machines_command, list_machines_usage, list_machines_description));
    helps.push_back(admin_help_info_t(list_directory_command, list_directory_usage, list_directory_description));
    helps.push_back(admin_help_info_t(list_namespaces_command, list_namespaces_usage, list_namespaces_description));
    helps.push_back(admin_help_info_t(list_datacenters_command, list_datacenters_usage, list_datacenters_description));
    helps.push_back(admin_help_info_t(resolve_command, resolve_usage, resolve_description));
    helps.push_back(admin_help_info_t(split_shard_command, split_shard_usage, split_shard_description));
    helps.push_back(admin_help_info_t(merge_shard_command, merge_shard_usage, merge_shard_description));
    helps.push_back(admin_help_info_t(create_namespace_command, create_namespace_usage, create_namespace_description));
    helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
    helps.push_back(admin_help_info_t(remove_command, remove_usage, remove_description));
    helps.push_back(admin_help_info_t(help_command, help_usage, help_description));

    if (console) {
        helps.push_back(admin_help_info_t(exit_command, exit_usage, exit_description));
        do_usage_internal(helps, options, "rethinkdb admin console - access or modify cluster metadata", console);
    } else {
        do_usage_internal(helps, options, "- access or modify cluster metadata", console);
    }
}

admin_command_parser_t::admin_command_parser_t(const std::string& peer_string, const std::set<peer_address_t>& joins, int client_port, signal_t *_interruptor) :
    join_peer(peer_string),
    joins_param(joins),
    client_port_param(client_port),
    cluster(NULL),
    console_mode(false),
    interruptor(_interruptor)
{
    rassert(instance == NULL);
    instance = this;

    build_command_descriptions();
}

admin_command_parser_t::~admin_command_parser_t() {
    rassert(instance == this);
    instance = NULL;

    delete cluster;
    destroy_command_descriptions(commands);
}

void admin_command_parser_t::destroy_command_descriptions(std::map<std::string, command_info *>& cmd_map) {
    for (std::map<std::string, command_info *>::iterator i = cmd_map.begin(); i != cmd_map.end(); ++i) {
        if (!i->second->subcommands.empty()) {
            destroy_command_descriptions(i->second->subcommands);
        }
        delete i->second;
    }
    cmd_map.clear();
}

admin_command_parser_t::command_info * admin_command_parser_t::add_command(std::map<std::string, command_info *>& cmd_map,
                                                  const std::string& full_cmd,
                                                  const std::string& cmd,
                                                  const std::string& usage,
                                                  bool post_sync,
                                                  void (admin_cluster_link_t::* const fn)(command_data&)) {
    command_info *info = NULL;
    size_t space_index = cmd.find_first_of(" \t\r\n");

    // Parse out the beginning of the command, in case it's a multi-level command, and do the add recursively
    if (space_index != std::string::npos) {
        std::string fragment(cmd.substr(0, space_index));
        std::map<std::string, command_info *>::iterator i = cmd_map.find(fragment);

        if (i == cmd_map.end()) {
            i = cmd_map.insert(std::make_pair(fragment, new command_info(full_cmd, fragment, "", false, NULL))).first;
        }

        info = add_command(i->second->subcommands, full_cmd, cmd.substr(space_index + 1), usage, post_sync, fn);
    } else {
        std::map<std::string, command_info *>::iterator i = cmd_map.find(cmd);

        if (i == cmd_map.end()) {
            i = cmd_map.insert(std::make_pair(cmd, new command_info(full_cmd, cmd, usage, post_sync, fn))).first;
        } else {
            // This node already exists, but this command should overwrite the current values
            i->second->usage = usage;
            i->second->post_sync = post_sync;
            i->second->do_function = fn;
        }

        info = i->second;
    }

    return info;
}

void admin_command_parser_t::build_command_descriptions() {
    rassert(commands.empty());
    command_info *info = NULL;

    info = add_command(commands, pin_shard_command, pin_shard_command, pin_shard_usage, true, &admin_cluster_link_t::do_admin_pin_shard);
    info->add_positional("namespace", 1, true)->add_option("!namespace");
    info->add_positional("key", 1, true); // TODO: list possible shards
    info->add_flag("master", 1, false)->add_option("!machine");
    info->add_flag("replicas", -1, false)->add_option("!machine");

    info = add_command(commands, split_shard_command, split_shard_command, split_shard_usage, true, &admin_cluster_link_t::do_admin_split_shard);
    info->add_positional("namespace", 1, true)->add_option("!namespace");
    info->add_positional("split-points", -1, true);

    info = add_command(commands, merge_shard_command, merge_shard_command, merge_shard_usage, true, &admin_cluster_link_t::do_admin_merge_shard);
    info->add_positional("namespace", 1, true)->add_option("!namespace");
    info->add_positional("split-points", -1, true); // TODO: list possible shards

    info = add_command(commands, resolve_command, resolve_command, resolve_usage, true, &admin_cluster_link_t::do_admin_resolve);
    info->add_positional("id", 1, true)->add_option("!conflict");
    info->add_positional("field", 1, true); // TODO: list the conflicted fields in the previous id

    info = add_command(commands, set_name_command, set_name_command, set_name_usage, true, &admin_cluster_link_t::do_admin_set_name);
    info->add_positional("id", 1, true)->add_option("!id");
    info->add_positional("new-name", 1, true);

    info = add_command(commands, set_acks_command, set_acks_command, set_acks_usage, true, &admin_cluster_link_t::do_admin_set_acks);
    info->add_positional("namespace", 1, true)->add_option("!namespace");
    info->add_positional("datacenter", 1, true)->add_option("!datacenter");
    info->add_positional("num-acks", 1, true);

    info = add_command(commands, set_replicas_command, set_replicas_command, set_replicas_usage, true, &admin_cluster_link_t::do_admin_set_replicas);
    info->add_positional("namespace", 1, true)->add_option("!namespace");
    info->add_positional("datacenter", 1, true)->add_option("!datacenter");
    info->add_positional("num-replicas", 1, true);

    info = add_command(commands, set_datacenter_command, set_datacenter_command, set_datacenter_usage, true, &admin_cluster_link_t::do_admin_set_datacenter);
    info->add_positional("id", 1, true)->add_options("!machine", "!namespace", NULL);
    info->add_positional("datacenter", 1, true)->add_option("!datacenter");

    info = add_command(commands, list_command, list_command, list_usage, false, &admin_cluster_link_t::do_admin_list);
    info->add_positional("object", 1, false)->add_options("!id", NULL);
    info->add_flag("long", 0, false);

    info = add_command(commands, list_stats_command, list_stats_command, list_stats_usage, false, &admin_cluster_link_t::do_admin_list_stats);
    info->add_positional("id-filter", -1, false)->add_options("!machine", "!namespace", NULL);

    info = add_command(commands, list_issues_command, list_issues_command, list_issues_usage, false, &admin_cluster_link_t::do_admin_list_issues);

    info = add_command(commands, list_machines_command, list_machines_command, list_machines_usage, false, &admin_cluster_link_t::do_admin_list_machines);
    info->add_flag("long", 0, false);

    info = add_command(commands, list_directory_command, list_directory_command, list_directory_usage, false, &admin_cluster_link_t::do_admin_list_directory);
    info->add_flag("long", 0, false);

    info = add_command(commands, list_namespaces_command, list_namespaces_command, list_namespaces_usage, false, &admin_cluster_link_t::do_admin_list_namespaces);
    info->add_flag("protocol", 1, false)->add_options("memcached", "dummy", NULL);
    info->add_flag("long", 0, false);

    info = add_command(commands, list_datacenters_command, list_datacenters_command, list_datacenters_usage, false, &admin_cluster_link_t::do_admin_list_datacenters);
    info->add_flag("long", 0, false);

    info = add_command(commands, create_namespace_command, create_namespace_command, create_namespace_usage, true, &admin_cluster_link_t::do_admin_create_namespace);
    info->add_positional("name", 1, true);
    info->add_flag("protocol", 1, true)->add_options("memcached", "dummy", NULL);
    info->add_flag("primary", 1, true)->add_option("!datacenter");
    info->add_flag("port", 1, true);

    info = add_command(commands, create_datacenter_command, create_datacenter_command, create_datacenter_usage, true, &admin_cluster_link_t::do_admin_create_datacenter);
    info->add_positional("name", 1, true);

    info = add_command(commands, remove_command, remove_command, remove_usage, true, &admin_cluster_link_t::do_admin_remove);
    info->add_positional("id", -1, true)->add_option("!id");

    info = add_command(commands, help_command, help_command, help_usage, false, NULL); // Special case, 'help' is not done through the cluster
    info->add_positional("command", 1, false)->add_options("split", "merge", "set", "ls", "create", "rm", "resolve", "help", NULL);
    info->add_positional("subcommand", 1, false);
}

admin_cluster_link_t * admin_command_parser_t::get_cluster() {
    if (cluster == NULL) {
        if (joins_param.empty()) {
            throw admin_no_connection_exc_t("no join parameter specified");
        }
        cluster = new admin_cluster_link_t(joins_param, client_port_param, interruptor);

        // Spin for some time, trying to connect to the whole cluster
        for (uint64_t time_waited = 0; time_waited < cluster_join_timeout &&
             (cluster->machine_count() > cluster->available_machine_count() ||
              cluster->available_machine_count() == 0); time_waited += 100) {
            nap(100);
        }

        if (console_mode) {
            cluster->sync_from();
            size_t machine_count = cluster->machine_count();
            fprintf(stdout, "Connected to cluster with %ld machine%s, run 'help' for more information\n", machine_count, machine_count > 1 ? "s" : "");
            size_t num_issues = cluster->issue_count();
            if (num_issues > 0) {
                fprintf(stdout, "There %s %ld outstanding issue%s, run 'ls issues' for more information\n", num_issues > 1 ? "are" : "is", num_issues, num_issues > 1 ? "s" : "");
            }
        }
    }

    return cluster;
}


admin_command_parser_t::command_info * admin_command_parser_t::find_command(const std::map<std::string, admin_command_parser_t::command_info *>& cmd_map, const std::vector<std::string>& line, size_t& index) {
    std::map<std::string, command_info *>::const_iterator i = cmd_map.find(line[0]);

    if (i == commands.end()) {
        return NULL;
    } else {
        // If any subcommands exist, either one is selected by the next substring, or we're at the final command
        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info *>::iterator temp = i->second->subcommands.find(line[index]);
            if (temp == i->second->subcommands.end()) {
                break;
            }
            i = temp;
        }
    }

    return i->second;
}

admin_command_parser_t::command_data admin_command_parser_t::parse_command(command_info *info, const std::vector<std::string>& line)
{
    command_data data(info);
    size_t positional_index = 0;
    size_t positional_count = 0;

    // Parse out options and parameters
    for (size_t index = 0; index < line.size(); ++index) {
        if (line[index].find("--") == 0) { // If the argument starts with a "--", it must be a flag option
            std::map<std::string, param_options *>::iterator opt_iter = info->flags.find(line[index].substr(2));
            if (opt_iter != info->flags.end()) {
                param_options *option = opt_iter->second;
                if (option->count == 0) {  // Flag only
                    data.params[option->name] = std::vector<std::string>();
                } else {
                    size_t remaining = option->count;
                    if (index + remaining >= line.size()) {
                        throw admin_parse_exc_t("not enough arguments provided for flag: " + line[index]);
                    }

                    while(remaining > 0 && index < line.size() - 1) {
                        ++index;
                        --remaining;
                        if (line[index].find("--") == 0) {
                            throw admin_parse_exc_t("not enough arguments provided for flag: " + line[index]);
                        }
                        data.params[option->name].push_back(line[index]);
                    }
                }
            } else {
                throw admin_parse_exc_t("unrecognized flag: " + line[index]);
            }
        } else { // Otherwise, it must be a positional
            if (positional_index + 1 > info->positionals.size()) {
                throw admin_parse_exc_t("too many positional arguments");
            }
            data.params[info->positionals[positional_index]->name].push_back(line[index]);
            if (++positional_count >= info->positionals[positional_index]->count) {
                positional_count = 0;
                ++positional_index;
            }
        }
    }

    // Make sure all required options have been provided
    for (size_t j = 0; j < info->positionals.size(); ++j) {
        if (info->positionals[j]->required && data.params.find(info->positionals[j]->name) == data.params.end()) {
            throw admin_parse_exc_t("insufficient positional parameters provided");
        }
    }

    for (std::map<std::string, param_options *>::iterator j = info->flags.begin(); j != info->flags.end(); ++j) {
        if (j->second->required && data.params.find(j->second->name) == data.params.end()) {
            throw admin_parse_exc_t("required flag not specified: " + j->second->name);
        }
    }

    return data;
}

void admin_command_parser_t::run_command(command_data& data) {
    // Special cases for help and join, which do nothing through the cluster
    if (data.info->command == "help") {
        do_admin_help(data);
    } else if (data.info->do_function == NULL) {
        throw admin_parse_exc_t("incomplete command (should have been caught earlier, though)");
    } else {
        get_cluster()->sync_from();

        // Retry until the command completes, this is necessary if multiple sources modify the same vclock
        while (true) {
            try {
                (get_cluster()->*(data.info->do_function))(data);
                break;
            } catch (admin_retry_exc_t& ex) {
                // Do nothing, command must be retried
            }
        }

        if (data.info->post_sync) {
            get_cluster()->sync_from();
        }
    }
}

void admin_command_parser_t::completion_generator_hook(const char *raw, linenoiseCompletions *completions) {
    if (instance == NULL) {
        logERR("linenoise completion generator called without an instance of admin_command_parser_t");
    } else {
        bool partial = !linenoiseIsUnescapedSpace(raw, strlen(raw) - 1);
        try {
            std::vector<std::string> line = parse_line(raw);
            instance->completion_generator(line, completions, partial);
        } catch(std::exception& ex) {
            // Do nothing - if the line can't be parsed or completions failed, we just can't complete the line
        }
    }
}

std::map<std::string, admin_command_parser_t::command_info *>::const_iterator admin_command_parser_t::find_command_with_completion(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches) {
    std::map<std::string, command_info *>::const_iterator i = commands.find(str);
    if (add_matches) {
        if (i == commands.end()) {
            for (i = commands.lower_bound(str); i != commands.end() && i->first.find(str) == 0; ++i) {
                linenoiseAddCompletion(completions, i->first.c_str());
            }
        } else {
            linenoiseAddCompletion(completions, i->first.c_str());
        }

        return commands.end();
    }
    return i;
}

void admin_command_parser_t::add_option_matches(const param_options *option, const std::string& partial, linenoiseCompletions *completions) {

    if (partial.find("!") == 0) {
        // Don't allow completions beginning with '!', as we use it as a special case
        return;
    }

    for (std::set<std::string>::iterator i = option->valid_options.lower_bound(partial); i != option->valid_options.end() && i->find(partial) == 0; ++i) {
        if (i->find("!") != 0) {
            // skip special cases
            linenoiseAddCompletion(completions, i->c_str());
        }
    }

    std::vector<std::string> ids;

    // !id and !conflict are mutually exclusive will all patterns, others can be combined
    if (option->valid_options.count("!id") > 0) {
        ids = get_cluster()->get_ids(partial);
    } else if (option->valid_options.count("!conflict") > 0) {
        ids = get_cluster()->get_conflicted_ids(partial);
    } else { // TODO: any way to make uuids show before names?
        if (option->valid_options.count("!datacenter") > 0) {
            std::vector<std::string> delta = get_cluster()->get_datacenter_ids(partial);
            std::copy(delta.begin(), delta.end(), std::back_inserter(ids));
        }
        if (option->valid_options.count("!machine") > 0) {
            std::vector<std::string> delta = get_cluster()->get_machine_ids(partial);
            std::copy(delta.begin(), delta.end(), std::back_inserter(ids));
        }
        if (option->valid_options.count("!namespace") > 0) {
            std::vector<std::string> delta = get_cluster()->get_namespace_ids(partial);
            std::copy(delta.begin(), delta.end(), std::back_inserter(ids));
        }
    }

    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); ++i) {
        linenoiseAddCompletion(completions, i->c_str());
    }
}

void admin_command_parser_t::add_positional_matches(const command_info *info, size_t offset, const std::string& partial, linenoiseCompletions *completions) {
    if (info->positionals.size() > offset) {
        add_option_matches(info->positionals[offset], partial, completions);
    }
}

void admin_command_parser_t::completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial) {
    // TODO: this function is too long, too complicated, and is probably redundant in some cases
    //   I'm sure it can be simplified, but for now it seems to work

    if (line.empty()) {
        // No command specified, available completions are all basic commands
        for (std::map<std::string, command_info *>::iterator i = commands.begin(); i != commands.end(); ++i) {
            linenoiseAddCompletion(completions, i->first.c_str());
        }
    } else {
        // Command specified, find a matching command/subcommand
        size_t index = 0;
        std::map<std::string, command_info *>::const_iterator i = commands.find(line[0]);
        std::map<std::string, command_info *>::const_iterator end = commands.end();

        i = find_command_with_completion(commands, line[index], completions, partial && line.size() == 1);
        if (i == end) {
            return;
        }

        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info *>::const_iterator old = i;
            i = find_command_with_completion(i->second->subcommands, line[index], completions, partial && line.size() == index + 1);
            if (i == old->second->subcommands.end()) {
                i = old;
                break;
            }
        }

        command_info *cmd = i->second;

        if (index == line.size() && !cmd->subcommands.empty()) {
            // We're at the end of the line, and there are still more subcommands
            find_command_with_completion(cmd->subcommands, std::string(), completions, !partial);
        }

        // Non-terminal commands cannot have arguments
        if (cmd->do_function == NULL) {
            return;
        }

        if (index == line.size()) {
            if (!partial) {
                // Only the command specified, show positionals
                add_positional_matches(cmd, 0, std::string(), completions);
            } else {
                // command specified with no space at the end, throw one on to show completion
                linenoiseAddCompletion(completions, line[index - 1].c_str());
            }
            return;
        }

        // We are now at a terminal command, figure out which parameter we're on
        size_t positional_index = 0;
        size_t positional_count = 0;

        for (; index < line.size(); ++index) {
            if (line[index].find("--") == 0) { // This is a flag, skip any completed options
                std::map<std::string, param_options *>::iterator opt_iter = cmd->flags.find(line[index].substr(2));
                if (opt_iter != cmd->flags.end() && (index != line.size() - 1 || !partial)) {
                    if (line.size() <= index + opt_iter->second->count) { // Not enough params for this flag
                        if (!partial) {
                            // print valid_options
                            add_option_matches(opt_iter->second, std::string(), completions);
                        } else {
                            // find matches of last word with valid_options
                            add_option_matches(opt_iter->second, line[line.size() - 1], completions);
                        }
                        return;
                    } else if (line.size() == index + opt_iter->second->count + 1) {
                        if (!partial) {
                            add_positional_matches(cmd, positional_index, std::string(), completions);
                        } else {
                            // find matches of last word with valid_options
                            add_option_matches(opt_iter->second, line[line.size() - 1], completions);
                        }
                        return;
                    } else {
                        index += opt_iter->second->count;
                    }
                } else if (line.size() == index + 1) {
                    if (partial) {
                        for (opt_iter = cmd->flags.lower_bound(line[index].substr(2));
                             opt_iter != cmd->flags.end() && opt_iter->first.find(line[index].substr(2)) == 0; ++opt_iter) {
                            linenoiseAddCompletion(completions, ("--" + opt_iter->first).c_str());
                        }
                    } else {
                        add_positional_matches(cmd, positional_index, std::string(), completions);
                    }
                    return;
                }

            } else { // this is a positional, attempt to advance
                // We're at the end of the line, handle using the current positional
                if (index == line.size() - 1) {
                    if (!partial) {
                        add_positional_matches(cmd, positional_index + 1, std::string(), completions);
                    } else {
                        add_positional_matches(cmd, positional_index, line[line.size() - 1], completions);
                    }
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

void admin_command_parser_t::run_completion(const std::vector<std::string>& command_args) {
    linenoiseCompletions completions = { 0, NULL };

    if (command_args.size() < 2 || (command_args[1] != "partial" && command_args[1] != "full")) {
        throw admin_parse_exc_t("usage: rethinkdb admin complete ( partial | full ) [...]");
    }

    completion_generator(std::vector<std::string>(command_args.begin() + 2, command_args.end()), &completions, command_args[1] == "partial");

    for (size_t i = 0; i < completions.len; ++i) {
        printf("%s\n", completions.cvec[i]);
    }

    linenoiseFreeCompletions(&completions);
}

std::string admin_command_parser_t::get_subcommands_usage(command_info *info) {
    std::string result;

    if (info->do_function != NULL) {
        result += "\t" + info->full_command + " " + info->usage + "\n";
    }

    if (!info->subcommands.empty()) {
        for (std::map<std::string, command_info *>::const_iterator i = info->subcommands.begin(); i != info->subcommands.end(); ++i) {
            result += get_subcommands_usage(i->second);
        }
    }

    return result;
}

void admin_command_parser_t::parse_and_run_command(const std::vector<std::string>& line) {
    command_info *info = NULL;

    try {
        size_t index;
        info = find_command(commands, line, index);

        if (info == NULL) {
            throw admin_parse_exc_t("unknown command: " + line[0]);
        } else if (info->do_function == NULL && info->full_command != help_command) {
            throw admin_parse_exc_t("incomplete command");
        }

        command_data data(parse_command(info, std::vector<std::string>(line.begin() + index, line.end())));
        run_command(data);
    } catch (admin_parse_exc_t& ex) {
        std::string exception_str(ex.what());
        if (info != NULL) {
            // Cut off the trailing newline from get_subcommand_usage
            std::string usage_str(get_subcommands_usage(info));
            exception_str += "\nusage: " + usage_str.substr(0, usage_str.size() - 1);
        }
        throw admin_parse_exc_t(exception_str);
    } catch (std::exception& ex) {
        throw;
    }
}

class linenoiseCallable {
public:
    linenoiseCallable(const char* _prompt, char** _raw_line_ptr) : prompt(_prompt), raw_line_ptr(_raw_line_ptr) { }
    const char* prompt;
    char** raw_line_ptr;

    void operator () () const { *raw_line_ptr = linenoise(prompt); }
};

void admin_command_parser_t::run_console(bool exit_on_failure) {
    // Can only run console mode with a connection, try to get one
    console_mode = true;
    get_cluster();

    char prompt[64];
    char* raw_line;
    linenoiseCallable linenoise_blocker(prompt, &raw_line);
    std::string line;

    // Build the prompt based on our initial join command
    std::string join_peer_truncated(join_peer);
    if (join_peer.length() > 50) {
        join_peer_truncated = join_peer.substr(0, 47) + "...";
    }
    snprintf(prompt, 64, "%s> ", join_peer_truncated.c_str());

    linenoiseSetCompletionCallback(completion_generator_hook);
    thread_pool_t::run_in_blocker_pool(linenoise_blocker);

    while(raw_line != NULL) {
        line.assign(raw_line);

        if (line == exit_command) {
            break;
        }

        if (!line.empty()) {

            try {
                std::vector<std::string> split_line = parse_line(line);
                if (!split_line.empty()) {
                    parse_and_run_command(split_line);
                }
            } catch (admin_no_connection_exc_t& ex) {
                free(raw_line);
                console_mode = false;
                throw admin_cluster_exc_t("fatal error: connection to cluster failed");
            } catch (std::exception& ex) {
                if (exit_on_failure) {
                    free(raw_line);
                    console_mode = false;
                    throw;
                } else {
                    fprintf(stderr, "%s\n", ex.what());
                }
            }

            linenoiseHistoryAdd(raw_line);
        }

        free(raw_line);
        thread_pool_t::run_in_blocker_pool(linenoise_blocker);
    }
    console_mode = false;
}

void admin_command_parser_t::do_admin_help(command_data& data) {
    if (data.params.count("command") == 1) {
        std::string command = data.params["command"][0];
        std::string subcommand = (data.params.count("subcommand") > 0 ? data.params["subcommand"][0] : "");
        std::vector<admin_help_info_t> helps;
        std::vector<std::string> options;

        // TODO: add option descriptions
        if (command == "ls") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(list_command, list_usage, list_description));
                helps.push_back(admin_help_info_t(list_stats_command, list_stats_usage, list_stats_description));
                helps.push_back(admin_help_info_t(list_issues_command, list_issues_usage, list_issues_description));
                helps.push_back(admin_help_info_t(list_machines_command, list_machines_usage, list_machines_description));
                helps.push_back(admin_help_info_t(list_directory_command, list_directory_usage, list_directory_description));
                helps.push_back(admin_help_info_t(list_namespaces_command, list_namespaces_usage, list_namespaces_description));
                helps.push_back(admin_help_info_t(list_datacenters_command, list_datacenters_usage, list_datacenters_description));
                do_usage_internal(helps, options, "ls - display information from the cluster", console_mode);
            } else if (subcommand == "stats") {
                helps.push_back(admin_help_info_t(list_stats_command, list_stats_usage, list_stats_description));
                do_usage_internal(helps, options, "ls stats - display statistics gathered from the cluster", console_mode);
            } else if (subcommand == "issues") {
                helps.push_back(admin_help_info_t(list_issues_command, list_issues_usage, list_issues_description));
                do_usage_internal(helps, options, "ls issues - display current issues detected by the cluster", console_mode);
            } else if (subcommand == "machines") {
                helps.push_back(admin_help_info_t(list_machines_command, list_machines_usage, list_machines_description));
                do_usage_internal(helps, options, "ls machines - display a list of machines in the cluster", console_mode);
            } else if (subcommand == "directory") {
                helps.push_back(admin_help_info_t(list_directory_command, list_directory_usage, list_directory_description));
                do_usage_internal(helps, options, "ls directory - display a list of nodes connected to the cluster", console_mode);
            } else if (subcommand == "namespaces") {
                helps.push_back(admin_help_info_t(list_namespaces_command, list_namespaces_usage, list_namespaces_description));
                do_usage_internal(helps, options, "ls namespaces - display a list of namespaces in the cluster", console_mode);
            } else if (subcommand == "datacenters") {
                helps.push_back(admin_help_info_t(list_datacenters_command, list_datacenters_usage, list_datacenters_description));
                do_usage_internal(helps, options, "ls datacenters - display a list of datacenters in the cluster", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "set") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
                helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
                helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
                helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
                do_usage_internal(helps, options, "set - change the value of a field in cluster metadata", console_mode);
            } else if (subcommand == "name") {
                helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
                do_usage_internal(helps, options, "set name - change the name of an object in the cluster", console_mode);
            } else if (subcommand == "acks") {
                helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
                do_usage_internal(helps, options, "set acks - change the number of acknowledgements required for a write operation to succeed", console_mode);
            } else if (subcommand == "replicas") {
                helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
                do_usage_internal(helps, options, "set replicas - change the number of replicas for a namespace in a datacenter", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
                do_usage_internal(helps, options, "set datacenter - change the primary datacenter for a namespace or machine", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "create") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(create_namespace_command, create_namespace_usage, create_namespace_description));
                helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
                do_usage_internal(helps, options, "create - add a new namespace or datacenter to the cluster", console_mode);
            } else if (subcommand == "namespace") {
                helps.push_back(admin_help_info_t(create_namespace_command, create_namespace_usage, create_namespace_description));
                do_usage_internal(helps, options, "create namespace - add a new namespace to the cluster", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
                do_usage_internal(helps, options, "create datacenter - add a new datacenter to the cluster", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "rm") {
            if (!subcommand.empty()) {
                throw admin_parse_exc_t("no recognized subcommands for 'rm'");
            }
            helps.push_back(admin_help_info_t(remove_command, remove_usage, remove_description));
            do_usage_internal(helps, options, "remove - delete an object from the cluster metadata", console_mode);
        } else if (command == "help") {
            if (!subcommand.empty()) {
                throw admin_parse_exc_t("no recognized subcommands for 'help'");
            }
            helps.push_back(admin_help_info_t(help_command, help_usage, help_description));
            do_usage_internal(helps, options, "help - provide information about a cluster administration command", console_mode);
        } else if (command == "pin") {
            if (!subcommand.empty() || subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(pin_shard_command, pin_shard_usage, pin_shard_description));
            do_usage_internal(helps, options, "pin shard - assign machines to host the master or replicas of a shard", console_mode);
        } else if (command == "split") {
            if (!subcommand.empty() || subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(split_shard_command, split_shard_usage, split_shard_description));
            do_usage_internal(helps, options, "split shard - split a shard in a namespace into more shards", console_mode);
        } else if (command == "merge") {
            if (!subcommand.empty() || subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(merge_shard_command, merge_shard_usage, merge_shard_description));
            do_usage_internal(helps, options, "merge shard - merge two or more shards in a namespace", console_mode);
        } else if (command == "resolve") {
            if (!subcommand.empty()) {
                throw admin_parse_exc_t("no recognized subcommands for 'resolve'");
            }
            helps.push_back(admin_help_info_t(resolve_command, resolve_usage, resolve_description));
            do_usage_internal(helps, options, "resolve - resolve a conflict on a cluster metadata value", console_mode);
        } else {
            throw admin_parse_exc_t("unknown command: " + command);
        }
    } else {
        do_usage(console_mode);
    }
}

