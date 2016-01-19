// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/log/static_header.hpp"

#include <functional>
#include <vector>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "containers/scoped.hpp"
#include "config/args.hpp"
#include "logger.hpp"
#include "utils.hpp"

// The CURRENT_SERIALIZER_VERSION_STRING might remain unchanged for a while --
// individual metablocks have a disk_format_version field that can be incremented
// for on-the-fly version updating.
#define CURRENT_SERIALIZER_VERSION_STRING "2.2"

// Since 1.13, we added the aux block ID space. We can still read 1.13 serializer
// files, but previous versions of RethinkDB cannot read 2.2+ files.
#define V1_13_SERIALIZER_VERSION_STRING "1.13"

// See also CLUSTER_VERSION_STRING and cluster_version_t.

bool static_header_check(file_t *file) {
    if (file->get_file_size() < DEVICE_BLOCK_SIZE) {
        return false;
    } else {
        scoped_device_block_aligned_ptr_t<static_header_t> buffer(DEVICE_BLOCK_SIZE);
        co_read(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT);
        bool equals = memcmp(buffer.get(), SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) == 0;
        return equals;
    }
}

void co_static_header_write(file_t *file, void *data, size_t data_size) {
    scoped_device_block_aligned_ptr_t<static_header_t> buffer(DEVICE_BLOCK_SIZE);
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);

    file->set_file_size_at_least(DEVICE_BLOCK_SIZE);

    memset(buffer.get(), 0, DEVICE_BLOCK_SIZE);

    rassert(sizeof(SOFTWARE_NAME_STRING) < 16);
    memcpy(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));

    rassert(sizeof(CURRENT_SERIALIZER_VERSION_STRING) < 16);
    memcpy(buffer->version, CURRENT_SERIALIZER_VERSION_STRING,
           sizeof(CURRENT_SERIALIZER_VERSION_STRING));

    memcpy(buffer->data, data, data_size);

    // We want to follow up the static header write with a datasync, because... it's the
    // most important block in the file!
    co_write(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT, file_t::WRAP_IN_DATASYNCS);
}

void co_static_header_write_helper(file_t *file, static_header_write_callback_t *cb, void *data, size_t data_size) {
    co_static_header_write(file, data, data_size);
    cb->on_static_header_write();
}

bool static_header_write(file_t *file, void *data, size_t data_size, static_header_write_callback_t *cb) {
    coro_t::spawn_later_ordered(std::bind(co_static_header_write_helper, file, cb, data, data_size));
    return false;
}

void co_static_header_read(
        file_t *file,
        static_header_read_callback_t *callback,
        void *data_out,
        size_t data_size,
        bool *needs_migration_out) {
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);
    scoped_device_block_aligned_ptr_t<static_header_t> buffer(DEVICE_BLOCK_SIZE);
    co_read(file, 0, DEVICE_BLOCK_SIZE, buffer.get(), DEFAULT_DISK_ACCOUNT);
    if (memcmp(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) != 0) {
        fail_due_to_user_error("This doesn't appear to be a RethinkDB data file.");
    }

    if (memcmp(buffer->version, V1_13_SERIALIZER_VERSION_STRING,
               sizeof(V1_13_SERIALIZER_VERSION_STRING)) == 0) {
        *needs_migration_out = true;
    } else if (memcmp(buffer->version, CURRENT_SERIALIZER_VERSION_STRING,
               sizeof(CURRENT_SERIALIZER_VERSION_STRING)) == 0) {
        *needs_migration_out = false;
    } else {
        fail_due_to_user_error("File version is incorrect. This file was created with "
                               "RethinkDB's serializer version %s, but you are trying "
                               "to read it with version %s.  See "
                               "http://rethinkdb.com/docs/migration/ for information on "
                               "migrating data from a previous version.",
                               buffer->version, CURRENT_SERIALIZER_VERSION_STRING);
    }
    memcpy(data_out, buffer->data, data_size);
    callback->on_static_header_read();
}

void static_header_read(
        file_t *file,
        void *data_out,
        size_t data_size,
        bool *needs_migration_out,
        static_header_read_callback_t *cb) {
    coro_t::spawn_later_ordered(std::bind(co_static_header_read,
        file,
        cb,
        data_out,
        data_size,
        needs_migration_out));
}

void migrate_static_header(file_t *file, size_t data_size) {
    // Migrate the static header by rewriting it
    logNTC("Migrating file to serializer version %s.",
           CURRENT_SERIALIZER_VERSION_STRING);

    std::vector<char> data(data_size);

    struct noop_cb_t : public static_header_read_callback_t {
        void on_static_header_read() { }
    } noop_cb;
    bool needs_migration;
    co_static_header_read(file,
        &noop_cb,
        data.data(),
        data_size,
        &needs_migration);
    guarantee(needs_migration);

    co_static_header_write(file, data.data(), data_size);
}
