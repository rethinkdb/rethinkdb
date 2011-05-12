#include "migrate/migrate.hpp"
#include <getopt.h>
#include "help.hpp"
#include <boost/algorithm/string/join.hpp>

namespace migrate {

void usage(UNUSED const char *name) {
    Help_Pager *help = Help_Pager::instance();
    help->pagef("Usage:\n"
                "        rethinkdb migrate -f <file_1> [-f <file_2> ...]\n");
    help->pagef("\n"
                "Migrate converts rethinkdb files from one version to another.\n"
                "Interrupting mid migration is very ill advised.  However if it\n"
                "should happen it is less than apocalyptic. If the server has\n"
                "yet to begin converting the file (indicated by the message\n"
                "\"Converting file...\" then no harm has been done and the\n"
                "old data files are intact. If migration was stopped during\n"
                "this stage then the file /tmp/rdb_migration_buffer will\n"
                "hold your data as a series of memcached commands this file\n"
                "should be copied to a safe location. Please refer to the\n"
                "manual or contact support for how to enter this data.\n");
    exit(0);
}

void parse_cmd_args(int argc, char **argv, config_t *config) {
    // TODO disallow rogue options.
    config->exec_name = argv[0];
    argc--;
    argv++;

    optind = 1;  // reinit getopt.
    if (argc >= 2) {
        if (!strncmp("help", argv[1], 4)) {
            usage(argv[0]);
        }
    }
    for (;;) {
        int do_help = 0;
        static const struct option long_options[] =
            {
                {"file", required_argument, 0, 'f'},
                {"help", no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f:", long_options, &option_index);

        if (do_help) {
            c = 'h';
        }

        // Detect the end of the options.
        if (c == -1)
            break;

        switch (c) {
        case 0:
            break;
        case 'f':
            config->input_filenames.push_back(optarg);
            break;
        default:
            // getopt_long already printed an error message.
            usage(argv[0]);
        }
    }

    if (optind < argc) {
        fail_due_to_user_error("Unexpected extra argument: \"%s\"", argv[optind]);
    }

    // Sanity checks

    if (config->input_filenames.empty()) {
        fprintf(stderr, "Please specify some files.\n");
        usage(argv[0]);
    }

    {
        std::vector<std::string> names = config->input_filenames;
        std::sort(names.begin(), names.end());
        if (std::unique(names.begin(), names.end()) != names.end()) {
            fail_due_to_user_error("Duplicate file names provided.");
        }
    }
}

} // namespace migrate

int run_migrate(int argc, char **argv) {

    migrate::config_t cfg;
    migrate::parse_cmd_args(argc, argv, &cfg);

    std::vector<std::string> command_line;
    command_line.push_back("rdb_migrate");
    command_line.push_back("-r");
    command_line.push_back(cfg.exec_name);

    for (std::vector<std::string>::iterator it = cfg.input_filenames.begin(); it != cfg.input_filenames.end(); it++) {
        command_line.push_back("-i");
        command_line.push_back(*it);
    }

    return system(boost::algorithm::join(command_line, " ").c_str());
}
