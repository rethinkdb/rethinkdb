#include <stdio.h>
#include <stdlib.h>

#include "filecheck/filewalk.hpp"

namespace {
    void usage(const char *name) {
        printf("Usage:\n");
        printf("\t%s dbfile dumpfile\n", name);

        exit(-1);
    }


    // TODO: rename this to main, compile separate executable.
    int checkmain(int argc, char **argv) {
        if (argc != 3) {
            usage(argv[0]);
        }

        const char *db_filepath = argv[1];
        const char *dump_filepath = argv[2];

        dumpfile(db_filepath, dump_filepath);
        return 0;
    }

}
