// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "migrate/migrate.hpp"

#include <getopt.h>
#include <string.h>

#include <string>

#include "errors.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "help.hpp"

namespace migrate {

NORETURN void usage(UNUSED const char *name) {
    help_pager_t *help = help_pager_t::instance();
    help->pagef("Usage:\n"
                "        rethinkdb migrate --in -f <file_1> [-f <file_2> ...] --out -f <file_1> [-f <file_2>] [--intermediate file]\n");
    help->pagef("\n"
                "Migrate legacy versions of RethinkDB data files to the current version.\n"
                "\n"
                "Options:\n"
                "      --in              Specify the input file flags -f (or --file) following\n"
                "                        this command. This is the older version of the database\n"
                "                        to be migrated.\n"
                "      --out             Specify the output file flags -f (or --file) following\n"
                "                        this command. This is where the database will be written\n"
                "                        to in a new format.\n"
                "      -f, --file        Path to a file or block device that constitutes\n"
                "                        either the input or the output database.\n"
                "      --intermediate    File to store intermediate raw memcached commands.\n"
                "                        It defaults to: %s.\n", TEMP_MIGRATION_FILE);
    help->pagef("      --force           Allow migrate to overwrite an existing database file.\n");
    help->pagef("\n"
                "Migration extracts data from the old database into a portable format of raw\n"
                "memcached commands and then reinserts the data into a new file version being\n"
                "migrated to.\n"
                "Migration can be done from a set of files to themselves. Effectively migrating\n"
                "in place. This requires a --force flag.\n"
                "Note: if migration in place (using the --force flag) is interrupted it has the\n"
                "potential to leave the target files with missing data. Should this happen the\n"
                "intermediate file will be the only remaining copy of the data. Please consult\n"
                "support for help getting this data back in to a database.\n");
    exit(EXIT_SUCCESS);
}


void parse_cmd_args(int argc, char **argv, config_t *config) {
    // TODO disallow rogue options.

    //Grab the exec-name
    config->exec_name = argv[0];
    argc--;
    argv++;

    enum reading_list_t {
        in = 0,
        out,
        unset
    } reading_list;

    reading_list = unset;

    optind = 1;  // reinit getopt.
    if (argc >= 2) {
        if (!strncmp("help", argv[1], 4)) {
            usage(argv[0]);
        }
    }
    for (;;) {
        int do_help = 0;

        /* Which list a -f argument goes into */
        int switch_to_reading_in_files = 0;
        int switch_to_reading_out_files = 0;

        static const struct option long_options[] =
            {
                {"in", no_argument, &switch_to_reading_in_files, 1},
                {"out", no_argument, &switch_to_reading_out_files, 1},
                {"file", required_argument, 0, 'f'},
                {"intermediate", required_argument, 0, 'i'},
                {"force", no_argument, &(config->force), 1},
                {"help", no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f:i:", long_options, &option_index);

        if (do_help) {
            c = 'h';
        }
        if (switch_to_reading_in_files) {
            reading_list = in;
        }
        if (switch_to_reading_out_files) {
            reading_list = out;
        }

        // Detect the end of the options.
        if (c == -1)
            break;

        switch (c) {
        case 0:
            break;
        case 'f':
            if (reading_list == in)
                config->input_filenames.push_back(optarg);
            else if (reading_list == out)
                config->output_filenames.push_back(optarg);
            else
                fail_due_to_user_error("-f argument must follow either a --in or a --out flag\n");
            break;
        case 'i':
            config->intermediate_file = optarg;
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
        fprintf(stderr, "At least one input file must be specified.\n");
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

std::string escape_spaces(const std::string& str) {
    std::string result;
    size_t len = str.length();
    size_t pos = 0;
    do {
        size_t space_pos = str.find(' ', pos);
        bool found = space_pos != std::string::npos;
        space_pos = found ? space_pos : len;
        result.append(str, pos, space_pos-pos);
        if (found)
            result.append("\\ ");
        pos = space_pos + 1;
    } while (pos < len);
    return result;
}

int run_migrate(int argc, char **argv) {

    migrate::config_t cfg;
    migrate::parse_cmd_args(argc, argv, &cfg);

    //BREAKPOINT;
    std::vector<std::string> command_line;
#ifdef MIGRATION_SCRIPT_LOCATION //Defined in src/Makefile
    //TODO check that the script exists and give a worthwhile error message
    command_line.push_back(MIGRATION_SCRIPT_LOCATION);
#else
    crash("Trying to run migration without a specified script location.\n"
          "This probably means that you're trying to run migration\n"
          "from a binary that wasn't installed from a package. That\n"
          "or the Makefile has been messed up in some way.\n");
#endif
    command_line.push_back("-r");
    command_line.push_back(cfg.exec_name);

    for (std::vector<std::string>::iterator it = cfg.input_filenames.begin(); it != cfg.input_filenames.end(); ++it) {
        command_line.push_back("-i");
        command_line.push_back(escape_spaces(*it));
    }
    for (std::vector<std::string>::iterator it = cfg.output_filenames.begin(); it != cfg.output_filenames.end(); ++it) {
        command_line.push_back("-o");
        command_line.push_back(escape_spaces(*it));
    }

    command_line.push_back("-s");
    command_line.push_back(cfg.intermediate_file);

    if (cfg.force)
        command_line.push_back("-f");

    int res = system(boost::algorithm::join(command_line, " ").c_str());
    if (res != 0)
        fprintf(stderr, "Migration failed.\n");

    return res;
}
