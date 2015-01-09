// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PARSING_UTF8_HPP_
#define PARSING_UTF8_HPP_

#include <string>

class datum_string_t;

namespace utf8 {

// Simple UTF-8 validation.
bool is_valid(const std::string &);
bool is_valid(const char *, const char *);
bool is_valid(const datum_string_t &);

struct reason_t {
    const char *explanation;
    size_t position;
};

// UTF-8 validation with an explanation
bool is_valid(const std::string &, reason_t *);
bool is_valid(const char *, const char *, reason_t *);
bool is_valid(const datum_string_t &, reason_t *);

}

#endif /* PARSING_UTF8_HPP_ */
