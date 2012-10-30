// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/uuid.hpp"

#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/sha.h>

#include "errors.hpp"
#define BOOST_UUID_NO_TYPE_TRAITS
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "containers/printf_buffer.hpp"

#include "utils.hpp"
#include "thread_local.hpp"

static const char *const magic_unset_uuid = "UNSET_UUID_____";
uuid_t::uuid_t() {
    rassert(strlen(magic_unset_uuid) == kStaticSize-1);
    memcpy(data_, magic_unset_uuid, kStaticSize);
}
bool uuid_t::is_unset() const {
    return !memcmp(data_, magic_unset_uuid, kStaticSize);
}

bool uuid_t::is_nil() const {
    for (size_t i = 0; i < kStaticSize; ++i) {
        if (data_[i] != 0) return false;
    }
    return true;
}

bool operator==(const uuid_t& x, const uuid_t& y) {
    return memcmp(x.data(), y.data(), uuid_t::static_size()) == 0;
}

bool operator<(const uuid_t& x, const uuid_t& y) {
    return memcmp(x.data(), y.data(), uuid_t::static_size()) < 0;
}

uuid_t from_boost_uuid(const boost::uuids::uuid& uuid) {
    CT_ASSERT(sizeof(uuid_t) == sizeof(boost::uuids::uuid));

    uuid_t ret;
    memcpy(ret.data(), uuid.data, uuid_t::static_size());
    return ret;
}

boost::uuids::uuid as_boost_uuid(const uuid_t& uuid) {
    boost::uuids::uuid ret;
    memcpy(ret.data, uuid.data(), uuid_t::static_size());
    return ret;
}

static __thread bool next_uuid_initialized = false;
static __thread uint8_t next_uuid[uuid_t::kStaticSize];

uuid_t get_and_increment_uuid() {
    uuid_t result;

    // Copy over the next_uuid buffer
    uint8_t *result_buffer = result.data();
    memcpy(result_buffer, next_uuid, uuid_t::static_size());

    // Increment the next_uuid buffer
    bool carry = true;
    for (size_t i = uuid_t::static_size(); carry && i > 0; --i) {
        next_uuid[i - 1] = next_uuid[i - 1] + 1;
        carry = (next_uuid[i - 1] == 0);
    }

    return result;
}

// TODO(sam):  Make sure this isn't messed up somehow.
void hash_uuid(uuid_t *uuid) {
    CT_ASSERT(SHA_DIGEST_LENGTH >= uuid_t::kStaticSize);
    uint8_t output_buffer[SHA_DIGEST_LENGTH];
    SHA_CTX ctx;

    // SHA-1 hash the UUID
    guarantee(SHA1_Init(&ctx) == 1);
    guarantee(SHA1_Update(&ctx, uuid->data(), uuid_t::static_size()) == 1);
    guarantee(SHA1_Final(output_buffer, &ctx) == 1);

    // Set some bits to obey standard for version 4 UUIDs.
    output_buffer[6] = ((output_buffer[6] & 0x0f) | 0x40);
    output_buffer[8] = ((output_buffer[8] & 0x3f) | 0x80);

    // Copy the beginning of the hash into our uuid
    memcpy(uuid->data(), output_buffer, uuid_t::static_size());
}

void initialize_dev_random_uuid() {
    int random_fd = open("/dev/urandom", O_RDONLY);
    guarantee(random_fd != -1);
    ssize_t readres = read(random_fd, next_uuid, uuid_t::static_size());
    guarantee(readres == static_cast<ssize_t>(uuid_t::static_size()));
    close(random_fd);
}

uuid_t generate_uuid() {
    if (!next_uuid_initialized) {
        initialize_dev_random_uuid();
        next_uuid_initialized = true;
    }
    uuid_t result = get_and_increment_uuid();
    hash_uuid(&result);
    return result;
}

uuid_t nil_uuid() {
    uuid_t ret;
    memset(ret.data(), 0, uuid_t::static_size());
    return ret;
}

void debug_print(append_only_printf_buffer_t *buf, const uuid_t& id) {
    buf->appendf("%s", uuid_to_str(id).c_str());
}

void push_hex(std::string *s, uint8_t byte) {
    const char *buf = "0123456789abcdef";
    s->push_back(buf[byte >> 4]);
    s->push_back(buf[byte & 0x0f]);
}

std::string uuid_to_str(uuid_t id) {
    const uint8_t *data = id.data();

    std::string ret;
    ret.reserve(uuid_t::kStaticSize * 2 + 4);  // hexadecimal, 4 hyphens
    size_t i = 0;
    for (; i < 4; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 6; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 8; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 10; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    CT_ASSERT(uuid_t::kStaticSize == 16);  // This code just feels this assertion in its bones.
    for (; i < uuid_t::kStaticSize; ++i) {
        push_hex(&ret, data[i]);
    }

    return ret;
}

uuid_t str_to_uuid(const std::string& uuid) {
    return from_boost_uuid(boost::uuids::string_generator()(uuid));
}

bool is_uuid(const std::string& str) {
    try {
        str_to_uuid(str);
    } catch (...) {
        return false;
    }
    return true;
}
