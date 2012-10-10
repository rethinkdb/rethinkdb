#include "clustering/administration/cli/admin_command_parser.hpp"

#include <stdarg.h>

#include <map>
#include <stdexcept>

#include "arch/runtime/thread_pool.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "clustering/administration/cli/admin_cluster_link.hpp"
#include "errors.hpp"
#include "help.hpp"

admin_command_parser_t *admin_command_parser_t::instance = NULL;
uint64_t admin_command_parser_t::cluster_join_timeout = 5000; // Give 5 seconds to connect to all machines in the cluster

const size_t MAX_RETRIES = 5;

const char *list_command = "ls";
const char *list_stats_command = "ls stats";
const char *list_issues_command = "ls issues";
const char *list_machines_command = "ls machines";
const char *list_directory_command = "ls directory";
const char *list_tables_command = "ls tables";
const char *list_datacenters_command = "ls datacenters";
const char *list_databases_command = "ls databases";
const char *exit_command = "exit";
const char *help_command = "help";
const char *resolve_command = "resolve";
const char *pin_shard_command = "pin shard";
const char *split_shard_command = "split shard";
const char *merge_shard_command = "merge shard";
const char *set_name_command = "set name";
const char *set_acks_command = "set acks";
const char *set_replicas_command = "set replicas";
const char *set_primary_command = "set primary";
const char *unset_primary_command = "unset primary";
const char *set_datacenter_command = "set datacenter";
const char *unset_datacenter_command = "unset datacenter";
const char *set_database_command = "set database";
const char *create_table_command = "create table";
const char *create_datacenter_command = "create datacenter";
const char *create_database_command = "create database";
const char *remove_machine_command = "rm machine";
const char *remove_table_command = "rm table";
const char *remove_datacenter_command = "rm datacenter";
const char *remove_database_command = "rm database";

// Special commands - used only in certain cases
const char *admin_command_parser_t::complete_command = "complete";

const char *exit_usage = "";
const char *list_usage = "[<ID> | --long]";
const char *list_stats_usage = "[<MACHINE>...] [<TABLE>...]";
const char *list_issues_usage = "";
const char *list_machines_usage = "[--long]";
const char *list_directory_usage = "[--long]";
// TODO: fix this once multiple protocols are supported again
const char *list_tables_usage = "[--long]";
// const char *list_tables_usage = "[--protocol <PROTOCOL>] [--long]";
const char *list_datacenters_usage = "[--long]";
const char *list_databases_usage = "[--long]";
const char *help_usage = "[ ls | create | rm | set | split | merge | pin | resolve | help ]";
const char *resolve_usage = "<ID> <FIELD>";
const char *pin_shard_usage = "<TABLE> <SHARD> [--master <MACHINE>] [--replicas <MACHINE>...]";
const char *split_shard_usage = "<TABLE> <SPLIT-POINT>...";
const char *merge_shard_usage = "<TABLE> <SPLIT-POINT>...";
const char *set_name_usage = "<ID> <NEW-NAME>";
const char *set_acks_usage = "<TABLE> <NUM-ACKS> [<DATACENTER>]";
const char *set_replicas_usage = "<TABLE> <NUM-REPLICAS> [<DATACENTER>]";
const char *set_primary_usage = "<TABLE> <DATACENTER>";
const char *unset_primary_usage = "<TABLE>";
const char *set_datacenter_usage = "<MACHINE> <DATACENTER>";
const char *unset_datacenter_usage = "<MACHINE>";
const char *set_database_usage = "<TABLE> <DATABASE>";
// TODO: fix this once multiple protocols are supported again
const char *create_table_usage = "<NAME> --database <DATABASE> [--primary <DATACENTER>] [--primary-key <KEY>]";
// const char *create_table_usage = "<NAME> --port <PORT> --protocol <PROTOCOL> --database <DATABASE> [--primary <DATACENTER>]";
const char *create_datacenter_usage = "<NAME>";
const char *create_database_usage = "<NAME>";
const char *remove_usage = "<ID>...";

const char *list_id_option = "[<ID>]";
const char *list_long_option = "[--long]";
const char *list_stats_machine_option = "[<MACHINE>...]";
const char *list_stats_table_option = "[<TABLE>...]";
// TODO: fix this once multiple protocols are supported again
// const char *list_tables_protocol_option = "[--protocol <PROTOCOL>]";
const char *resolve_id_option = "<ID>";
const char *resolve_field_option = "<FIELD>";
const char *pin_shard_table_option = "<TABLE>";
const char *pin_shard_shard_option = "<SHARD>";
const char *pin_shard_master_option = "[--master <MACHINE>]";
const char *pin_shard_replicas_option = "[--replicas <MACHINE>...]";
const char *split_shard_table_option = "<TABLE>";
const char *split_shard_split_point_option = "<SPLIT-POINT>";
const char *merge_shard_table_option = "<TABLE>";
const char *merge_shard_split_point_option = "<SPLIT-POINT>";
const char *set_name_id_option = "<ID>";
const char *set_name_new_name_option = "<NEW-NAME>";
const char *set_acks_table_option = "<TABLE>";
const char *set_acks_datacenter_option = "<DATACENTER>";
const char *set_acks_num_acks_option = "<NUM-ACKS>";
const char *set_replicas_table_option = "<TABLE>";
const char *set_replicas_datacenter_option = "<DATACENTER>";
const char *set_replicas_num_replicas_option = "<NUM-REPLICAS>";
const char *set_primary_table_option = "<TABLE>";
const char *set_primary_datacenter_option = "<DATACENTER>";
const char *unset_primary_table_option = "<TABLE>";
const char *set_datacenter_machine_option = "<MACHINE>";
const char *set_datacenter_datacenter_option = "<DATACENTER>";
const char *unset_datacenter_machine_option = "<MACHINE>";
const char *set_database_table_option = "<TABLE>";
const char *set_database_database_option = "<DATABASE>";
const char *create_table_name_option = "<NAME>";
// TODO: fix this once multiple protocols are supported again
// const char *create_table_port_option = "--port <PORT>";
// const char *create_table_protocol_option = "--protocol <PROTOCOL>";
const char *create_table_primary_option = "--primary <DATACENTER>";
const char *create_table_primary_key_option = "--primary-key <KEY>";
const char *create_table_database_option = "--database <DATABASE>";
const char *create_datacenter_name_option = "<NAME>";
const char *create_database_name_option = "<NAME>";
const char *remove_id_option = "<ID>";

const char *list_id_option_desc = "print out a detailed description of a single object in the cluster, this can be a machine, table, or datacenter";
const char *list_long_option_desc = "print out full uuids (and extra information when listing machines, tables, datacenters, or databases)";
const char *list_stats_machine_option_desc = "limit stat collection to the set of machines specified";
const char *list_stats_table_option_desc = "limit stat collection to the set of tables specified";
// TODO: fix this once multiple protocols are supported again
// const char *list_tables_protocol_option_desc = "limit the list of tables to tables matching the specified protocol";
const char *resolve_id_option_desc = "the name or uuid of an object with a conflicted field";
// TODO: fix this once multiple protocols are supported again
const char *resolve_field_option_desc = "the conflicted field of the specified object to resolve, for machines this can be 'name' or 'datacenter', for datacenters and databases this can be 'name' only, and for tables, this can be 'name', 'database', 'datacenter', 'replicas', 'acks', 'shards', 'primary_key', primary_pinnings', or 'secondary_pinnings'";
// const char *resolve_field_option_desc = "the conflicted field of the specified object to resolve, for machines this can be 'name' or 'datacenter', for datacenters and databases this can be 'name' only, and for tables, this can be 'name', 'datacenter', 'database', 'replicas', 'acks', 'shards', 'port', master_pinnings', or 'replica_pinnings'";
const char *pin_shard_table_option_desc = "the table to change the shard pinnings of";
const char *pin_shard_shard_option_desc = "the shard to be affected, this is of the format [<LOWER-BOUND>]-[<UPPER-BOUND>] where one or more of the bounds must be specified.  Any non-alphanumeric character should be specified using escaped hexadecimal ASCII, e.g. '\\7E' for '~', the minimum and maximum bounds can be referred to as '-inf' and '+inf', respectively.  Only one shard may be modified at a time.";
const char *pin_shard_master_option_desc = "the machine to host the master replica of the shard, this machine must belong to the primary datacenter of the table";
const char *pin_shard_replicas_option_desc = "the machines to host the replicas of the shards, these must belong to datacenters that have been configured to replicate the table using the 'set replicas' command, and their numbers should not exceed the number of replicas.  The master replica counts towards this value.";
const char *split_shard_table_option_desc = "the table to add another shard to";
const char *split_shard_split_point_option_desc = "the key at which to split an existing shard into two shards, any non-alphanumeric character should be specified using escaped hexadecimal ASCII, e.g. '\\7E' for '~'";
const char *merge_shard_table_option_desc = "the table to remove a shard from";
const char *merge_shard_split_point_option_desc = "the shard boundary to remove, combining the shard on each side into one, any non-alphanumeric character should be specified using escaped hexadecimal ASCII, e.g. '\\7E' for '~'";
const char *set_name_id_option_desc = "a machine, datacenter, or table to change the name of";
const char *set_name_new_name_option_desc = "the new name for the specified object";
const char *set_acks_table_option_desc = "the table to change the acks for";
const char *set_acks_datacenter_option_desc = "a datacenter hosting the table to change the acks for, defaults to 'universe' if not specified";
const char *set_acks_num_acks_option_desc = "the number of acknowledgements required from the replicas in a datacenter for a write operation to be considered successful, this value should not exceed the number of replicas of the specified table for the specified datacenter";
const char *set_replicas_table_option_desc = "the table to change the number of replicas of";
const char *set_replicas_datacenter_option_desc = "the datacenter which will host the replicas, defaults to 'universe' if not specified";
const char *set_replicas_num_replicas_option_desc = "the number of replicas of the specified table to host in the specified datacenter, this value should not exceed the number of machines in the datacenter";
const char *set_primary_table_option_desc = "the table which will have its shards' master replicas moved to the specified datacenter";
const char *set_primary_datacenter_option_desc = "the datacenter to move to";
const char *unset_primary_table_option_desc = "the table which will have its shards' master replicas automatically redistributed across the cluster";
const char *set_datacenter_machine_option_desc = "the machine to move to the specified datacenter";
const char *set_datacenter_datacenter_option_desc = "the datacenter to move to";
const char *unset_datacenter_machine_option_desc = "the machine to move out of any datacenter";
const char *set_database_table_option_desc = "the table to move to the specified database";
const char *set_database_database_option_desc = "the database to move to";
const char *create_table_name_option_desc = "the name of the new table";
// TODO: fix this once multiple protocols are supported again
// const char *create_table_port_option_desc = "the port for the table to serve data from for every machine in the cluster";
// const char *create_table_protocol_option_desc = "the protocol for the table to use, either 'rdb' or 'memcached'";
const char *create_table_primary_option_desc = "the primary datacenter of the new table, this datacenter will host the master replicas of each shard";
const char *create_table_primary_key_option_desc = "the field to use as the primary key in the new table";
const char *create_table_database_option_desc = "the database that the table will exist in, client requests must be directed to this database";
const char *create_datacenter_name_option_desc = "the name of the new datacenter";
const char *create_database_name_option_desc = "the name of the new database";
const char *remove_id_option_desc = "the name or uuid of the object to remove";

const char *list_description = "Print a list of objects in the cluster.  An individual object can be selected by name or uuid for a detailed description of the object.";
const char *list_stats_description = "Print a list of statistics gathered by the cluster.  Statistics will be on a per-machine and per-table basis, if applicable, and can be filtered by machine or table.";
const char *list_issues_description = "Print a list of issues currently detected by the cluster.";
const char *list_machines_description = "Print a list of machines in the cluster along with some relevant data about each machine.";
const char *list_directory_description = "Print a list of nodes currently connected to the running admin client, this may include data servers, proxy nodes, or other admin clients.";
// TODO: fix this once multiple protocols are supported again
const char *list_tables_description = "Print a list of tables in the cluster along with some relevant data about each table.";
// const char *list_tables_description = "Print a list of tables in the cluster along with some relevant data about each table. The list may be filtered by a table protocol type.";
const char *list_datacenters_description = "Print a list of datacenters in the cluster along with some relevant data about each datacenter.";
const char *list_databases_description = "Print a list of databases in the cluster along with some relevant data about each database.";
const char *exit_description = "Quit the cluster administration console.";
const char *help_description = "Print help on a cluster administration command.";
const char *resolve_description = "If there are any conflicted values in the cluster, list the possible values for a conflicted field, then resolve the conflict by selecting one of the values.";
const char *pin_shard_description = "Set machines to host the master and/or replicas for a given shard in a table.";
const char *split_shard_description = "Add a new shard to a table by creating a new split point.  This will subdivide a given shard into two shards at the specified key, and clear all existing pinnings for the table.";
const char *merge_shard_description = "Remove a shard from a table by deleting a split point.  This will merge the two shards on each side of the split point into one, and clear all existing pinnings for the table.";
const char *set_name_description = "Set the name of an object.  This object may be referred to by its existing name or its UUID.  An object may have only one name at a time.";
const char *set_acks_description = "Set how many replicas must acknowledge a write operation for it to succeed, for the given table and datacenter.";
const char *set_replicas_description = "Set the replica affinities of a table.  This represents the number of replicas that the table will have in each specified datacenter.";
const char *set_primary_description = "Set the primary datacenter of a table, which will move the master replicas to this datacenter.";
const char *unset_primary_description = "Clear the primary datacenter of a table, which will allow the cluster to automatically distribute replicas.";
const char *set_datacenter_description = "Set the datacenter that a machine belongs to.";
const char *unset_datacenter_description = "Clear the datacenter of a machine.";
const char *set_database_description = "Set the database that a table belongs to.";
// TODO: fix this once multiple protocols are supported again
const char *create_table_description = "Create a new table in the given database and primary datacenter.";
// const char *create_table_description = "Create a new table with the given protocol.  The table's primary datacenter and listening port must be specified.";
const char *create_datacenter_description = "Create a new datacenter with the given name.  Machines and replicas may be assigned to the datacenter.";
const char *create_database_description = "Create a new database with the given name.  Tables may be assigned to the database.";
const char *remove_description = "Remove one or more objects from the cluster.";
const char *remove_machine_description = "Remove one or more machines from the cluster.";
const char *remove_table_description = "Remove one or more tables from the cluster.";
const char *remove_datacenter_description = "Remove one or more datacenters from the cluster.";
const char *remove_database_description = "Remove one or more database from the cluster.";

std::vector<std::string> parse_line(const std::string& line) {
    std::vector<std::string> result;
    char quote = '\0';
    bool escape = false;
    std::string argument;

    for (size_t i = 0; i < line.length(); ++i) {
        if (escape) {
            // It was easier for me to understand the boolean logic like this, empty if statements intentional
            if (quote == '\0' && (line[i] == '\'' || line[i] == '\"' || line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')) {
            } else if (quote != '\0' && line[i] == quote) {
            } else {
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

void admin_command_parser_t::param_options_t::add_option(const char *term) {
    valid_options.insert(term);
}

void admin_command_parser_t::param_options_t::add_options(const char *term, ...) {
    va_list terms;
    va_start(terms, term);
    while (term != NULL) {
        add_option(term);
        term = va_arg(terms, const char *);
    }
    va_end(terms);
}

admin_command_parser_t::command_info_t::~command_info_t() {
    for(std::map<std::string, param_options_t *>::iterator i = flags.begin(); i != flags.end(); ++i) {
        delete i->second;
    }

    for(std::vector<param_options_t *>::iterator i = positionals.begin(); i != positionals.end(); ++i) {
        delete *i;
    }

    for(std::map<std::string, command_info_t *>::iterator i = subcommands.begin(); i != subcommands.end(); ++i) {
        delete i->second;
    }
}

admin_command_parser_t::param_options_t *admin_command_parser_t::command_info_t::add_flag(const std::string& name,
                                                                                          int count,
                                                                                          bool required,
                                                                                          bool hidden)
{
    param_options_t *option = new param_options_t(name, count, required, hidden);
    flags.insert(std::make_pair(name, option));
    return option;
}

admin_command_parser_t::param_options_t *admin_command_parser_t::command_info_t::add_positional(const std::string& name,
                                                                                                int count,
                                                                                                bool required,
                                                                                                bool hidden)
{
    param_options_t *option = new param_options_t(name, count, required, hidden);
    positionals.push_back(option);
    return option;
}

std::string make_bold(const std::string& str) {
    return "\x1b[1m" + str + "\x1b[0m";
}

std::string underline_options(const std::string& str) {
    std::string result;
    bool format_on = false;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '<') {
            result.append("\x1b[1m\x1b[4m");
            format_on = true;
        } else if (str[i] == '>') {
            result.append("\x1b[0m");
            format_on = false;
        } else {
            result += str[i];
        }
    }
    if (format_on) {
        result.append("\x1b[0m");
    }
    return result;
}

// Format by width should only be passed a string with spaces as whitespace, any others will be converted to spaces
std::string indent_and_underline(const std::string& str, size_t initial_indent, size_t subsequent_indent, size_t terminal_width) {
    std::string result(initial_indent, ' ');
    std::string indent_string(subsequent_indent, ' ');
    std::string current_word;
    size_t effective_word_length = 0; // Track word length seperately, as angle brackets should not count
    size_t current_width = result.length();
    bool need_space = false; // Flag to add a space, always true once a word has been written to the result

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] != ' ' && str[i] != '\r' && str[i] != '\n' && str[i] != '\t') {
            current_word += str[i];
            if (str[i] != '<' && str[i] != '>') {
                ++effective_word_length;
            }
        }

        if (i == str.length() - 1 || str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t') {
            if (current_width + effective_word_length + (need_space ? 1 : 0) > terminal_width) {
                // Word will not fit on the current line, may need to split it onto multiple lines if it's too long
                // TODO: if the word is too long to fit on its own line, break it up
                result += '\n' + indent_string + current_word;
                current_width = indent_string.length() + effective_word_length;
            } else {
                // Word should fit on this same line, append it
                if (need_space) {
                    result += ' ';
                    ++current_width;
                }
                result += current_word;
                current_width += effective_word_length;
            }
            current_word.clear();
            effective_word_length = 0;
            need_space = true;
        }
    }

    return underline_options(result);
}

void admin_command_parser_t::do_usage_internal(const std::vector<admin_help_info_t>& helps,
                                               const std::vector<std::pair<std::string, std::string> >& options,
                                               const std::string& header,
                                               bool console) {
    const char *prefix = console ? "" : "rethinkdb admin <ADMIN-OPTIONS> ";
    scoped_ptr_t<help_pager_t> help(new help_pager_t());

    struct winsize ws;
    size_t width = 80;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0) width = ws.ws_col;

    if (!header.empty()) {
        help->pagef("%s\n", make_bold("INFO").c_str());
        if (console) help->pagef("%s\n\n", indent_and_underline(header, 4, 6, width).c_str());
        else         help->pagef("%s\n\n", indent_and_underline("rethinkdb admin " + header, 4, 6, width).c_str());
    }

    help->pagef("%s\n", make_bold("COMMANDS").c_str());
    for (size_t i = 0; i < helps.size(); ++i) {
        std::string text = prefix + helps[i].command + " " + helps[i].usage;
        help->pagef("%s\n", indent_and_underline(text, 4, 6, width).c_str());
    }
    help->pagef("\n");

    bool description_header_printed = false;
    for (size_t i = 0; i < helps.size(); ++i) {
        if (helps[i].description.c_str() == NULL) {
            continue;
        }
        if (description_header_printed == false) {
            help->pagef("%s\n", make_bold("DESCRIPTION").c_str());
            description_header_printed = true;
        }
        std::string some_other_header = prefix + helps[i].command + " " + helps[i].usage;
        std::string desc = helps[i].description;
        help->pagef("%s\n%s\n\n", indent_and_underline(some_other_header, 4, 6, width).c_str(),
                                  indent_and_underline(desc, 8, 8, width).c_str());
    }

    if (!console || !options.empty()) {
        help->pagef("%s\n", make_bold("OPTIONS").c_str());

        if (!console) {
            help->pagef("%s\n", indent_and_underline("<ADMIN-OPTIONS>", 4, 6, width).c_str());
#ifndef NDEBUG
            help->pagef("%s\n", indent_and_underline("[--client-port <PORT>]", 8, 10, width).c_str());
            help->pagef("%s\n\n", indent_and_underline("debug only option, specify the local port to use when connecting to the cluster", 12, 12, width).c_str());
#endif
            help->pagef("%s\n", indent_and_underline("-j,--join <HOST>:<PORT>", 8, 10, width).c_str());
            help->pagef("%s\n\n", indent_and_underline("specify the host and cluster port of a node in the cluster to join", 12, 12, width).c_str());
        }

        for (size_t i = 0; i < options.size(); ++i) {
            help->pagef("%s\n", indent_and_underline(options[i].first, 4, 6, width).c_str());
            help->pagef("%s\n\n", indent_and_underline(options[i].second, 8, 8, width).c_str());
        }
    }
}

void admin_command_parser_t::do_usage(bool console) {
    std::vector<admin_help_info_t> helps;
    std::vector<std::pair<std::string, std::string> > options;
    std::string header_string("- access or modify cluster metadata, run 'help <COMMAND>' for additional information");

    if (!console) {
        helps.push_back(admin_help_info_t("", "", "open a console to issue multiple commands"));
    }
    helps.push_back(admin_help_info_t("set", "", "change a value in the cluster"));
    helps.push_back(admin_help_info_t("unset", "", "clear a value in the cluster"));
    helps.push_back(admin_help_info_t(list_command, "", "print cluster data"));
    helps.push_back(admin_help_info_t(resolve_command, "", "resolve a value conflict"));
    helps.push_back(admin_help_info_t(split_shard_command, "", "add shards to a table"));
    helps.push_back(admin_help_info_t(merge_shard_command, "", "remove shards from a table"));
    helps.push_back(admin_help_info_t(pin_shard_command, "", "assign the machines to host a shard"));
    helps.push_back(admin_help_info_t("create", "", "add a new object to the cluster"));
    helps.push_back(admin_help_info_t("rm", "", "remove an object from the cluster"));
    helps.push_back(admin_help_info_t(help_command, "", "print help about the specified command"));

    if (console) {
        helps.push_back(admin_help_info_t(exit_command, "", "quit the cluster administration console"));
        header_string = "rethinkdb admin console " + header_string;
    }

    do_usage_internal(helps, options, header_string, console);
}

admin_command_parser_t::admin_command_parser_t(const std::string& peer_string, const peer_address_set_t& joins, int client_port, signal_t *_interruptor) :
    join_peer(peer_string),
    joins_param(joins),
    client_port_param(client_port),
    cluster(NULL),
    console_mode(false),
    interruptor(_interruptor)
{
    guarantee(instance == NULL);
    instance = this;

    build_command_descriptions();
}

admin_command_parser_t::~admin_command_parser_t() {
    guarantee(instance == this);
    instance = NULL;

    destroy_command_descriptions(&commands);
}

void admin_command_parser_t::destroy_command_descriptions(std::map<std::string, command_info_t *> *cmd_map) {
    for (std::map<std::string, command_info_t *>::iterator i = cmd_map->begin(); i != cmd_map->end(); ++i) {
        destroy_command_descriptions(&i->second->subcommands);
        delete i->second;
    }
    cmd_map->clear();
}

admin_command_parser_t::command_info_t *admin_command_parser_t::add_command(const std::string& full_cmd,
                                                                            const std::string& cmd,
                                                                            const std::string& usage,
                                                                            void (admin_cluster_link_t::*const fn)(const command_data&),
                                                                            std::map<std::string, command_info_t *> *cmd_map) {
    command_info_t *info = NULL;
    size_t space_index = cmd.find_first_of(" \t\r\n");

    // Parse out the beginning of the command, in case it's a multi-level command, and do the add recursively
    if (space_index != std::string::npos) {
        std::string fragment(cmd.substr(0, space_index));
        std::map<std::string, command_info_t *>::iterator i = cmd_map->find(fragment);

        if (i == cmd_map->end()) {
            i = cmd_map->insert(std::make_pair(fragment, new command_info_t(full_cmd, fragment, "", NULL))).first;
        }

        info = add_command(full_cmd, cmd.substr(space_index + 1), usage, fn, &i->second->subcommands);
    } else {
        std::map<std::string, command_info_t *>::iterator i = cmd_map->find(cmd);

        if (i == cmd_map->end()) {
            i = cmd_map->insert(std::make_pair(cmd, new command_info_t(full_cmd, cmd, usage, fn))).first;
        } else {
            // This node already exists, but this command should overwrite the current values
            i->second->usage = usage;
            i->second->do_function = fn;
        }

        info = i->second;
    }

    return info;
}

void admin_command_parser_t::build_command_descriptions() {
    guarantee(commands.empty());
    command_info_t *info = NULL;

    info = add_command(pin_shard_command, pin_shard_command, pin_shard_usage, &admin_cluster_link_t::do_admin_pin_shard, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("key", 1, true); // TODO: list possible shards
    info->add_flag("master", 1, false)->add_option("!machine");
    info->add_flag("replicas", -1, false)->add_option("!machine");

    info = add_command(split_shard_command, split_shard_command, split_shard_usage, &admin_cluster_link_t::do_admin_split_shard, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("split-points", -1, true);

    info = add_command(merge_shard_command, merge_shard_command, merge_shard_usage, &admin_cluster_link_t::do_admin_merge_shard, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("split-points", -1, true); // TODO: list possible shards

    info = add_command(resolve_command, resolve_command, resolve_usage, &admin_cluster_link_t::do_admin_resolve, &commands);
    info->add_positional("id", 1, true)->add_option("!conflict");
    info->add_positional("field", 1, true); // TODO: list the conflicted fields in the previous id

    info = add_command(set_name_command, set_name_command, set_name_usage, &admin_cluster_link_t::do_admin_set_name, &commands);
    info->add_positional("id", 1, true)->add_option("!id");
    info->add_positional("new-name", 1, true);

    info = add_command(set_acks_command, set_acks_command, set_acks_usage, &admin_cluster_link_t::do_admin_set_acks, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("num-acks", 1, true);
    info->add_positional("datacenter", 1, false)->add_option("!datacenter");

    info = add_command(set_replicas_command, set_replicas_command, set_replicas_usage, &admin_cluster_link_t::do_admin_set_replicas, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("num-replicas", 1, true);
    info->add_positional("datacenter", 1, false)->add_option("!datacenter");

    info = add_command(set_primary_command, set_primary_command, set_primary_usage, &admin_cluster_link_t::do_admin_set_primary, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("datacenter", 1, true)->add_option("!datacenter");

    info = add_command(unset_primary_command, unset_primary_command, unset_primary_usage, &admin_cluster_link_t::do_admin_unset_primary, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");

    info = add_command(set_datacenter_command, set_datacenter_command, set_datacenter_usage, &admin_cluster_link_t::do_admin_set_datacenter, &commands);
    info->add_positional("machine", 1, true)->add_option("!machine");
    info->add_positional("datacenter", 1, true)->add_option("!datacenter");

    info = add_command(unset_datacenter_command, unset_datacenter_command, unset_datacenter_usage, &admin_cluster_link_t::do_admin_unset_datacenter, &commands);
    info->add_positional("machine", 1, true)->add_option("!machine");

    info = add_command(set_database_command, set_database_command, set_database_usage, &admin_cluster_link_t::do_admin_set_database, &commands);
    info->add_positional("table", 1, true)->add_option("!namespace");
    info->add_positional("database", 1, true)->add_option("!database");

    info = add_command(list_command, list_command, list_usage, &admin_cluster_link_t::do_admin_list, &commands);
    info->add_positional("object", 1, false)->add_options("!id", NULLPTR);
    info->add_flag("long", 0, false);

    info = add_command(list_stats_command, list_stats_command, list_stats_usage, &admin_cluster_link_t::do_admin_list_stats, &commands);
    info->add_positional("id-filter", -1, false)->add_options("!machine", "!namespace", NULLPTR);

    info = add_command(list_issues_command, list_issues_command, list_issues_usage, &admin_cluster_link_t::do_admin_list_issues, &commands);

    info = add_command(list_machines_command, list_machines_command, list_machines_usage, &admin_cluster_link_t::do_admin_list_machines, &commands);
    info->add_flag("long", 0, false);

    info = add_command(list_directory_command, list_directory_command, list_directory_usage, &admin_cluster_link_t::do_admin_list_directory, &commands);
    info->add_flag("long", 0, false);

    info = add_command(list_tables_command, list_tables_command, list_tables_usage, &admin_cluster_link_t::do_admin_list_tables, &commands);
    // TODO: fix this once multiple protocols are supported again
    info->add_flag("protocol", 1, false, true); // hidden option
    // info->add_flag("protocol", 1, false)->add_options("rdb", "memcached", NULLPTR);
    info->add_flag("long", 0, false);

    info = add_command(list_datacenters_command, list_datacenters_command, list_datacenters_usage, &admin_cluster_link_t::do_admin_list_datacenters, &commands);
    info->add_flag("long", 0, false);

    info = add_command(list_databases_command, list_databases_command, list_databases_usage, &admin_cluster_link_t::do_admin_list_databases, &commands);
    info->add_flag("long", 0, false);

    info = add_command(create_table_command, create_table_command, create_table_usage, &admin_cluster_link_t::do_admin_create_table, &commands);
    info->add_positional("name", 1, true);
    // TODO: fix this once multiple protocols are supported again
    info->add_flag("protocol", 1, false, true); // hidden option
    info->add_flag("port", 1, false, true); // hidden option
    // info->add_flag("protocol", 1, false)->add_options("rdb", "memcached", NULLPTR);
    // info->add_flag("port", 1, true);
    info->add_flag("primary", 1, false)->add_option("!datacenter");
    info->add_flag("primary-key", 1, false);
    info->add_flag("database", 1, true)->add_option("!database");

    info = add_command(create_datacenter_command, create_datacenter_command, create_datacenter_usage, &admin_cluster_link_t::do_admin_create_datacenter, &commands);
    info->add_positional("name", 1, true);

    info = add_command(create_database_command, create_database_command, create_database_usage, &admin_cluster_link_t::do_admin_create_database, &commands);
    info->add_positional("name", 1, true);

    info = add_command(remove_machine_command, remove_machine_command, remove_usage, &admin_cluster_link_t::do_admin_remove_machine, &commands);
    info->add_positional("id", -1, true)->add_option("!machine");

    info = add_command(remove_table_command, remove_table_command, remove_usage, &admin_cluster_link_t::do_admin_remove_table, &commands);
    info->add_positional("id", -1, true)->add_option("!namespace");

    info = add_command(remove_datacenter_command, remove_datacenter_command, remove_usage, &admin_cluster_link_t::do_admin_remove_datacenter, &commands);
    info->add_positional("id", -1, true)->add_option("!datacenter");

    info = add_command(remove_database_command, remove_database_command, remove_usage, &admin_cluster_link_t::do_admin_remove_database, &commands);
    info->add_positional("id", -1, true)->add_option("!database");

    info = add_command(help_command, help_command, help_usage, NULL, &commands); // Special case, 'help' is not done through the cluster
    info->add_positional("command", 1, false)->add_options("split", "merge", "set", "ls", "create", "rm", "resolve", "help", NULLPTR);
    info->add_positional("subcommand", 1, false);
}

admin_cluster_link_t *admin_command_parser_t::get_cluster() {
    if (!cluster.has()) {
        if (joins_param.empty()) {
            throw admin_no_connection_exc_t("no join parameter specified");
        }
        cluster.init(new admin_cluster_link_t(joins_param, client_port_param, interruptor));

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

    return cluster.get();
}


admin_command_parser_t::command_info_t *admin_command_parser_t::find_command(const std::map<std::string, admin_command_parser_t::command_info_t *>& cmd_map, const std::vector<std::string>& line, size_t& index) {
    std::map<std::string, command_info_t *>::const_iterator i = cmd_map.find(line[0]);

    if (i == commands.end()) {
        return NULL;
    } else {
        // If any subcommands exist, either one is selected by the next substring, or we're at the final command
        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info_t *>::iterator temp = i->second->subcommands.find(line[index]);
            if (temp == i->second->subcommands.end()) {
                break;
            }
            i = temp;
        }
    }

    return i->second;
}

admin_command_parser_t::command_data admin_command_parser_t::parse_command(command_info_t *info, const std::vector<std::string>& line)
{
    command_data data(info);
    size_t positional_index = 0;
    size_t positional_count = 0;

    // Parse out options and parameters
    for (size_t index = 0; index < line.size(); ++index) {
        if (line[index].find("--") == 0) { // If the argument starts with a "--", it must be a flag option
            std::map<std::string, param_options_t *>::iterator opt_iter = info->flags.find(line[index].substr(2));
            if (opt_iter != info->flags.end()) {
                param_options_t *option = opt_iter->second;
                if (option->count == 0) { // Flag only, no extra parameters
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

    for (std::map<std::string, param_options_t *>::iterator j = info->flags.begin(); j != info->flags.end(); ++j) {
        if (j->second->required && data.params.find(j->second->name) == data.params.end()) {
            throw admin_parse_exc_t("required flag not specified: " + j->second->name);
        }
    }

    return data;
}

void admin_command_parser_t::run_command(const command_data& data) {
    // Special cases for help and join, which do nothing through the cluster
    if (data.info->command == "help") {
        do_admin_help(data);
    } else if (data.info->do_function == NULL) {
        throw admin_parse_exc_t("incomplete command (should have been caught earlier, though)");
    } else {
        get_cluster()->sync_from();

        // Retry until the command completes, this is necessary if multiple sources modify the same vclock
        size_t tries = 0;
        while (true) {
            ++tries;
            try {
                (get_cluster()->*(data.info->do_function))(data);
                break;
            } catch (const admin_retry_exc_t& ex) { // commit rejected by the selected peer
            } catch (const resource_lost_exc_t& ex) { // selected peer went down
            }

            if (tries == MAX_RETRIES) {
                throw admin_cluster_exc_t("too many failed tries to update the metadata through the cluster");
            }
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
        } catch (const std::exception& ex) {
            // Do nothing - if the line can't be parsed or completions failed, we just can't complete the line
        }
    }
}

std::map<std::string, admin_command_parser_t::command_info_t *>::const_iterator admin_command_parser_t::find_command_with_completion(const std::map<std::string, command_info_t *>& _commands, const std::string& str, linenoiseCompletions *completions, bool add_matches) {
    std::map<std::string, command_info_t *>::const_iterator i = _commands.find(str);
    if (add_matches) {
        if (i == _commands.end()) {
            for (i = _commands.lower_bound(str); i != _commands.end() && i->first.find(str) == 0; ++i) {
                linenoiseAddCompletion(completions, i->first.c_str());
            }
        } else {
            linenoiseAddCompletion(completions, i->first.c_str());
        }

        return _commands.end();
    }
    return i;
}

void admin_command_parser_t::add_option_matches(const param_options_t *option, const std::string& partial, linenoiseCompletions *completions) {
    if (option->hidden || (partial.find("!") == 0) ) {
        // Skip any hidden options, no completions there
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
        if (option->valid_options.count("!database") > 0) {
            std::vector<std::string> delta = get_cluster()->get_database_ids(partial);
            std::copy(delta.begin(), delta.end(), std::back_inserter(ids));
        }
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

void admin_command_parser_t::add_positional_matches(const command_info_t *info, size_t offset, const std::string& partial, linenoiseCompletions *completions) {
    if (info->positionals.size() > offset) {
        add_option_matches(info->positionals[offset], partial, completions);
    }
}

void admin_command_parser_t::completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial) {
    // TODO: this function is too long, too complicated, and is probably redundant in some cases
    //   I'm sure it can be simplified, but for now it seems to work

    if (line.empty()) {
        // No command specified, available completions are all basic commands
        for (std::map<std::string, command_info_t *>::iterator i = commands.begin(); i != commands.end(); ++i) {
            linenoiseAddCompletion(completions, i->first.c_str());
        }
    } else {
        // Command specified, find a matching command/subcommand
        size_t index = 0;
        std::map<std::string, command_info_t *>::const_iterator i = commands.find(line[0]);
        std::map<std::string, command_info_t *>::const_iterator end = commands.end();

        i = find_command_with_completion(commands, line[index], completions, partial && line.size() == 1);
        if (i == end) {
            return;
        }

        for (index = 1; index < line.size() && !i->second->subcommands.empty(); ++index) {
            std::map<std::string, command_info_t *>::const_iterator old = i;
            i = find_command_with_completion(i->second->subcommands, line[index], completions, partial && line.size() == index + 1);
            if (i == old->second->subcommands.end()) {
                i = old;
                break;
            }
        }

        command_info_t *cmd = i->second;

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
                std::map<std::string, param_options_t *>::iterator opt_iter = cmd->flags.find(line[index].substr(2));
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
                            if (!opt_iter->second->hidden) {
                                linenoiseAddCompletion(completions, ("--" + opt_iter->first).c_str());
                            }
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

std::string admin_command_parser_t::get_subcommands_usage(command_info_t *info) {
    std::string result;

    if (!info->usage.empty()) {
        result += "\t" + info->full_command + " " + info->usage + "\n";
    }

    if (!info->subcommands.empty()) {
        for (std::map<std::string, command_info_t *>::const_iterator i = info->subcommands.begin(); i != info->subcommands.end(); ++i) {
            result += get_subcommands_usage(i->second);
        }
    }

    return result;
}

void admin_command_parser_t::parse_and_run_command(const std::vector<std::string>& line) {
    command_info_t *info = NULL;

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
    } catch (const admin_parse_exc_t& ex) {
        std::string exception_str(ex.what());
        if (info != NULL) {
            // Cut off the trailing newline from get_subcommands_usage
            std::string usage_str(get_subcommands_usage(info));
            exception_str += "\nusage: " + usage_str.substr(0, usage_str.size() - 1);
        }
        throw admin_parse_exc_t(exception_str);
    } catch (const std::exception& ex) {
        throw;
    }
}

class linenoiseCallable {
public:
    linenoiseCallable(const char*_prompt, char**_raw_line_ptr) : prompt(_prompt), raw_line_ptr(_raw_line_ptr) { }
    const char *prompt;
    char **raw_line_ptr;

    void operator () () const { *raw_line_ptr = linenoise(prompt); }
};

void admin_command_parser_t::run_console(bool exit_on_failure) {
    // Can only run console mode with a connection, try to get one
    console_mode = true;
    get_cluster();

    char prompt[64];
    char *raw_line;
    linenoiseCallable linenoise_blocker(prompt, &raw_line);
    std::string line;

    // TODO: Make this use strprintf, not snprintf.

    // Build the prompt based on our initial join command
    std::string join_peer_truncated(join_peer);
    if (join_peer.length() > 50) {
        join_peer_truncated = join_peer.substr(0, 47) + "...";
    }
    snprintf(prompt, sizeof(prompt), "%s> ", join_peer_truncated.c_str());

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
            } catch (const admin_no_connection_exc_t& ex) {
                free(raw_line);
                console_mode = false;
                throw admin_cluster_exc_t("fatal error: connection to cluster failed");
            } catch (const std::exception& ex) {
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

void admin_command_parser_t::do_admin_help(const command_data& data) {
    if (data.params.count("command") == 1) {
        std::string command = guarantee_param_0(data.params, "command");
        std::string subcommand = (data.params.count("subcommand") > 0 ? guarantee_param_0(data.params, "subcommand") : "");
        std::vector<admin_help_info_t> helps;
        std::vector<std::pair<std::string, std::string> > options;

        // TODO: add option descriptions
        if (command == "ls") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(list_command, list_usage, list_description));
                helps.push_back(admin_help_info_t(list_stats_command, list_stats_usage, list_stats_description));
                helps.push_back(admin_help_info_t(list_issues_command, list_issues_usage, list_issues_description));
                helps.push_back(admin_help_info_t(list_machines_command, list_machines_usage, list_machines_description));
                helps.push_back(admin_help_info_t(list_directory_command, list_directory_usage, list_directory_description));
                helps.push_back(admin_help_info_t(list_tables_command, list_tables_usage, list_tables_description));
                helps.push_back(admin_help_info_t(list_datacenters_command, list_datacenters_usage, list_datacenters_description));
                helps.push_back(admin_help_info_t(list_databases_command, list_databases_usage, list_databases_description));
                options.push_back(std::make_pair(list_id_option, list_id_option_desc));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                options.push_back(std::make_pair(list_stats_machine_option, list_stats_machine_option_desc));
                options.push_back(std::make_pair(list_stats_table_option, list_stats_table_option_desc));
                // TODO: fix this once multiple protocols are supported again
                // options.push_back(std::make_pair(list_tables_protocol_option, list_tables_protocol_option_desc));
                do_usage_internal(helps, options, "ls - display information from the cluster, run 'help ls <SUBCOMMAND>' for more information", console_mode);
            } else if (subcommand == "stats") {
                helps.push_back(admin_help_info_t(list_stats_command, list_stats_usage, list_stats_description));
                options.push_back(std::make_pair(list_stats_machine_option, list_stats_machine_option_desc));
                options.push_back(std::make_pair(list_stats_table_option, list_stats_table_option_desc));
                do_usage_internal(helps, options, "ls stats - display statistics gathered from the cluster", console_mode);
            } else if (subcommand == "issues") {
                helps.push_back(admin_help_info_t(list_issues_command, list_issues_usage, list_issues_description));
                do_usage_internal(helps, options, "ls issues - display current issues detected by the cluster", console_mode);
            } else if (subcommand == "machines") {
                helps.push_back(admin_help_info_t(list_machines_command, list_machines_usage, list_machines_description));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                do_usage_internal(helps, options, "ls machines - display a list of machines in the cluster", console_mode);
            } else if (subcommand == "directory") {
                helps.push_back(admin_help_info_t(list_directory_command, list_directory_usage, list_directory_description));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                do_usage_internal(helps, options, "ls directory - display a list of nodes connected to the cluster", console_mode);
            } else if (subcommand == "tables") {
                helps.push_back(admin_help_info_t(list_tables_command, list_tables_usage, list_tables_description));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                // TODO: fix this once multiple protocols are supported again
                // options.push_back(std::make_pair(list_tables_protocol_option, list_tables_protocol_option_desc));
                do_usage_internal(helps, options, "ls tables - display a list of tables in the cluster", console_mode);
            } else if (subcommand == "datacenters") {
                helps.push_back(admin_help_info_t(list_datacenters_command, list_datacenters_usage, list_datacenters_description));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                do_usage_internal(helps, options, "ls datacenters - display a list of datacenters in the cluster", console_mode);
            } else if (subcommand == "databases") {
                helps.push_back(admin_help_info_t(list_databases_command, list_databases_usage, list_databases_description));
                options.push_back(std::make_pair(list_long_option, list_long_option_desc));
                do_usage_internal(helps, options, "ls databases - display a list of databases in the cluster", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "set") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
                helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
                helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
                helps.push_back(admin_help_info_t(set_primary_command, set_primary_usage, set_primary_description));
                helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
                helps.push_back(admin_help_info_t(set_database_command, set_database_usage, set_database_description));
                do_usage_internal(helps, options, "set - change the value of a field in cluster metadata, run 'help set <SUBCOMMAND>' for more information", console_mode);
            } else if (subcommand == "name") {
                helps.push_back(admin_help_info_t(set_name_command, set_name_usage, set_name_description));
                options.push_back(std::make_pair(set_name_id_option, set_name_id_option_desc));
                options.push_back(std::make_pair(set_name_new_name_option, set_name_new_name_option_desc));
                do_usage_internal(helps, options, "set name - change the name of an object in the cluster", console_mode);
            } else if (subcommand == "acks") {
                helps.push_back(admin_help_info_t(set_acks_command, set_acks_usage, set_acks_description));
                options.push_back(std::make_pair(set_acks_table_option, set_acks_table_option_desc));
                options.push_back(std::make_pair(set_acks_datacenter_option, set_acks_datacenter_option_desc));
                options.push_back(std::make_pair(set_acks_num_acks_option, set_acks_num_acks_option_desc));
                do_usage_internal(helps, options, "set acks - change the number of acknowledgements required for a write operation to succeed", console_mode);
            } else if (subcommand == "replicas") {
                helps.push_back(admin_help_info_t(set_replicas_command, set_replicas_usage, set_replicas_description));
                options.push_back(std::make_pair(set_replicas_table_option, set_replicas_table_option_desc));
                options.push_back(std::make_pair(set_replicas_datacenter_option, set_replicas_datacenter_option_desc));
                options.push_back(std::make_pair(set_replicas_num_replicas_option, set_replicas_num_replicas_option_desc));
                do_usage_internal(helps, options, "set replicas - change the number of replicas for a table in a datacenter", console_mode);
            } else if (subcommand == "primary") {
                helps.push_back(admin_help_info_t(set_primary_command, set_primary_usage, set_primary_description));
                options.push_back(std::make_pair(set_primary_table_option, set_primary_table_option_desc));
                options.push_back(std::make_pair(set_primary_datacenter_option, set_primary_datacenter_option_desc));
                do_usage_internal(helps, options, "set primary - change the primary datacenter for a table", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(set_datacenter_command, set_datacenter_usage, set_datacenter_description));
                options.push_back(std::make_pair(set_datacenter_machine_option, set_datacenter_machine_option_desc));
                options.push_back(std::make_pair(set_datacenter_datacenter_option, set_datacenter_datacenter_option_desc));
                do_usage_internal(helps, options, "set datacenter - change the datacenter that a machine belongs in", console_mode);
            } else if (subcommand == "database") {
                helps.push_back(admin_help_info_t(set_database_command, set_database_usage, set_database_description));
                options.push_back(std::make_pair(set_database_table_option, set_database_table_option_desc));
                options.push_back(std::make_pair(set_database_database_option, set_database_database_option_desc));
                do_usage_internal(helps, options, "set database - change the database that a table belongs in", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "unset") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(unset_primary_command, unset_primary_usage, unset_primary_description));
                helps.push_back(admin_help_info_t(unset_datacenter_command, unset_datacenter_usage, unset_datacenter_description));
                do_usage_internal(helps, options, "unset - clear the value of a field in cluster metadata, run 'help unset <SUBCOMMAND>' for more information", console_mode);
            } else if (subcommand == "primary") {
                helps.push_back(admin_help_info_t(unset_primary_command, unset_primary_usage, unset_primary_description));
                options.push_back(std::make_pair(unset_primary_table_option, unset_primary_table_option_desc));
                do_usage_internal(helps, options, "unset primary - clear the primary datacenter for a table", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(unset_datacenter_command, unset_datacenter_usage, unset_datacenter_description));
                options.push_back(std::make_pair(unset_datacenter_machine_option, unset_datacenter_machine_option_desc));
                do_usage_internal(helps, options, "unset datacenter - clear the datacenter for a machine", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "create") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(create_table_command, create_table_usage, create_table_description));
                helps.push_back(admin_help_info_t(create_database_command, create_database_usage, create_database_description));
                helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
                do_usage_internal(helps, options, "create - add a new table or datacenter to the cluster, run 'help create <SUBCOMMAND>' for more information", console_mode);
            } else if (subcommand == "table") {
                helps.push_back(admin_help_info_t(create_table_command, create_table_usage, create_table_description));
                options.push_back(std::make_pair(create_table_name_option, create_table_name_option_desc));
                // TODO: fix this once multiple protocols are supported again
                // options.push_back(std::make_pair(create_table_port_option, create_table_port_option_desc));
                // options.push_back(std::make_pair(create_table_protocol_option, create_table_protocol_option_desc));
                options.push_back(std::make_pair(create_table_database_option, create_table_database_option_desc));
                options.push_back(std::make_pair(create_table_primary_option, create_table_primary_option_desc));
                options.push_back(std::make_pair(create_table_primary_key_option, create_table_primary_key_option_desc));
                do_usage_internal(helps, options, "create table - add a new table to the cluster", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(create_datacenter_command, create_datacenter_usage, create_datacenter_description));
                options.push_back(std::make_pair(create_datacenter_name_option, create_datacenter_name_option_desc));
                do_usage_internal(helps, options, "create datacenter - add a new datacenter to the cluster", console_mode);
            } else if (subcommand == "database") {
                helps.push_back(admin_help_info_t(create_database_command, create_database_usage, create_database_description));
                options.push_back(std::make_pair(create_database_name_option, create_database_name_option_desc));
                do_usage_internal(helps, options, "create database - add a new database to the cluster", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "rm") {
            if (subcommand.empty()) {
                helps.push_back(admin_help_info_t(remove_machine_command, remove_usage, remove_machine_description));
                helps.push_back(admin_help_info_t(remove_table_command, remove_usage, remove_table_description));
                helps.push_back(admin_help_info_t(remove_datacenter_command, remove_usage, remove_datacenter_description));
                helps.push_back(admin_help_info_t(remove_database_command, remove_usage, remove_database_description));
                options.push_back(std::make_pair(remove_id_option, remove_id_option_desc));
                do_usage_internal(helps, options, "remove - delete an object from the cluster metadata", console_mode);
            } else if (subcommand == "machine") {
                helps.push_back(admin_help_info_t(remove_machine_command, remove_usage, remove_machine_description));
                options.push_back(std::make_pair(remove_id_option, remove_id_option_desc));
                do_usage_internal(helps, options, "remove machine - delete a machine from the cluster metadata", console_mode);
            } else if (subcommand == "table") {
                helps.push_back(admin_help_info_t(remove_table_command, remove_usage, remove_table_description));
                options.push_back(std::make_pair(remove_id_option, remove_id_option_desc));
                do_usage_internal(helps, options, "remove table - delete a table from the cluster metadata", console_mode);
            } else if (subcommand == "datacenter") {
                helps.push_back(admin_help_info_t(remove_datacenter_command, remove_usage, remove_datacenter_description));
                options.push_back(std::make_pair(remove_id_option, remove_id_option_desc));
                do_usage_internal(helps, options, "remove datacenter - delete a datacenter from the cluster metadata", console_mode);
            } else if (subcommand == "database") {
                helps.push_back(admin_help_info_t(remove_database_command, remove_usage, remove_database_description));
                options.push_back(std::make_pair(remove_id_option, remove_id_option_desc));
                do_usage_internal(helps, options, "remove database - delete a database from the cluster metadata", console_mode);
            } else {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
        } else if (command == "help") {
            if (!subcommand.empty()) {
                throw admin_parse_exc_t("no recognized subcommands for 'help'");
            }
            helps.push_back(admin_help_info_t(help_command, help_usage, help_description));
            do_usage_internal(helps, options, "help - provide information about a cluster administration command", console_mode);
        } else if (command == "pin") {
            if (!subcommand.empty() && subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(pin_shard_command, pin_shard_usage, pin_shard_description));
            options.push_back(std::make_pair(pin_shard_table_option, pin_shard_table_option_desc));
            options.push_back(std::make_pair(pin_shard_shard_option, pin_shard_shard_option_desc));
            options.push_back(std::make_pair(pin_shard_master_option, pin_shard_master_option_desc));
            options.push_back(std::make_pair(pin_shard_replicas_option, pin_shard_replicas_option_desc));
            do_usage_internal(helps, options, "pin shard - assign machines to host the master or replicas of a shard", console_mode);
        } else if (command == "split") {
            if (!subcommand.empty() && subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(split_shard_command, split_shard_usage, split_shard_description));
            options.push_back(std::make_pair(split_shard_table_option, split_shard_table_option_desc));
            options.push_back(std::make_pair(split_shard_split_point_option, split_shard_split_point_option_desc));
            do_usage_internal(helps, options, "split shard - split a shard in a table into more shards", console_mode);
        } else if (command == "merge") {
            if (!subcommand.empty() && subcommand != "shard") {
                throw admin_parse_exc_t("unrecognized subcommand: " + subcommand);
            }
            helps.push_back(admin_help_info_t(merge_shard_command, merge_shard_usage, merge_shard_description));
            options.push_back(std::make_pair(merge_shard_table_option, merge_shard_table_option_desc));
            options.push_back(std::make_pair(merge_shard_split_point_option, merge_shard_split_point_option_desc));
            do_usage_internal(helps, options, "merge shard - merge two or more shards in a table", console_mode);
        } else if (command == "resolve") {
            if (!subcommand.empty()) {
                throw admin_parse_exc_t("no recognized subcommands for 'resolve'");
            }
            helps.push_back(admin_help_info_t(resolve_command, resolve_usage, resolve_description));
            options.push_back(std::make_pair(resolve_id_option, resolve_id_option_desc));
            options.push_back(std::make_pair(resolve_field_option, resolve_field_option_desc));
            do_usage_internal(helps, options, "resolve - resolve a conflict on a cluster metadata value", console_mode);
        } else {
            throw admin_parse_exc_t("unknown command: " + command);
        }
    } else {
        do_usage(console_mode);
    }
}

