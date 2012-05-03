#ifndef CLUSTERING_ADMINISTRATION_CLI_ADMIN_COMMAND_PARSER_HPP_
#define CLUSTERING_ADMINISTRATION_CLI_ADMIN_COMMAND_PARSER_HPP_

#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include "clustering/administration/cli/linenoise.hpp"
#include "rpc/connectivity/connectivity.hpp"

class admin_cluster_link_t;

struct admin_parse_exc_t : public std::exception {
public:
    explicit admin_parse_exc_t(const std::string& data) : info(data) { }
    ~admin_parse_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class admin_command_parser_t {
public:
    struct command_data;

    // Command strings for various commands
    static const char *set_command;
    static const char *list_command;
    static const char *make_command;
    static const char *move_command;
    static const char *help_command;
    static const char *join_command;
    static const char *rename_command;
    static const char *remove_command;
    static const char *complete_command;

    // Usage strings for various commands
    static const char *set_usage;
    static const char *list_usage;
    static const char *make_usage;
    static const char *make_namespace_usage;
    static const char *make_datacenter_usage;
    static const char *move_usage;
    static const char *help_usage;
    static const char *join_usage;
    static const char *rename_usage;
    static const char *remove_usage;

private:

    struct param_options {
        param_options(const std::string& _name, int _count, bool _required) :
            name(_name), count(_count), required(_required) { }

        void add_option(const char *term);
        void add_options(const char *term, ...);

        const std::string name;
        const size_t count; // -1: unlimited, 0: flag only, n: n params expected
        const bool required;
        std::set<std::string> valid_options; // !uuid or !name or literal
    };

    struct command_info {
        command_info(std::string cmd,
                     std::string use,
                     bool sync,
                     void (admin_cluster_link_t::* const fn)(command_data&)) :
            command(cmd), usage(use), post_sync(sync), do_function(fn) { }

        ~command_info();

        param_options * add_flag(const std::string& name, int count, bool required);
        param_options * add_positional(const std::string& name, int count, bool required);
        void add_subcommand(command_info *info);

        std::string command;
        std::string usage;
        const bool post_sync;
        void (admin_cluster_link_t::* const do_function)(command_data&);

        std::vector<param_options *> positionals; // TODO: it is an error to have both positionals and subcommands
        std::map<std::string, param_options *> flags;
        std::map<std::string, command_info *> subcommands;
    };

public:

    struct command_data {
        explicit command_data(const command_info *cmd_info) : info(cmd_info) { }
        const command_info * const info;
        std::map<std::string, std::vector<std::string> > params;
    };

    admin_command_parser_t(const std::set<peer_address_t>& joins, int client_port);
    ~admin_command_parser_t();

    command_data parse_command(const std::vector<std::string>& command_args);
    void run_command(command_data& data);
    void run_console();
    void run_complete(const std::vector<std::string>& command_args);

private:

    void build_command_descriptions();
    admin_cluster_link_t* get_cluster();

    void do_admin_help_shell(command_data& data);
    void do_admin_help_console(command_data& data);
    void do_admin_join_shell(command_data& data);
    void do_admin_join_console(command_data& data);

    std::map<std::string, command_info *>::const_iterator find_command(const std::map<std::string, command_info *>& commands, const std::string& str, linenoiseCompletions *completions, bool add_matches);
    void add_option_matches(const param_options *option, const std::string& partial, linenoiseCompletions *completions);
    void add_positional_matches(const command_info *info, size_t offset, const std::string& partial, linenoiseCompletions *completions);
    void get_id_completions(const std::string& base, linenoiseCompletions *completions);
    static void completion_generator_hook(const char *raw, linenoiseCompletions *completions);
    void completion_generator(const std::vector<std::string>& line, linenoiseCompletions *completions, bool partial);

    std::map<std::string, command_info *> command_descriptions;

    // Static instance variable to hook in from a c-style library (linenoise), singleton enforced
    static admin_command_parser_t *instance;

    // Variables to instantiate a link to the cluster
    std::set<peer_address_t> joins_param;
    int client_port_param;
    admin_cluster_link_t *cluster;
    bool console_mode;

    DISABLE_COPYING(admin_command_parser_t);
};

#endif
