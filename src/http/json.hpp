#ifndef HTTP_JSON_HPP_
#define HTTP_JSON_HPP_

#include <string>
#include <set>

#include "errors.hpp"
#include "http/json/cJSON.hpp"
#include "http/http.hpp"

http_res_t http_json_res(cJSON *json);

class scoped_cJSON_t {
private:
    cJSON *val;

public:
    explicit scoped_cJSON_t(cJSON *);
    ~scoped_cJSON_t();
    cJSON *get() const;
    cJSON *release();
    void reset(cJSON *);
};

class json_iterator_t {
public:
    explicit json_iterator_t(cJSON *target);

    cJSON *next();
private:
    cJSON *node;
};

class json_object_iterator_t : public json_iterator_t {
public:
    explicit json_object_iterator_t(cJSON *target);
};

class json_array_iterator_t : public json_iterator_t {
public:
    explicit json_array_iterator_t(cJSON *target);
};

std::string cJSON_print_std_string(cJSON *json);
std::string cJSON_print_unformatted_std_string(cJSON *json);

void project(cJSON *json, std::set<std::string> keys);

//Merge two cJSON objects, crashes if there are overlapping keys
cJSON *merge(cJSON *, cJSON *);

#endif /* HTTP_JSON_HPP_ */
