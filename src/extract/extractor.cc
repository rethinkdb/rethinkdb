#include "extract/extract.hpp"
#include <string.h>

#ifdef EXTRACTOR

int main(int argc, char **argv) {
    strcpy(argv[0], "extract");
    return run_extract(argc, argv);
}

#endif
