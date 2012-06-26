#include "containers/uuid.hpp"

#include <string.h>

#include "errors.hpp"
#define BOOST_UUID_NO_TYPE_TRAITS
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "containers/printf_buffer.hpp"

#include "utils.hpp"

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

uuid_t generate_uuid() {
#ifndef VALGRIND
    return from_boost_uuid(boost::uuids::random_generator()());
#else
    uuid_t ret;
    uint8_t *dat = ret.data();
    for (size_t i = 0; i < ret.static_size(); i++) {
        dat[i] = static_cast<uint8_t>(randint(256));
    }
    return ret;
#endif
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
