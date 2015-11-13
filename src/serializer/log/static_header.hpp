// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_STATIC_HEADER_HPP_
#define SERIALIZER_LOG_STATIC_HEADER_HPP_

#include <stddef.h>
#include "arch/types.hpp"

struct static_header_t {
    char software_name[16];
    char version[16];
    char data[0];
};

bool static_header_check(file_t *file);

struct static_header_write_callback_t {
    virtual void on_static_header_write() = 0;
    virtual ~static_header_write_callback_t() {}
};

void co_static_header_write(file_t *file, void *data, size_t data_size);

bool static_header_write(
    file_t *file,
    void *data,
    size_t data_size,
    static_header_write_callback_t *cb);

struct static_header_read_callback_t {
    virtual void on_static_header_read() = 0;
    virtual ~static_header_read_callback_t() {}
};

void static_header_read(
    file_t *file,
    void *data_out,
    size_t data_size,
    bool *needs_migration_out,
    static_header_read_callback_t *cb);

// Blocks, must be run in a coroutine
void migrate_static_header(file_t *file, size_t data_size);

#endif /* SERIALIZER_LOG_STATIC_HEADER_HPP_ */
