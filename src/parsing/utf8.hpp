// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PARSING_UTF8_HPP_
#define PARSING_UTF8_HPP_

#include <string>

class datum_string_t;

namespace utf8 {

// Simple utf8 validation.
bool is_valid(const std::string &);
bool is_valid(const char *);
bool is_valid(const datum_string_t &);

}

#endif /* PARSING_UTF8_HPP_ */
