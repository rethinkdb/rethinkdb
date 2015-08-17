// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_SCOPED_REGEX_HPP_
#define CONTAINERS_SCOPED_REGEX_HPP_

#include <regex.h>
#include <string>

#include "errors.hpp"

/* It's safe to reuse the REG_NOMATCH error code constant because it's
   only returned by regexec, not regcomp. */
class scoped_regex_t {
public:
    scoped_regex_t() : errcode(REG_UNCOMPILED) { }
    ~scoped_regex_t();

    MUST_USE bool compile(const std::string &pat, int flags = REG_EXTENDED | REG_NOSUB);

    bool matches(const std::string &str) const {
        rassert(is_compiled()); // Matching an uncompiled regex is an error.
        return !regexec(&r, str.c_str(), 0, 0, 0);
    }

    bool is_compiled() const { return !errcode; }
    std::string get_error();

private:
    static const int REG_UNCOMPILED = REG_NOMATCH;

    int errcode;
    regex_t r;
    DISABLE_COPYING(scoped_regex_t);
};

#endif /* CONTAINERS_SCOPED_REGEX_HPP_ */
