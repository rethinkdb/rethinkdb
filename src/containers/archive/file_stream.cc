// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/archive/file_stream.hpp"

blocking_read_file_stream_t::blocking_read_file_stream_t() : fp_(NULL) { }

blocking_read_file_stream_t::~blocking_read_file_stream_t() {
    if (fp_) {
        fclose(fp_);
    }
}

bool blocking_read_file_stream_t::init(const char *path) {
    rassert(!fp_);
    fp_ = fopen(path, "rb");
    return fp_ != NULL;
}

int64_t blocking_read_file_stream_t::read(void *p, int64_t n) {
    rassert(fp_);

    size_t num_read = fread(p, 1, n, fp_);

    if (int64_t(num_read) < n) {
        if (feof(fp_)) {
            return num_read;
        } else {
            return -1;
        }
    } else {
        rassert(int64_t(num_read) == n);
        return num_read;
    }
}






blocking_write_file_stream_t::blocking_write_file_stream_t() : fp_(NULL) { }

blocking_write_file_stream_t::~blocking_write_file_stream_t() {
    if (fp_) {
        fclose(fp_);
    }
}

bool blocking_write_file_stream_t::init(const char *path) {
    rassert(!fp_);
    fp_ = fopen(path, "wb");
    return fp_ != NULL;
}

int64_t blocking_write_file_stream_t::write(const void *p, int64_t n) {
    rassert(fp_);

    size_t num_written = fwrite(p, 1, n, fp_);

    if (int64_t(num_written) < n) {
        return -1;
    }

    rassert(int64_t(num_written) == n);

    int res = fflush(fp_);
    if (res) {
        return -1;
    }

    return n;
}
