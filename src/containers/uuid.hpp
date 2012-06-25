#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string>

#include "errors.hpp"
#define BOOST_UUID_NO_TYPE_TRAITS
#include <boost/uuid/uuid.hpp>

class append_only_printf_buffer_t;

class uuid_t {
public:
    uuid_t() { }
    explicit uuid_t(const boost::uuids::uuid& uuid) : uuid_(uuid) { }

    const boost::uuids::uuid& uuid() const { return uuid_; }

    bool is_nil() const { return uuid_.is_nil(); }

    static size_t static_size() {
        CT_ASSERT(sizeof(uuid_t) == sizeof(boost::uuids::uuid));
        return sizeof(uuid_t);
    }

    uint8_t *data() { return uuid_.data; }
    const uint8_t *data() const { return uuid_.data; }

private:
    boost::uuids::uuid uuid_;
};


bool operator==(const uuid_t& x, const uuid_t& y);
inline bool operator!=(const uuid_t& x, const uuid_t& y) { return !(x == y); }
bool operator<(const uuid_t& x, const uuid_t& y);

/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
uuid_t generate_uuid();

// Returns boost::uuids::nil_generator()().
uuid_t nil_uuid();

void debug_print(append_only_printf_buffer_t *buf, const uuid_t& id);

std::string uuid_to_str(uuid_t id);

uuid_t str_to_uuid(const std::string&);

bool is_uuid(const std::string& str);


#endif  // CONTAINERS_UUID_HPP_
