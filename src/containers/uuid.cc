#include "containers/uuid.hpp"

#include "errors.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "containers/printf_buffer.hpp"

#include "utils.hpp"

boost::uuids::uuid generate_uuid() {
#ifndef VALGRIND
    return boost::uuids::random_generator()();
#else
    boost::uuids::uuid uuid;
    for (size_t i = 0; i < sizeof uuid.data; i++) {
        uuid.data[i] = static_cast<uint8_t>(randint(256));
    }
    return uuid;
#endif
}

boost::uuids::uuid nil_uuid() {
    return boost::uuids::nil_generator()();
}

void debug_print(append_only_printf_buffer_t *buf, const boost::uuids::uuid& id) {
    buf->appendf("%s", boost::uuids::to_string(id).c_str());
}


std::string uuid_to_str(boost::uuids::uuid id) {
    return boost::uuids::to_string(id);
}

boost::uuids::uuid str_to_uuid(const std::string& uuid) {
    return boost::uuids::string_generator()(uuid);
}

bool is_uuid(const std::string& str) {
    try {
        str_to_uuid(str);
    } catch (...) {
        return false;
    }
    return true;
}
