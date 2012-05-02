#include "clustering/administration/cli/admin_command_parser.hpp"
#include "clustering/administration/cli/admin_cluster_link.hpp"

#include <cstdarg>
#include <iostream>
#include <map>
#include <stdexcept>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>

// TODO: make a useful prompt
const char *prompt = " > ";

admin_command_parser_t *admin_command_parser_t::instance = NULL;

const char *admin_command_parser_t::set_command = "set";
const char *admin_command_parser_t::list_command = "ls";
const char *admin_command_parser_t::make_command = "mk";
const char *admin_command_parser_t::move_command = "mv";
const char *admin_command_parser_t::help_command = "help";
const char *admin_command_parser_t::rename_command = "rename";
const char *admin_command_parser_t::remove_command = "rm";
const char *admin_command_parser_t::complete_command = "complete";

const char *admin_command_parser_t::set_usage = "( <uuid> | <name> ) <field> ... [--resolve] [--value <value>]";
const char *admin_command_parser_t::list_usage = "[ datacenters | namespaces [--protocol <protocol>] | machines | issues | <name> | <uuid> ] [--long]";
const char *admin_command_parser_t::make_usage = "( datacenter | namespace --port <port> --protocol ( memcached | dummy ) [--primary <primary datacenter uuid>] ) [--name <name>]";
const char *admin_command_parser_t::make_namespace_usage = "[--port <port>] [--protocol ( memcached | dummy ) ] [--primary <primary datacenter uuid>] [--name <name>]";
const char *admin_command_parser_t::make_datacenter_usage = "[--name <name>]";
const char *admin_command_parser_t::move_usage = "( <target-uuid> | <target-name> ) ( <datacenter-uuid> | <datacenter-name> ) [--resolve]";
const char *admin_command_parser_t::help_usage = "[ ls | mk | mv | rm | set | rename | help ]";
const char *admin_command_parser_t::rename_usage = "( <uuid> | <name> ) <new name> [--resolve]";
const char *admin_command_parser_t::remove_usage = "( <uuid> | <name> )";

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

admin_command_parser_t::admin_command_parser_t(const std::set<peer_address_t>& joins, int client_port) :
    joins_param(joins),
    client_port_param(client_port),
    cluster(NULL)
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

void admin_command_parser_t::build_command_descriptions() {
    command_descriptions.clear();

    {
        command_info *info = new command_info(set_command, set_usage, true, &admin_cluster_link_t::do_admin_set);
        info->add_flag("resolve", 0, false);
        info->add_flag("value", 1, true);
        info->add_positional("path", 1, true)->add_option("!id");
        info->add_positional("fields", -1, true);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(list_command, list_usage, false, &admin_cluster_link_t::do_admin_list);
        info->add_positional("filter", 1, false)->add_options("issues", "machines", "namespaces", "datacenters", "!id", NULL); // TODO: how to handle --protocol: for now, just error if specified in the wrong situation
        info->add_flag("protocol", 1, false)->add_options("memcached", "dummy", NULL);
        info->add_flag("long", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(make_command, make_usage, true, NULL);
        command_info *namespace_command = new command_info("namespace", make_namespace_usage, true, &admin_cluster_link_t::do_admin_make_namespace);
        command_info *datacenter_command = new command_info("datacenter", make_datacenter_usage, true, &admin_cluster_link_t::do_admin_make_datacenter);

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
        command_info *info = new command_info(move_command, move_usage, true, &admin_cluster_link_t::do_admin_move);
        info->add_positional("id", 1, true)->add_option("!id");
        info->add_positional("datacenter", 1, true)->add_option("!id"); // TODO: restrict this to datacenters only
        info->add_flag("resolve", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(remove_command, remove_usage, true, &admin_cluster_link_t::do_admin_remove);
        info->add_positional("id", 1, true)->add_option("!id");
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(rename_command, rename_usage, true, &admin_cluster_link_t::do_admin_rename);
        info->add_positional("id", 1, true)->add_option("!id");
        info->add_positional("new-name", 1, true);
        info->add_flag("resolve", 0, false);
        command_descriptions.insert(std::make_pair(info->command, info));
    }

    {
        command_info *info = new command_info(help_command, help_usage, false, NULL); // Special case, 'help' is not done through the cluster
        info->add_positional("type", -1, false)->add_options("ls", "mk", "mv", "rm", "set", "help", "rename", NULL);
        command_descriptions.insert(std::make_pair(info->command, info));
    }
}

admin_cluster_link_t* admin_command_parser_t::get_cluster() {
    if (cluster == NULL) {
        if (joins_param.empty())
            throw admin_parse_exc_t("need to join a cluster to proceed");
        cluster = new admin_cluster_link_t(joins_param, client_port_param);
    }

    return cluster;
}

admin_command_parser_t::command_data admin_command_parser_t::parse_command(const std::vector<std::string>& line)
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
    if (data.info->command == "help") {
        // Special case for help, which uses the existing command descriptions, and does nothing through the cluster
        do_admin_help(data);
    } else if (data.info->do_function == NULL) {
        throw admin_parse_exc_t("incomplete command (should have been caught earlier, though)");
    } else {
        get_cluster()->sync_from();

        (get_cluster()->*(data.info->do_function))(data);

        if (data.info->post_sync)
            get_cluster()->sync_to();
    }
}

void admin_command_parser_t::completion_generator_hook(const char *raw, linenoiseCompletions *completions) {
    if (instance == NULL)
        logERR("linenoise completion generator called without an instance of admin_command_parser_t");
    else {
        char last_char = raw[strlen(raw) - 1];
        bool partial = (last_char != ' ') && (last_char != '\t') && (last_char != '\r') && (last_char != '\n');
        std::vector<std::string> line = po::split_unix(raw);
        instance->completion_generator(line, completions, partial);
    }
}

void admin_command_parser_t::get_id_completions(const std::string& base, linenoiseCompletions *completions) {

    std::vector<std::string> ids = get_cluster()->get_ids(base);

    for (std::vector<std::string>::iterator i = ids.begin(); i != ids.end(); ++i)
        linenoiseAddCompletion(completions, i->c_str());
}

std::map<std::string, admin_command_parser_t::command_info *>::const_iterator admin_command_parser_t::find_command(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches) {
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
        for (std::map<std::string, command_info *>::iterator i = command_descriptions.begin(); i != command_descriptions.end(); ++i)
            linenoiseAddCompletion(completions, i->first.c_str());
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

void admin_command_parser_t::run_complete(const std::vector<std::string>& command_args) {
    linenoiseCompletions completions = { 0, NULL };

    if (command_args.size() < 2 || (command_args[1] != "partial" && command_args[1] != "full"))
        throw admin_parse_exc_t("usage: rethinkdb admin complete ( partial | full ) [...]");

    completion_generator(std::vector<std::string>(command_args.begin() + 2, command_args.end()), &completions, command_args[1] == "partial");

    for (size_t i = 0; i < completions.len; ++i)
        printf("%s\n", completions.cvec[i]);

    linenoiseFreeCompletions(&completions);
}

void admin_command_parser_t::run_console() {
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

void admin_command_parser_t::do_admin_help(command_data& data) {
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

