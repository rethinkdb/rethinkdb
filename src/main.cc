#include <set>
#include <sys/resource.h>

#include "clustering/clustering.hpp"
#include "utils.hpp"
#include "help.hpp"

void print_version_message() {
    printf("rethinkdb " RETHINKDB_VERSION
#ifndef NDEBUG
           " (debug)"
#endif
           "\n");
}

int main(int argc, char *argv[]) {
    initialize_precise_time();
    install_generic_crash_handler();

#ifndef NDEBUG
    rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
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

        } else if (subcommand == "--version") {
            if (argc != 2) {
                std::cout << "WARNING: Ignoring extra parameters after '--version'." << std::endl;
            }
            print_version_message();
            return 0;

        } else if (subcommand == "help" || subcommand == "-h" || subcommand == "--help") {

            if (argc == 2) {
                std::cout << "'rethinkdb' is divided into a number of "
                    "subcommands:" << std::endl;
                std::cout << std::endl;
                std::cout << "    'rethinkdb create': prepare files on disk" << std::endl;
                std::cout << "    'rethinkdb serve': serve queries" << std::endl;
                std::cout << std::endl;
                std::cout << "For more information, run 'rethinkdb help [subcommand]'." << std::endl;
                return 0;

            } else if (argc == 3) {
                std::string subcommand2 = argv[2];
                if (subcommand2 == "create") {
                    help_rethinkdb_create();
                    return 0;
                } else if (subcommand2 == "serve") {
                    help_rethinkdb_serve();
                    return 0;
                } else {
                    std::cout << "ERROR: No help for '" << subcommand2 << "'." << std::endl;
                    return 1;
                }

            } else {
                std::cout << "ERROR: Too many parameters to 'rethinkdb help'. "
                    "Try 'rethinkdb help [subcommand]'." << std::endl;
                return 1;
            }

        } else {
            std::cout << "ERROR: Unrecognized subcommand '" << subcommand <<
                "'. Try 'rethinkdb help'." << std::endl;
            return 1;
        }
    }
}
