#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "extract/filewalk.hpp"
#include "errors.hpp"
#include "logger.hpp"
#include "help.hpp"

namespace extract {

void usage(const char *name) {
    // Note: some error messages may refer to the names of command
    // line options here, so keep them updated accordingly.
    Help_Pager *help = Help_Pager::instance();

    help->pagef("Usage:\n"
                "        rethinkdb extract [OPTIONS] -f <file_1> [-f <file_2> ...]\n"
                "        Extract data from possibly corrupted database files as a series of memcached commands\n");
    help->pagef("\n"
                "Options:\n"
                "  -f  --file                Path to file or block device where part or all of\n"
                "                            the database exists.\n");
    help->pagef("\n"
                "On disk format options:\n"
                "      --force-block-size    Specifies block size, overriding file headers\n"
                "      --force-extent-size   Specifies extent size, overriding file headers\n"
                "      --force-slice-count     Specifies number of slices in *this* file,\n"
                "                            overriding file headers.\n");
    help->pagef("\n"
                "Output options:\n"
                "  -l  --log-file            File to log to.  If not provided, messages will be\n"
                "                            printed to stderr.\n"
                "  -o  --output-file         File to which to output text memcached protocol\n"
                "                            messages.  This file must not already exist.\n");
    help->pagef("                            Defaults to \"%s\"\n", EXTRACT_CONFIG_DEFAULT_OUTPUT_FILE);
    help->pagef("\n"
                "Extract is useful both for recovering data from a corrupted database and exporting data\n"
                "to another system. Unlike serve extract need not be run on all of the files made with a\n"
                "create command it can be run on them individually. Furthermore it can be run on files made with\n"
                "distinct create commands.\n"
                "\n"
                "--force-block-size, --force-extent-size and --force-slice-count should be used if the file is corrupted\n"
                "to a point that these values are not available (it will tell you when this is the case).\n");
    exit(0);        
}

enum { force_block_size = 256,  // Start these values above the ASCII range.
       force_extent_size,
       force_mod_count
};

void parse_cmd_args(int argc, char **argv, config_t *config) {
    config->init();

    if (argc >= 2) {
        if (!strncmp("help", argv[1], 4)) {
            usage(argv[0]);
        }
    }

    optind = 1;  // reinit getopt.
    for (;;) {
        int do_help = 0;
        static const struct option long_options[] =
            {
                {"force-block-size", required_argument, 0, force_block_size},
                {"force-extent-size", required_argument, 0, force_extent_size},
                {"force-slice-count", required_argument, 0, force_mod_count},

                {"file", required_argument, 0, 'f'},
                {"log-file", required_argument, 0, 'l'},
                {"output-file", required_argument, 0, 'o'},
                {"help", no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f:l:o:h", long_options, &option_index);

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
            config->input_files.push_back(optarg);
            break;
        case 'l':
            config->log_file_name = optarg;
            break;
        case 'o':
            config->output_file = optarg;
            break;
        case force_block_size: {
            char *endptr;
            long bs = strtol(optarg, &endptr, 10);
            config->overrides.block_size_ = bs;
            if (*endptr != '\0' || bs <= 0) {
                fail_due_to_user_error("Block size must be a positive integer.\n");
            }
        } break;
        case force_extent_size: {
            char *endptr;
            config->overrides.extent_size = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || config->overrides.extent_size <= 0) {
                fail_due_to_user_error("Extent size must be a positive integer.\n");
            }
        } break;
        case force_mod_count: {
            char *endptr;
            config->overrides.mod_count = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || config->overrides.mod_count <= 0) {
                fail_due_to_user_error("The mod count must be a positive integer.\n");
            }
        } break;
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

    // Sanity-check the input.

    if (config->input_files.empty()) {
        fail_due_to_user_error("A path must be specified with -f.");
    }

    if (config->overrides.any() && config->input_files.size() >= 2) {
        fail_due_to_user_error("--force-* options can only be used with one file at a time.");
    }
}

}  // namespace extract

int run_extract(int argc, char **argv) {
    extract::config_t config;
    extract::parse_cmd_args(argc, argv, &config);

    if (config.log_file_name != "") {
        log_file = fopen(config.log_file_name.c_str(), "w");
    }

    extract::dumpfile(config);

    return 0;
}
