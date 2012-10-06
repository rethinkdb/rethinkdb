#include "serializer/log/static_header.hpp"

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "config/args.hpp"


bool static_header_check(direct_file_t *file) {
    if (!file->exists() || file->get_size() < DEVICE_BLOCK_SIZE) {
        return false;
    } else {
        static_header_t *buffer = reinterpret_cast<static_header_t *>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
        co_read(file, 0, DEVICE_BLOCK_SIZE, buffer, DEFAULT_DISK_ACCOUNT);
        bool equals = memcmp(buffer, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) == 0;
        free(buffer);
        return equals;
    }
}

void co_static_header_write(direct_file_t *file, void *data, size_t data_size) {
    static_header_t *buffer = reinterpret_cast<static_header_t *>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);

    file->set_size_at_least(DEVICE_BLOCK_SIZE);

    bzero(buffer, DEVICE_BLOCK_SIZE);

    rassert(sizeof(SOFTWARE_NAME_STRING) < 16);
    memcpy(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING));

    rassert(sizeof(VERSION_STRING) < 16);
    memcpy(buffer->version, VERSION_STRING, sizeof(VERSION_STRING));

    memcpy(buffer->data, data, data_size);

    co_write(file, 0, DEVICE_BLOCK_SIZE, buffer, DEFAULT_DISK_ACCOUNT);

    free(buffer);
}

void co_static_header_write_helper(direct_file_t *file, static_header_write_callback_t *cb, void *data, size_t data_size) {
    co_static_header_write(file, data, data_size);
    cb->on_static_header_write();
}

bool static_header_write(direct_file_t *file, void *data, size_t data_size, static_header_write_callback_t *cb) {
    coro_t::spawn(boost::bind(co_static_header_write_helper, file, cb, data, data_size));
    return false;
}

void co_static_header_read(direct_file_t *file, static_header_read_callback_t *callback, void *data_out, size_t data_size) {
    rassert(sizeof(static_header_t) + data_size < DEVICE_BLOCK_SIZE);
    rassert(file->exists());
    static_header_t *buffer = reinterpret_cast<static_header_t *>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
    co_read(file, 0, DEVICE_BLOCK_SIZE, buffer, DEFAULT_DISK_ACCOUNT);
    if (memcmp(buffer->software_name, SOFTWARE_NAME_STRING, sizeof(SOFTWARE_NAME_STRING)) != 0) {
        fail_due_to_user_error("This doesn't appear to be a RethinkDB data file.");
    }

    if (memcmp(buffer->version, VERSION_STRING, sizeof(VERSION_STRING)) != 0) {
        fail_due_to_user_error("File version is incorrect. This file was created with version %s of RethinkDB, "
            "but you are trying to read it with version %s.", buffer->version, VERSION_STRING);
    }
    memcpy(data_out, buffer->data, data_size);
    callback->on_static_header_read();
    // TODO: free buffer before you call the callback.
    free(buffer);
}

bool static_header_read(direct_file_t *file, void *data_out, size_t data_size, static_header_read_callback_t *cb) {
    coro_t::spawn(boost::bind(co_static_header_read, file, cb, data_out, data_size));
    return false;
}
