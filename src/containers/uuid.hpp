#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string>

#include "errors.hpp"
#define BOOST_UUID_NO_TYPE_TRAITS
#include <boost/uuid/uuid.hpp>

class append_only_printf_buffer_t;


/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
boost::uuids::uuid generate_uuid();

// Returns boost::uuids::nil_generator()().
boost::uuids::uuid nil_uuid();

void debug_print(append_only_printf_buffer_t *buf, const boost::uuids::uuid& id);

std::string uuid_to_str(boost::uuids::uuid id);

boost::uuids::uuid str_to_uuid(const std::string&);

bool is_uuid(const std::string& str);


#endif  // CONTAINERS_UUID_HPP_
