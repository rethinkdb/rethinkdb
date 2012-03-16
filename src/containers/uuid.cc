#include "containers/uuid.hpp"

#include <sstream>

#include "errors.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

std::string uuid_to_str(boost::uuids::uuid id) {
    std::stringstream ss;
    ss << id;
    return ss.str();
}

boost::uuids::uuid str_to_uuid(const std::string& uuid) {
    return boost::uuids::string_generator()(uuid);
}


