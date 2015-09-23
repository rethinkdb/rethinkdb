// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/log/static_header.hpp"

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "config/args.hpp"
#include "containers/scoped.hpp"


bool static_header_check(file_t *file) {
    if (file->get_file_size() < DEVICE_BLOCK_SIZE) {
        return false;
    } else {
        scoped_aligned_malloc_t<static_header_t> buffer = malloc_aligned<static_header_t>(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
        co_read(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT);
        bool equals = memcmp(buffer.get(), SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) == 0;
        buffer.reset();
        return equals;
    }
}

void co_static_header_write(file_t *file, void *data, size_t data_size) {
    scoped_aligned_malloc_t<static_header_t> buffer = malloc_aligned<static_header_t>(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);

    file->set_file_size_at_least(DEVICE_BLOCK_SIZE);

    memset(buffer.get(), 0, DEVICE_BLOCK_SIZE);

    rassert(sizeof(SOFTWARE_NAME_STRING) < 16);
    memcpy(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));

    rassert(sizeof(SERIALIZER_VERSION_STRING) < 16);
    memcpy(buffer->version, SERIALIZER_VERSION_STRING, sizeof(SERIALIZER_VERSION_STRING));

    memcpy(buffer->data, data, data_size);

    // We want to follow up the static header write with a datasync, because... it's the
    // most important block in the file!
    co_write(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT, file_t::WRAP_IN_DATASYNCS);

    buffer.reset();
}

void co_static_header_write_helper(file_t *file, static_header_write_callback_t *cb, void *data, size_t data_size) {
    co_static_header_write(file, data, data_size);
    cb->on_static_header_write();
}

bool static_header_write(file_t *file, void *data, size_t data_size, static_header_write_callback_t *cb) {
    coro_t::spawn_later_ordered(boost::bind(co_static_header_write_helper, file, cb, data, data_size));
    return false;
}

void co_static_header_read(file_t *file, static_header_read_callback_t *callback, void *data_out, size_t data_size) {
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);
    scoped_aligned_malloc_t<static_header_t> buffer = malloc_aligned<static_header_t>(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE);
    co_read(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT);
    if (memcmp(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) != 0) {
        fail_due_to_user_error("This doesn't appear to be a RethinkDB data file.");
    }

    if (memcmp(buffer->version, SERIALIZER_VERSION_STRING, sizeof(SERIALIZER_VERSION_STRING)) != 0) {
        fail_due_to_user_error("File version is incorrect. This file was created with "
                               "RethinkDB's serializer version %s, but you are trying "
                               "to read it with version %s.  See "
                               "http://rethinkdb.com/docs/migration/ for information on "
                               "migrating data from a previous version.",
                               buffer->version, SERIALIZER_VERSION_STRING);
    }
    memcpy(data_out, buffer->data, data_size);
    callback->on_static_header_read();
    // TODO: free buffer before you call the callback.
    buffer.reset();
}

bool static_header_read(file_t *file, void *data_out, size_t data_size, static_header_read_callback_t *cb) {
     coro_t::spawn_later_ordered(boost::bind(co_static_header_read, file, cb, data_out, data_size));
    return false;
}
