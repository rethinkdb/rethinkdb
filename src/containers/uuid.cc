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


std::string uuid_to_str(uuid_t id) {
    return boost::uuids::to_string(as_boost_uuid(id));
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
