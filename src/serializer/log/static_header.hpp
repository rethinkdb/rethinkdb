
#ifndef __SERIALIZER_STATIC_HEADER_HPP__
#define __SERIALIZER_STATIC_HEADER_HPP__

#include "config/args.hpp"
#include <string.h>

struct static_header {
    public:
        char software_name[sizeof(SOFTWARE_NAME_STRING)];
        char version[sizeof(VERSION_STRING)];

        static_header() {
            mempcpy(software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));
            mempcpy(version, VERSION_STRING, sizeof(VERSION_STRING));
}

#endif
