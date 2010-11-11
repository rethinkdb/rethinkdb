#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "extract/filewalk.hpp"
#include "config/cmd_args.hpp"
#include "errors.hpp"
#include "logger.hpp"

namespace extract {

void usage(const char *name) {
    // Note: some error messages may refer to the names of command
    // line options here, so keep them updated accordingly.

    printf("Usage:\n");
    printf("        %s [OPTIONS] -f data_file [-o dumpfile]\n", name);
    printf("\nOptions:\n"
           "  -h  --help                Print these usage options.\n"
           "      --force-block-size    Specifies block size, overriding file headers\n"
           "      --force-extent-size   Specifies extent size, overriding file headers\n"
           "      --force-mod-count     Specifies number of slices in *this* file,\n"
           "                            overriding file headers.\n"
           "  -f  --file                Path to file or block device where part or all of\n"
           "                            the database exists.\n"
           "  -l  --log-file            File to log to.  If not provided, messages will be\n"
           "                            printed to stderr.\n"
           "  -o  --output-file         File to which to output text memcached protocol\n"
           "                            messages.  This file must not already exist.\n");
    printf("                            Defaults to \"%s\"\n", EXTRACT_CONFIG_DEFAULT_OUTPUT_FILE);

    exit(-1);
}

enum { force_block_size = 256,  // Start these values above the ASCII range.
       force_extent_size,
       force_mod_count
};

void parse_cmd_args(int argc, char **argv, config_t *config) {
    config->init();

    optind = 1;  // reinit getopt.
    for (;;) {
        int do_help = 0;
        const static struct option long_options[] =
            {
                {"force-block-size", required_argument, 0, force_block_size},
                {"force-extent-size", required_argument, 0, force_extent_size},
                {"force-mod-count", required_argument, 0, force_mod_count},

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
            config->overrides.block_size = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || config->overrides.block_size <= 0) {
                fail("Block size must be a positive integer.\n");
            }
        } break;
        case force_extent_size: {
            char *endptr;
            config->overrides.extent_size = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || config->overrides.extent_size <= 0) {
                fail("Extent size must be a positive integer.\n");
            }
        } break;
        case force_mod_count: {
            char *endptr;
            config->overrides.mod_count = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || config->overrides.mod_count <= 0) {
                fail("The mod count must be a positive integer.\n");
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
        fail("Unexpected extra argument: \"%s\"", argv[optind]);
    }

    // Sanity-check the input.

    if (config->input_files.empty()) {
        fail("A path must be specified with -f.");
    }

    if (config->overrides.any() && config->input_files.size() >= 2) {
        fail("--force-* options can only be used with one file at a time.");
    }
}

}  // namespace extract

int main(int argc, char **argv) {

    install_generic_crash_handler();

    extract::config_t config;
    extract::parse_cmd_args(argc, argv, &config);

    if (config.log_file_name != "") {
        log_file = fopen(config.log_file_name.c_str(), "w");
    }

    extract::dumpfile(config);
}
