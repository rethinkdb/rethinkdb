#include <getopt.h>
#include <stdio.h>

#include "fsck/checker.hpp"
#include "logger.hpp"
#include "utils.hpp"

namespace fsck {

void usage(const char *name) {
    printf("Usage:\n");
    printf("        %s [OPTIONS] -f data_file_1 -f data_file_2 ...\n", name);
    printf("\nOptions:\n"
           "  -h  --help                Print these usage options.\n"
           "  -f  --file                Path to file or block device where part or all of\n"
           "                            the database exists.\n"
           "  -l  --log-file            File to log to.  If not provided, messages will be\n"
           "                            printed to stderr.\n");

    exit(-1);
}

void parse_cmd_args(int argc, char **argv, config_t *config) {

    // TODO disallow rogue options.

    optind = 1;  // reinit getopt.
    for (;;) {
        int do_help = 0;
        const static struct option long_options[] =
            {
                {"file", required_argument, 0, 'f'},
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

    if (config->input_filenames.size() == 0) {
        fprintf(stderr, "Please specify some files.");
        usage(argv[0]);
    }
}


}  // namespace fsck

int main(int argc, char **argv) {
    install_generic_crash_handler();

    fsck::config_t config;
    fsck::parse_cmd_args(argc, argv, &config);

    if (config.log_file_name != "") {
        log_file = fopen(config.log_file_name.c_str(), "w");
    }

    check_files(config);
}
