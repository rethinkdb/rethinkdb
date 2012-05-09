#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <cstdarg>
#include <map>
#include <stdexcept>
#include <iostream>

#include "help.hpp"
#include "errors.hpp"

#include <boost/program_options.hpp>

// TODO: make a useful prompt
const char *prompt = " > ";

admin_command_parser_t *admin_command_parser_t::instance = NULL;

const char *admin_command_parser_t::list_command = "ls";
const char *admin_command_parser_t::exit_command = "exit";
const char *admin_command_parser_t::help_command = "help";
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
const char *admin_command_parser_t::join_command = "join";
const char *admin_command_parser_t::complete_command = "complete";

const char *admin_command_parser_t::exit_usage = "";
const char *admin_command_parser_t::list_usage = "[ datacenters | namespaces [--protocol <protocol>] | machines | issues | <id> ] [--long]";
const char *admin_command_parser_t::help_usage = "[ ls | create | rm | set | rename | help ]";
const char *admin_command_parser_t::split_shard_usage = "<namespace> <split-point>...";
const char *admin_command_parser_t::merge_shard_usage = "<namespace> <split-point>...";
const char *admin_command_parser_t::set_name_usage = "<id> <new name> [--resolve]";
const char *admin_command_parser_t::set_acks_usage = "<namespace> <datacenter> <num-acks> [--resolve]";
const char *admin_command_parser_t::set_replicas_usage = "<namespace> <datacenter> <num-replicas> [--resolve]";
const char *admin_command_parser_t::set_datacenter_usage = "( <namespace> | <machine> ) <datacenter> [--resolve]";
const char *admin_command_parser_t::create_namespace_usage = "[--port <port>] [--protocol ( memcached | dummy ) ] [--primary <datacenter>] [--name <name>]";
const char *admin_command_parser_t::create_datacenter_usage = "[--name <name>]";
const char *admin_command_parser_t::remove_usage = "<id>...";
const char *admin_command_parser_t::join_usage = "<host>:<port>";

const char *admin_command_parser_t::list_description = "Print a list of objects in the cluster.  If a type of object is specified, only objects of that type are listed.  An individual object can be selected by name or uuid.";
const char *admin_command_parser_t::exit_description = "Quit the cluster administration console.";
const char *admin_command_parser_t::help_description = "Print help on a cluster administration command.";
const char *admin_command_parser_t::split_shard_description = "Add a new shard to a namespace by creating a new split point.  This will subdivide a given shard into two shards at the specified key.";
const char *admin_command_parser_t::merge_shard_description = "Remove a shard from a namespace by deleting a split point.  This will merge the two shards on each side of the split point into one.";
const char *admin_command_parser_t::set_name_description = "Set the name of an object.  This object may be referred to by its existing name or its UUID.  An object may have only one name at a time.";
const char *admin_command_parser_t::set_acks_description = "NYI";
const char *admin_command_parser_t::set_replicas_description = "Set the replica affinities of a namespace.  This represents the number of replicas that the namespace will have in each specified datacenter.";
const char *admin_command_parser_t::set_datacenter_description = "Set the primary datacenter of a namespace, or the datacenter that a machine belongs to.";
const char *admin_command_parser_t::create_namespace_description = "Create a new namespace.  ";
const char *admin_command_parser_t::create_datacenter_description = "Create a new datacenter with the given name, if specified.  Once this is done, machines then replicas may be assigned to the datacenter.";
const char *admin_command_parser_t::remove_description = "Remove an object from the cluster.";
const char *admin_command_parser_t::join_description = "Join a cluster so that administration commands may be issued.";

namespace po = boost::program_options;

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
    for(std::map<std::string, param_options *>::iterator i = flags.begin(); i != flags.end(); ++i)
        delete i->second;

    for(std::vector<param_options *>::iterator i = positionals.begin(); i != positionals.end(); ++i)
        delete *i;

    for(std::map<std::string, command_info *>::iterator i = subcommands.begin(); i != subcommands.end(); ++i)
        delete i->second;
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

void admin_command_parser_t::command_info::add_subcommand(command_info *info)
{
    rassert(subcommands.count(info->command) == 0);
    subcommands.insert(std::make_pair(info->command, info));
}

void admin_command_parser_t::do_usage_internal(const std::vector<admin_help_info_t>& helps,
                                               const std::vector<const char *>& options,
                                               const char * header,
                                               bool console) {
    const char *prefix = console ? "" : "rethinkdb admin [OPTIONS] ";
    Help_Pager *help = new Help_Pager();

    if (header != NULL) {
        if (console) help->pagef("%s\n\n", header);
        else         help->pagef("rethinkdb admin %s\n\n", header);
    }

    help->pagef("Usage:\n");
    for (size_t i = 0; i < helps.size(); ++i)
        help->pagef("  %s%s %s\n", prefix, helps[i].command.c_str(), helps[i].usage.c_str());

    if (!console)
        help->pagef("\n"
                    "Clustering Options:\n"
#ifndef NDEBUG
                    "     --client-port <port>   debug only option, specify the local port to use when\n"
                    "                              connecting to the cluster\n"
#endif
                    "  -j,--join <host>:<port>   specify the host and cluster port of a node in the cluster\n"
                    "                              to join\n");

    bool description_header_printed = false;
    for (size_t i = 0; i < helps.size(); ++i) {
        if (helps[i].description.c_str() == NULL)
            continue;
        if (description_header_printed == false) {
            help->pagef("\nDescription:\n");
            description_header_printed = true;
        }
        help->pagef("  %s%s %s\n    %s\n\n", prefix, helps[i].command.c_str(), helps[i].usage.c_str(), helps[i].description.c_str());
    }

    if (!options.empty()) {
        help->pagef("\n"
                    "Options:\n");
        for (size_t i = 0; i < options.size(); ++i)
            help->pagef("  %s\n", options[i]);
    }

    delete help;
}

void admin_command_parser_t::do_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
    helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
    helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
    helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
    helps.push_back(admin_help_info_t(list_command, list_usage, list_description));
    helps.push_back(admin_help_info_t(split_shard_command, split_shard_usage, split_shard_description));
    helps.push_back(admin_help_info_t(merge_shard_command, merge_shard_usage, merge_shard_description));
    helps.push_back(admin_help_info_t(create_namespace_command, create_namespace_usage, create_namespace_description));
    helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
    helps.push_back(admin_help_info_t(remove_command, remove_usage, remove_description));
    helps.push_back(admin_help_info_t(help_command, help_usage, help_description));

    if (console) {
        helps.push_back(admin_help_info_t(join_command, join_usage, join_description));
        helps.push_back(admin_help_info_t(exit_command, exit_usage, exit_description));
        do_usage_internal(helps, options, "rethinkdb admin console - access or modify cluster metadata", console);
    } else
        do_usage_internal(helps, options, "- access or modify cluster metadata", console);
}

void admin_command_parser_t::do_set_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
    helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
    helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
    helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "set - change the value of a field in cluster metadata, and resolve conflicts", console);
}

void admin_command_parser_t::do_create_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(create_namespace_command, create_namespace_usage, create_namespace_description));
    helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "create - create a namespace or datacenter in the cluster", console);
}

void admin_command_parser_t::do_remove_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(remove_command, remove_usage, remove_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "remove - delete an object from the cluster metadata", console);
}

void admin_command_parser_t::do_join_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(join_command, join_usage, join_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "join - join an existing cluster to issue commands", console);
}

void admin_command_parser_t::do_merge_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(merge_shard_command, merge_shard_usage, merge_shard_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "merge - merge two or more shards in a namespace", console);
}

void admin_command_parser_t::do_list_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(list_command, list_usage, list_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "list - display information from the cluster", console);
}

void admin_command_parser_t::do_help_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(help_command, help_usage, help_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "help - provide information about a cluster administration command", console);
}

void admin_command_parser_t::do_split_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<const char *> options;
    helps.push_back(admin_help_info_t(split_shard_command, split_shard_usage, split_shard_description));
    // TODO: add option descriptions
    do_usage_internal(helps, options, "split - split a shard in a namespace into more shards", console);
}

admin_command_parser_t::admin_command_parser_t(const std::set<peer_address_t>& joins, int client_port) :
    joins_param(joins),
    client_port_param(client_port),
    cluster(NULL),
    console_mode(false)
{
    rassert(instance == NULL);
    instance = this;

    build_command_descriptions();
}

admin_command_parser_t::~admin_command_parser_t() {
    rassert(instance == this);
    instance = NULL;

    delete cluster;
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

        if (i == cmd_map.end())
            i = cmd_map.insert(std::make_pair(fragment, new command_info(full_cmd, fragment, "", false, NULL))).first;

        info = add_command(i->second->subcommands, full_cmd, cmd.substr(space_index + 1), usage, post_sync, fn);
    } else {
        info = new command_info(full_cmd, cmd, usage, post_sync, fn);
        cmd_map.insert(std::make_pair(info->command, info));
    }

    return info;
}

void admin_command_parser_t::build_command_descriptions() {
    rassert(commands.empty());
    command_info *info = NULL;

    info = add_command(commands, split_shard_command, split_shard_command, split_shard_usage, true, &admin_cluster_link_t::do_admin_split_shard);
    info->add_positional("namespace", 1, true)->add_option("!id");
    info->add_positional("split-points", -1, true);

    info = add_command(commands, merge_shard_command, merge_shard_command, merge_shard_usage, true, &admin_cluster_link_t::do_admin_merge_shard);
    info->add_positional("namespace", 1, true)->add_option("!id");
    info->add_positional("split-points", -1, true);

    info = add_command(commands, set_name_command, set_name_command, set_name_usage, true, &admin_cluster_link_t::do_admin_set_name);
    info->add_positional("id", 1, true)->add_option("!id");
    info->add_positional("new-name", 1, true);
    info->add_flag("resolve", 0, false);

    info = add_command(commands, set_acks_command, set_acks_command, set_acks_usage, true, &admin_cluster_link_t::do_admin_set_acks);
    info->add_positional("namespace", 1, true)->add_option("!id"); // TODO: restrict this to namespaces only
    info->add_positional("datacenter", 1, true)->add_option("!id"); // TODO: restrict this to datacenters only
    info->add_positional("num-acks", 1, true);
    info->add_flag("resolve", 0, false);

    info = add_command(commands, set_replicas_command, set_replicas_command, set_replicas_usage, true, &admin_cluster_link_t::do_admin_set_replicas);
    info->add_positional("namespace", 1, true)->add_option("!id"); // TODO: restrict this to namespaces only
    info->add_positional("datacenter", 1, true)->add_option("!id"); // TODO: restrict this to datacenters only
    info->add_positional("num-replicas", 1, true);
    info->add_flag("resolve", 0, false);

    info = add_command(commands, set_datacenter_command, set_datacenter_command, set_datacenter_usage, true, &admin_cluster_link_t::do_admin_set_datacenter);
    info->add_positional("id", 1, true)->add_option("!id"); // TODO: restrict this to machines and namespaces only
    info->add_positional("datacenter", 1, true)->add_option("!id"); // TODO: restrict this to datacenters only
    info->add_flag("resolve", 0, false);

    info = add_command(commands, list_command, list_command, list_usage, false, &admin_cluster_link_t::do_admin_list);
    info->add_positional("filter", 1, false)->add_options("issues", "machines", "namespaces", "datacenters", "!id", NULL); // TODO: how to handle --protocol: for now, just error if specified in the wrong situation
    info->add_flag("protocol", 1, false)->add_options("memcached", "dummy", NULL);
    info->add_flag("long", 0, false);

    info = add_command(commands, create_namespace_command, create_namespace_command, create_namespace_usage, true, &admin_cluster_link_t::do_admin_create_namespace);
    info->add_flag("name", 1, false);
    info->add_flag("protocol", 1, true)->add_options("memcached", "dummy", NULL);
    info->add_flag("primary", 1, false)->add_option("!id");
    info->add_flag("port", 1, true);

    info = add_command(commands, create_datacenter_command, create_datacenter_command, create_datacenter_usage, true, &admin_cluster_link_t::do_admin_create_datacenter);
    info->add_flag("name", 1, false);

    info = add_command(commands, remove_command, remove_command, remove_usage, true, &admin_cluster_link_t::do_admin_remove);
    info->add_positional("id", -1, true)->add_option("!id");

    info = add_command(commands, join_command, join_command, join_usage, false, NULL); // Special case, 'join' creates the cluster link
    info->add_positional("host-port", 1, true);

    info = add_command(commands, help_command, help_command, help_usage, false, NULL); // Special case, 'help' is not done through the cluster
    info->add_positional("type", 1, false)->add_options("split", "merge", "set", "ls", "create", "rm", "join", "help", NULL);
}

admin_cluster_link_t * admin_command_parser_t::get_cluster() {
    if (cluster == NULL) {
        if (joins_param.empty())
            throw admin_no_connection_exc_t("no join parameter specified");
        cluster = new admin_cluster_link_t(joins_param, client_port_param);

        if (console_mode) {
            cluster->sync_from();
            fprintf(stdout, "Connected to cluster with %ld machines, run 'help' for more information\n", cluster->machine_count());
        }
    }

    return cluster;
}


admin_command_parser_t::command_info * admin_command_parser_t::find_command(const std::map<std::string, admin_command_parser_t::command_info *>& cmd_map, const std::vector<std::string>& line, size_t& index) {
    std::map<std::string, command_info *>::const_iterator i = cmd_map.find(line[0]);

    if (i == commands.end())
        return NULL;
    else
        // If any subcommands exist, one must be selected by the next substring
        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info *>::iterator temp = i->second->subcommands.find(line[index]);
            if (temp == i->second->subcommands.end())
                break;
            i = temp;
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
            } else {
                throw admin_parse_exc_t("unrecognized flag: " + line[index]);
            }
        } else { // Otherwise, it must be a positional
            if (positional_index + 1 > info->positionals.size())
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

void admin_command_parser_t::run_command(command_data& data)
{
    // Special cases for help and join, which do nothing through the cluster
    if (data.info->command == "help")
        do_admin_help(data);
    else if (data.info->command == "join")
        do_admin_join(data);
    else if (data.info->do_function == NULL)
        throw admin_parse_exc_t("incomplete command (should have been caught earlier, though)");
    else {
        // TODO: sometimes this isn't downloading the data in time - supposed to be synchronous...
        get_cluster()->sync_from();

        (get_cluster()->*(data.info->do_function))(data);

        if (data.info->post_sync)
            get_cluster()->sync_to();
    }
}

void admin_command_parser_t::completion_generator_hook(const char *raw, linenoiseCompletions *completions) {
    if (instance == NULL) {
        logERR("linenoise completion generator called without an instance of admin_command_parser_t");
    } else {
        bool partial = !linenoiseIsUnescapedSpace(raw, strlen(raw) - 1);
        try {
            std::vector<std::string> line = po::split_unix(raw);
            instance->completion_generator(line, completions, partial);
        } catch(std::exception& ex) {
            // Do nothing - if the line can't be parsed or completions failed, we just can't complete the line
        }
    }
}

void admin_command_parser_t::get_id_completions(const std::string& base, linenoiseCompletions *completions) {

    std::vector<std::string> ids = get_cluster()->get_ids(base);

    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); ++i)
        linenoiseAddCompletion(completions, i->c_str());
}

std::map<std::string, admin_command_parser_t::command_info *>::const_iterator admin_command_parser_t::find_command_with_completion(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches) {
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

void admin_command_parser_t::add_option_matches(const param_options *option, const std::string& partial, linenoiseCompletions *completions) {

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

void admin_command_parser_t::add_positional_matches(const command_info *info, size_t offset, const std::string& partial, linenoiseCompletions *completions) {
    if (info->positionals.size() > offset)
        add_option_matches(info->positionals[offset], partial, completions);
}

void admin_command_parser_t::completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial) {
    // TODO: this function is too long, too complicated, and is probably redundant in some cases
    //   I'm sure it can be simplified, but for now it seems to work

    if (line.empty()) { // No command specified, available completions are all basic commands
        for (std::map<std::string, command_info *>::iterator i = commands.begin(); i != commands.end(); ++i)
            linenoiseAddCompletion(completions, i->first.c_str());
    } else { // Command specified, find a matching command/subcommand
        size_t index = 0;
        std::map<std::string, command_info *>::const_iterator i = commands.find(line[0]);
        std::map<std::string, command_info *>::const_iterator end = commands.end();

        i = find_command_with_completion(commands, line[index], completions, partial && line.size() == 1);
        if (i == end)
            return;

        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            end = i->second->subcommands.end();
            i = find_command_with_completion(i->second->subcommands, line[index], completions, partial && line.size() == index + 1);
            if (i == end)
                return;
        }

        command_info *cmd = i->second;

        if (!cmd->subcommands.empty()) {
            // We're at the end of the line, and there are still more subcommands
            find_command_with_completion(cmd->subcommands, std::string(), completions, !partial);
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
                    } else {
                        index += opt_iter->second->count;
                    }
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

    if (command_args.size() < 2 || (command_args[1] != "partial" && command_args[1] != "full"))
        throw admin_parse_exc_t("usage: rethinkdb admin complete ( partial | full ) [...]");

    completion_generator(std::vector<std::string>(command_args.begin() + 2, command_args.end()), &completions, command_args[1] == "partial");

    for (size_t i = 0; i < completions.len; ++i)
        printf("%s\n", completions.cvec[i]);

    linenoiseFreeCompletions(&completions);
}

void admin_command_parser_t::print_subcommands_usage(command_info *info, FILE *file) {
    if (!info->subcommands.empty()) {
        for (std::map<std::string, command_info *>::const_iterator i = info->subcommands.begin(); i != info->subcommands.end(); ++i) {
            print_subcommands_usage(i->second, file);
        }
    } else
        fprintf(file, "\t%s %s\n", info->full_command.c_str(), info->usage.c_str());
}

void admin_command_parser_t::parse_and_run_command(const std::vector<std::string>& line) {
    command_info *info = NULL;
    try {
        size_t index;
        info = find_command(commands, line, index);

        if (info == NULL)
            throw admin_parse_exc_t("unrecognized command: " + line[0]);
        else if (!info->subcommands.empty())
            throw admin_parse_exc_t("incomplete command");

        command_data data(parse_command(info, std::vector<std::string>(line.begin() + index, line.end())));
        run_command(data);

    } catch (admin_parse_exc_t& ex) {
        fprintf(stderr, "%s\n", ex.what());
        if (info != NULL) {
            fprintf(stderr, "usage: ");
            print_subcommands_usage(info, stderr);
        }
    } catch (admin_no_connection_exc_t& ex) {
        throw; // This will be caught and handled elsewhere
    } catch (std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
}

void admin_command_parser_t::run_console() {
    console_mode = true;

    if (!joins_param.empty())
        get_cluster();

    linenoiseSetCompletionCallback(completion_generator_hook);
    char *raw_line = linenoise(prompt);
    std::string line;

    while(raw_line != NULL) {
        line.assign(raw_line);

        if (line == exit_command)
            break;

        if (!line.empty()) {

            try {
                std::vector<std::string> split_line = po::split_unix(line);

                if (!split_line.empty())
                    parse_and_run_command(split_line);
            } catch (admin_no_connection_exc_t& ex) {
                fprintf(stderr, "not connected to a cluster, run 'help join' for more information\n");
            } catch (...) {
                fprintf(stderr, "could not parse line\n");
            }

            linenoiseHistoryAdd(raw_line);
        }

        // TODO: update prompt, timeout?
        raw_line = linenoise(prompt);
    }
    console_mode = false;
}

void admin_command_parser_t::do_admin_help(command_data& data) {
    if (data.params.count("type") == 1) {
        std::string type = data.params["type"][0];

        if (type == "rm")           do_remove_usage(console_mode);
        else if (type == "ls")      do_list_usage(console_mode);
        else if (type == "set")     do_set_usage(console_mode);
        else if (type == "help")    do_help_usage(console_mode);
        else if (type == "join")    do_join_usage(console_mode);
        else if (type == "split")   do_split_usage(console_mode);
        else if (type == "merge")   do_merge_usage(console_mode);
        else if (type == "create")  do_create_usage(console_mode);
        else throw admin_parse_exc_t("unknown command: " + type);
    } else if (data.params.count("type") == 0)
        do_usage(console_mode);
    else
        throw admin_parse_exc_t("only one help topic allowed at a time");
}

void admin_command_parser_t::do_admin_join(command_data& data) {
    if (!console_mode)
        throw admin_parse_exc_t("join specified at the shell - nothing to do");

    // Parse out the host and port
    std::string host_port = data.params["host-port"][0];
    size_t split = host_port.find(":");

    if (split == std::string::npos)
        throw admin_parse_exc_t("invalid \"host:port\" format: " + host_port);

    std::string host = host_port.substr(0, split);
    std::string port = host_port.substr(split + 1);

    // Verify that port is a number
    for (size_t i = 0; i < port.length(); ++i)
        if (port[i] > '9' || port[i] < '0')
            throw admin_parse_exc_t("invalid port: " + port);

    joins_param.clear();
    joins_param.insert(peer_address_t(ip_address_t(host), atoi(port.c_str())));

    // TODO: this probably doesn't work
    delete cluster;
    get_cluster();
}
