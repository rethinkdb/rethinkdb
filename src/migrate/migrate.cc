#include "migrate/migrate.hpp"
#include <getopt.h>
#include "help.hpp"
#include <boost/algorithm/string/join.hpp>

namespace migrate {

void usage(UNUSED const char *name) {
    Help_Pager *help = Help_Pager::instance();
    help->pagef("Usage:\n"
                "        rethinkdb fsck [OPTIONS] -f <file_1> [-f <file_2> ...]\n");
    help->pagef("\n"
                "Options:\n"
                "  -f  --file                Path to file or block device where part or all of\n"
                "                            the database exists.\n"
                "      --ignore-diff-log     Do not apply patches from the diff log while\n"
                "                            checking the database.\n");
    help->pagef("\n"
                "Output options:\n"
                "  -l  --log-file            File to log to.  If not provided, messages will be\n"
                "                            printed to stderr.\n");
#ifndef NDEBUG
    help->pagef("  -c  --command-line        Print the command line arguments that were used\n"
                "                            to start this server.\n");
#endif
    help->pagef("\n"
                "Fsck is used to check one or more files for consistency\n");
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

    system(boost::algorithm::join(command_line, " ").c_str());
    return 0;
}
