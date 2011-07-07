#include "http/json.hpp"

namespace json {

std::string print_visitor::operator()(json_num_t num) const {
    std::ostringstream res;
    res << num;
    return res.str();
}

std::string print_visitor::operator()(json_str_t str) const { 
    return "\"" + str + "\"";
}

std::string print_visitor::operator()(json_bool_t b) const { 
    if (b) 
        return "true";
    else
        return "false";
}

std::string print_visitor::operator()(json_array_t array) const { 
    std::string res = "[";
    for (json_array_t::iterator it = array.begin(); it != array.end(); it++) {
        res += boost::apply_visitor(print_visitor(), *it);
        res += ",";
    }
    res += "]";
    return res;
}

std::string print_visitor::operator()(json_object_t object) const { 
    std::string res = "{";
    for (json_object_t::iterator it = object.begin(); it != object.end(); it++) {
        res += print_visitor()(it->first) + ": " + boost::apply_visitor(print_visitor(), it->second);
        res += ", ";
    }
    res += "}";
    return res;
}

std::string print_visitor::operator()(json_null_t) const { 
    return "null";
}

std::string print(const json_t &json) {
    return print_visitor()(json);
}

}; //namespace json
