
#ifndef __SERIALIZER_STATIC_HEADER_HPP__
#define __SERIALIZER_STATIC_HEADER_HPP__

#include "arch/arch.hpp"

struct static_header_t {
    char software_name[16];
    char version[16];
    byte_t data[0];
};

struct static_header_check_callback_t {
    virtual void on_static_header_check(bool is_valid) = 0;
};

void static_header_check(direct_file_t *file, static_header_check_callback_t *cb);

struct static_header_write_callback_t {
    virtual void on_static_header_write() = 0;
};

bool static_header_write(direct_file_t *file, void *data, size_t data_size, static_header_write_callback_t *cb);

struct static_header_read_callback_t {
    virtual void on_static_header_read() = 0;
};

bool static_header_read(direct_file_t *file, void *data_out, size_t data_size, static_header_read_callback_t *cb);

#endif /* __SERIALIZER_STATIC_HEADER_HPP__ */
