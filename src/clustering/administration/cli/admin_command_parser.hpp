#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_COMMAND_PARSER_HPP_
#define CLUSTERING_ADMINISTRATION_CLI_ADMIN_COMMAND_PARSER_HPP_

#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include "clustering/administration/cli/linenoise.hpp"
#include "rpc/connectivity/cluster.hpp"

class admin_cluster_link_t;

struct admin_parse_exc_t : public std::exception {
public:
    explicit admin_parse_exc_t(const std::string& data) : info(data) { }
    ~admin_parse_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

struct admin_no_connection_exc_t : public std::exception {
public:
    explicit admin_no_connection_exc_t(const std::string& data) : info(data) { }
    ~admin_no_connection_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class admin_command_parser_t {
public:
    struct command_data;

    // Used by the command-line launcher to run complete in a special mode
    static const char *complete_command;

private:

    struct param_options_t {
        param_options_t(const std::string& _name, int _count, bool _required) :
            name(_name), count(_count), required(_required) { }

        void add_option(const char *term);
        void add_options(const char *term, ...);

        const std::string name;
        const size_t count; // -1: unlimited, 0: flag only, n: n params expected
        const bool required;
        std::set<std::string> valid_options; // !uuid or !name or literal
    };

    struct command_info_t {
        command_info_t(std::string full_cmd,
                     std::string cmd,
                     std::string use,
                     void (admin_cluster_link_t::* const fn)(command_data&)) :
            full_command(full_cmd), command(cmd), usage(use), do_function(fn) { }

        ~command_info_t();

        param_options_t * add_flag(const std::string& name, int count, bool required);
        param_options_t * add_positional(const std::string& name, int count, bool required);

        std::string full_command;
        std::string command;
        std::string usage;
        void (admin_cluster_link_t::* do_function)(command_data&);

        std::vector<param_options_t *> positionals;
        std::map<std::string, param_options_t *> flags;
        std::map<std::string, command_info_t *> subcommands;
    };

public:

    struct command_data {
        explicit command_data(const command_info_t *cmd_info) : info(cmd_info) { }
        const command_info_t * const info;
        std::map<std::string, std::vector<std::string> > params;
    };

    admin_command_parser_t(const std::string& peer_string, const std::set<peer_address_t>& joins, int client_port, signal_t *_interruptor);
    ~admin_command_parser_t();

    void parse_and_run_command(const std::vector<std::string>& line);
    void run_console(bool exit_on_failure);
    void run_completion(const std::vector<std::string>& command_args);

    static void do_usage(bool console);

private:

    struct admin_help_info_t {
        admin_help_info_t(const char *_command, const char *_usage, const char *_description) :
            command(_command), usage(_usage), description(_description) { }
        std::string command;
        std::string usage;
        std::string description;
    };

    static void do_usage_internal(const std::vector<admin_help_info_t>& helps,
                                  const std::vector<std::pair<std::string, std::string> >& options,
                                  const std::string& header,
                                  bool console);

    void build_command_descriptions();
    void destroy_command_descriptions(std::map<std::string, command_info_t *>& cmd_map);
    command_info_t * add_command(std::map<std::string, command_info_t *>& cmd_map,
                               const std::string& full_cmd,
                               const std::string& cmd,
                               const std::string& usage,
                               void (admin_cluster_link_t::* const fn)(command_data&));
    admin_cluster_link_t * get_cluster();

    void do_admin_help(command_data& data);

    command_info_t * find_command(const std::map<std::string, command_info_t *>& cmd_map, const std::vector<std::string>& line, size_t& index);
    command_data parse_command(command_info_t *info, const std::vector<std::string>& command_args);
    void run_command(command_data& data);

    std::string get_subcommands_usage(command_info_t *info);

    std::map<std::string, command_info_t *>::const_iterator find_command_with_completion(const std::map<std::string, command_info_t *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches);
    void add_option_matches(const param_options_t *option, const std::string& partial, linenoiseCompletions *completions);
    void add_positional_matches(const command_info_t *info, size_t offset, const std::string& partial, linenoiseCompletions *completions);
    void get_id_completions(const std::string& base, linenoiseCompletions *completions);
    static void completion_generator_hook(const char *raw, linenoiseCompletions *completions);
    void completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial);

    std::map<std::string, command_info_t *> commands;

    // Static instance variable to hook in from a c-style library (linenoise), singleton enforced
    static admin_command_parser_t *instance;
    static uint64_t cluster_join_timeout;

    // Variables to instantiate a link to the cluster
    std::string join_peer;
    std::set<peer_address_t> joins_param;
    int client_port_param;
    admin_cluster_link_t *cluster;
    bool console_mode;
    signal_t *interruptor;

    DISABLE_COPYING(admin_command_parser_t);
};

#endif /* CLUSTERING_ADMINISTRATION_CLI_ADMIN_COMMAND_PARSER_HPP_ */
