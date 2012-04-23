#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string>

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>



/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
boost::uuids::uuid generate_uuid();

std::string uuid_to_str(boost::uuids::uuid id);

boost::uuids::uuid str_to_uuid(const std::string&);



#endif  // CONTAINERS_UUID_HPP_
