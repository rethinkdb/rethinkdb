#ifndef HTTP_JSON_HPP_
#define HTTP_JSON_HPP_

#include <string>
#include <set>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "http/json/cJSON.hpp"
#include "containers/archive/archive.hpp"

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

class copyable_cJSON_t {
private:
    cJSON *val;
public:
    explicit copyable_cJSON_t(cJSON *);
    copyable_cJSON_t(const copyable_cJSON_t &);
    ~copyable_cJSON_t();
    cJSON *get() const;

    write_message_t &operator<<(write_message_t &msg);
    MUST_USE archive_result_t deserialize(read_stream_t *s);
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

/* Json serialization */
write_message_t &operator<<(write_message_t &msg, const cJSON &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, cJSON *cjson);

write_message_t &operator<<(write_message_t &msg, const boost::shared_ptr<scoped_cJSON_t> &cjson);
MUST_USE archive_result_t deserialize(read_stream_t *s, boost::shared_ptr<scoped_cJSON_t> *cjson);

/* Convenience function for creating shared_ptrs to scoped_cJSON */
inline boost::shared_ptr<scoped_cJSON_t> shared_scoped_json(cJSON *json) {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json));
}

#endif /* HTTP_JSON_HPP_ */
