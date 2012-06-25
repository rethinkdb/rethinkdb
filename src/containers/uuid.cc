#include "containers/uuid.hpp"

#include "errors.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "containers/printf_buffer.hpp"

#include "utils.hpp"

bool operator==(const uuid_t& x, const uuid_t& y) {
    return x.uuid() == y.uuid();
}

bool operator<(const uuid_t& x, const uuid_t& y) {
    return x.uuid() < y.uuid();
}


uuid_t generate_uuid() {
#ifndef VALGRIND
    return uuid_t(boost::uuids::random_generator()());
#else
    boost::uuids::uuid uuid;
    for (size_t i = 0; i < sizeof uuid.data; i++) {
        uuid.data[i] = static_cast<uint8_t>(randint(256));
    }
    return uuid_t(uuid);
#endif
}

uuid_t nil_uuid() {
    return uuid_t(boost::uuids::nil_generator()());
}

void debug_print(append_only_printf_buffer_t *buf, const uuid_t& id) {
    buf->appendf("%s", uuid_to_str(id).c_str());
}


std::string uuid_to_str(uuid_t id) {
    return boost::uuids::to_string(id.uuid());
}

uuid_t str_to_uuid(const std::string& uuid) {
    return uuid_t(boost::uuids::string_generator()(uuid));
}

bool is_uuid(const std::string& str) {
    try {
        str_to_uuid(str);
    } catch (...) {
        return false;
    }
    return true;
}
