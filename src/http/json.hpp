#ifndef __HTTP_JSON_HPP__
#define __HTTP_JSON_HPP__

//#include "http/json/json_spirit_reader_template.h"
//#include "http/json/json_spirit_writer_template.h"
#include "http/json/cJSON.hpp"

class scoped_cJSON_t {
private:
    cJSON *val;

public:
    explicit scoped_cJSON_t(cJSON *);
    ~scoped_cJSON_t();
    cJSON *get();
};

#endif
