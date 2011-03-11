#include "concurrency/access.hpp"

bool is_read_mode(access_t mode) {
    return mode == rwi_read || mode == rwi_read_outdated_ok;
}

bool is_write_mode(access_t mode) {
    return !is_read_mode(mode);
}


