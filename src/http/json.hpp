#ifndef __HTTP_JSON_HPP__
#define __HTTP_JSON_HPP__

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace json {

typedef double json_num_t;
typedef std::string json_str_t;
typedef bool json_bool_t;
typedef int json_null_t;

typedef boost::make_recursive_variant<json_num_t, 
                                      json_str_t, 
                                      json_bool_t, 
                                      std::vector<boost::recursive_variant_>, 
                                      std::map<json_str_t, boost::recursive_variant_>, 
                                      json_null_t
                                      >::type json_value_t;

typedef std::vector<json_value_t> json_array_t;
typedef std::map<json_str_t, json_value_t> json_object_t;

/* class json_value_t
    : public boost::variant<json_num_t, json_str_t, json_bool_t, json_array_t, json_object_t, json_null_t> 
{ 
public:
}; */

class print_visitor
    : public boost::static_visitor<std::string>
{
    public:
        std::string operator()(json_num_t num) const;
        std::string operator()(json_str_t str) const;
        std::string operator()(json_bool_t b) const;
        std::string operator()(json_array_t array) const;
        std::string operator()(json_object_t object) const;
        std::string operator()(json_null_t) const;
};

typedef json_object_t json_t;

}; //namespace json

#endif
