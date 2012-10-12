#ifndef CONTAINERS_NAME_STRING_HPP_
#define CONTAINERS_NAME_STRING_HPP_

#include <string>

// The kind of string that can only contain either the empty string or acceptable names for
// things.

class name_string_t {
public:
    name_string_t();
    bool assign(const std::string& s);

private:
    std::string str_;
};


#endif  // CONTAINERS_NAME_STRING_HPP_
