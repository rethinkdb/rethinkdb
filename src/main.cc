// Copyright 2010-2013 RethinkDB, all rights reserved.

#ifndef _WIN32
#include <sys/resource.h>
#endif

#include <set>

#include "clustering/administration/main/command_line.hpp"
#include "crypto/initialization_guard.hpp"
#include "utils.hpp"
#include "config/args.hpp"
#include "extproc/extproc_spawner.hpp"

int main(int argc, char *argv[]) {

    startup_shutdown_t startup_shutdown;
    crypto::initialization_guard_t crypto_initialization_guard;

#ifdef _WIN32
    extproc_maybe_run_worker(argc, argv);
#endif

    std::set<std::string> subcommands_that_look_like_flags;
    subcommands_that_look_like_flags.insert("--version");
    subcommands_that_look_like_flags.insert("-v");
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
        } else if (subcommand == "export") {
            return main_rethinkdb_export(argc, argv);
        } else if (subcommand == "import") {
            return main_rethinkdb_import(argc, argv);
        } else if (subcommand == "dump") {
            return main_rethinkdb_dump(argc, argv);
        } else if (subcommand == "restore") {
            return main_rethinkdb_restore(argc, argv);
        } else if (subcommand == "index-rebuild") {
            return main_rethinkdb_index_rebuild(argc, argv);
#ifdef _WIN32
        } else if (subcommand == "run-service") {
            return main_rethinkdb_run_service(argc, argv);
        } else if (subcommand == "install-service") {
            return main_rethinkdb_install_service(argc, argv);
        } else if (subcommand == "remove-service") {
            return main_rethinkdb_remove_service(argc, argv);
#endif /* _WIN32 */
        } else if (subcommand == "--version" || subcommand == "-v") {
            if (argc != 2) {
                printf("WARNING: Ignoring extra parameters after '%s'.", subcommand.c_str());
            }
            print_version_message();
            return 0;

        } else if (subcommand == "help" || subcommand == "-h" || subcommand == "--help") {

            if (argc == 2) {
                help_rethinkdb_porcelain();
                return 0;
            } else if (argc == 3) {
                std::string subcommand2 = argv[2];
                if (subcommand2 == "create") {
                    help_rethinkdb_create();
                    return 0;
                } else if (subcommand2 == "serve") {
                    help_rethinkdb_serve();
                    return 0;
                } else if (subcommand2 == "proxy") {
                    help_rethinkdb_proxy();
                } else if (subcommand2 == "export") {
                    help_rethinkdb_export();
                } else if (subcommand2 == "import") {
                    help_rethinkdb_import();
                } else if (subcommand2 == "dump") {
                    help_rethinkdb_dump();
                } else if (subcommand2 == "restore") {
                    help_rethinkdb_restore();
                } else if (subcommand2 == "index-rebuild") {
                    help_rethinkdb_index_rebuild();
#ifdef _WIN32
                } else if (subcommand2 == "install-service") {
                    help_rethinkdb_install_service();
                } else if (subcommand2 == "remove-service") {
                    help_rethinkdb_remove_service();
#endif
                } else {
                    printf("ERROR: No help for '%s'\n", subcommand2.c_str());
                    return 1;
                }

            } else {
                puts("ERROR: Too many parameters to 'rethinkdb help'.  Try 'rethinkdb help [subcommand]'.");
                return 1;
            }

        } else {
            printf("ERROR: Unrecognized subcommand '%s'. Try 'rethinkdb help'.\n", subcommand.c_str());
            return 1;
        }
    }
}
