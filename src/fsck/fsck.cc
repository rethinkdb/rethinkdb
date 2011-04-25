#include <getopt.h>
#include <stdio.h>

#include <algorithm>

#include "fsck/checker.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "help.hpp"
#include "checker.hpp"

namespace fsck {

// TODO: Should this still be UNUSED?
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
    help->pagef("\n"
                "Fsck is used to check one or more files for consistency\n");
    exit(0);
}

enum { ignore_diff_log = 256,  // Start these values above the ASCII range.
};

void parse_cmd_args(int argc, char **argv, config_t *config) {
    // TODO disallow rogue options.

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
                {"ignore-diff-log", no_argument, 0, ignore_diff_log},
                {"log-file", required_argument, 0, 'l'},
                {"help", no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f:l:h", long_options, &option_index);

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
        case ignore_diff_log:
            config->ignore_diff_log = true;
            break;
        case 'l':
            config->log_file_name = optarg;
            break;
        case 'h':
            usage(argv[0]);
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


}  // namespace fsck

int run_fsck(int argc, char **argv) {
    fsck::config_t config;
    fsck::parse_cmd_args(argc, argv, &config);

    if (config.log_file_name != "") {
        log_file = fopen(config.log_file_name.c_str(), "w");
    }

    if (check_files(config)) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
