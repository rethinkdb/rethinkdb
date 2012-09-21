#include <sys/resource.h>

#include <set>

#include "clustering/administration/main/command_line.hpp"
#include "clustering/administration/cli/admin_command_parser.hpp"
#include "utils.hpp"
#include "help.hpp"
#include "config/args.hpp"

void print_version_message() {
    printf("rethinkdb " RETHINKDB_VERSION
#ifndef NDEBUG
           " (debug)"
#endif
           "\n");
}

int main(int argc, char *argv[]) {
    install_generic_crash_handler();

#ifndef NDEBUG
    rlimit core_limit;
    core_limit.rlim_cur = 100 * MEGABYTE;
    core_limit.rlim_max = 200 * MEGABYTE;
    setrlimit(RLIMIT_CORE, &core_limit);
#endif

    std::set<std::string> subcommands_that_look_like_flags;
    subcommands_that_look_like_flags.insert("--version");
    subcommands_that_look_like_flags.insert("--help");
    subcommands_that_look_like_flags.insert("-h");

    if (argc == 1 || (argv[1][0] == '-' && subcommands_that_look_like_flags.count(argv[1]) == 0)) {
        return main_rethinkdb_porcelain(argc, argv);

    } else {
        std::string subcommand = argv[1];

        if (subcommand == "create") {
            return main_rethinkdb_create(argc, argv);
        } else if (subcommand == "serve") {
            return main_rethinkdb_serve(argc, argv);
        } else if (subcommand == "proxy") {
            return main_rethinkdb_proxy(argc, argv);
        } else if (subcommand == "admin") {
            return main_rethinkdb_admin(argc, argv);
        } else if (subcommand == "import") {
            return main_rethinkdb_import(argc, argv);
        } else if (subcommand == "--version") {
            if (argc != 2) {
                puts("WARNING: Ignoring extra parameters after '--version'.");
            }
            print_version_message();
            return 0;

        } else if (subcommand == "help" || subcommand == "-h" || subcommand == "--help") {

            if (argc == 2) {
                puts("'rethinkdb' is divided into a number of subcommands:");
                puts("");
                puts("    'rethinkdb create': prepare files on disk");
                puts("    'rethinkdb serve': serve queries and host data");
                puts("    'rethinkdb proxy': serve queries but don't host data");
                puts("    'rethinkdb admin': access and modify cluster metadata");
                puts("");
                puts("For more information, run 'rethinkdb help [subcommand]'.");
                return 0;

            } else if (argc == 3) {
                std::string subcommand2 = argv[2];
                if (subcommand2 == "create") {
                    help_rethinkdb_create();
                    return 0;
                } else if (subcommand2 == "serve") {
                    help_rethinkdb_serve();
                    return 0;
                } else if (subcommand2 == "admin") {
                    admin_command_parser_t::do_usage(false);
                    return 0;
                } else if (subcommand2 == "proxy") {
                    help_rethinkdb_proxy();
                } else {
                    printf("ERROR: No help for '%s'\n", subcommand2.c_str());
                    return 1;
                }

            } else {
                puts("ERROR: Too many parameters to 'rethinkdb help'.  Try 'rethinkdb help [subcommand]'.");
                return 1;
            }

        } else {
            puts("ERROR: Unrecognized subcommand ''. Try 'rethinkdb help'.");
            return 1;
        }
    }
}
