// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/base64.hpp"

#include "rdb_protocol/error.hpp"
#include "errors.hpp"

const char base64_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void binary_to_base64_chunk(const char *in, char *out) {
    CT_ASSERT(sizeof(base64_map) == 65);
    out[0] = base64_map[(in[0] & 0xFC) >> 2];
    out[1] = base64_map[((in[0] & 0x03) << 4) + ((in[1] & 0xF0) >> 4)];
    out[2] = base64_map[((in[1] & 0x0F) << 2) + ((in[2] & 0xC0) >> 6)];
    out[3] = base64_map[in[2] & 0x3F];
}

std::string encode_base64(const char *data, size_t size) {
    size_t remaining_bytes = size;

    if (remaining_bytes == 0) {
        return std::string();
    }

    std::string res;
    // Scale output size for base64 encoding and account for a CRLF every 76 characters
    res.reserve(((remaining_bytes * 52) + 39) / 38);

    char encoded_chunk[4];
    const char *chunk = data;

    for (; remaining_bytes > 3;
         remaining_bytes -= 3) {
        binary_to_base64_chunk(chunk, encoded_chunk);
        res.append(encoded_chunk, 4);
        if (res.size() % 78 == 76) {
            res.append("\r\n");
        }
        chunk += 3;
    }

    char partial_chunk[4] = { 0, 0, 0, 0 };
    for (size_t i = 0; i < remaining_bytes; ++i) {
        partial_chunk[i] = chunk[i];
    }
    binary_to_base64_chunk(partial_chunk, encoded_chunk);

    for (size_t i = remaining_bytes + 1; i < 4; ++i) {
        encoded_chunk[i] = '=';
    }
    res.append(encoded_chunk, 4);
    return res;
}

inline char base64_value(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    } else if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    } else if (c == '+') {
        return 62;
    } else if (c == '/') {
        return 63;
    }
    rfail_datum(ql::base_exc_t::GENERIC, "Invalid base64 character found: '%c'.", c);
}

// Takes a chunk of four 6-bit values and converts it into 3 8-bit values
inline void base64_chunk_to_binary(const char *in, char *out) {
    out[0] = (in[0] << 2) + (in[1] >> 4);
    out[1] = ((in[1] & 0xF) << 4) + (in[2] >> 2);
    out[2] = ((in[2] & 0x3) << 6) + in[3];
}

const char* fill_chunk_values(const char *in, const char *in_end, char *out,
                              bool *done, size_t *chars_filled) {
    *done = false;
    *chars_filled = 0;
    while (*chars_filled < 4 && !*done) {
        if (in == in_end || *in == '=') {
            *done = true;
            return in;
        } else if (*in != '\r' && *in != '\n' && *in != ' ' && *in != '\t') {
            *out++ = base64_value(*in);
            *chars_filled += 1;
        }
        in++;
    }
    return in;
}

std::string decode_base64(const char *bdata, size_t bsize) {
    // This assumes no whitespace in the input, so we may be overallocating a bit
    std::string res;
    res.reserve((bsize / 4) * 3);

    bool done = false;
    size_t chars_filled;
    char chunk_values[4];
    char decoded_chunk[3];
    const char *current_data = bdata;
    const char *data_end = bdata + bsize;

    while (!done) {
        current_data = fill_chunk_values(current_data, data_end, chunk_values,
                                         &done, &chars_filled);
        base64_chunk_to_binary(chunk_values, decoded_chunk);

        if (chars_filled == 1) {
            rfail_datum(ql::base_exc_t::GENERIC,
                        "Invalid base64 length: 1 character remaining, "
                        "cannot decode a full byte.");
        } else if (chars_filled != 0) {
            res.append(decoded_chunk, chars_filled - 1);
        }
    }

    // Check if we stopped early (due to a stray padding character)
    for (const char *i = current_data; i != data_end; ++i) {
        if (*i != '=' && *i != '\r' && *i != '\n' && *i != ' ' && *i != '\t') {
            rfail_datum(ql::base_exc_t::GENERIC,
                        "Invalid base64 format, data found after "
                        "padding character '='.");
        }
    }

    return res;
}
