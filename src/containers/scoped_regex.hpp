#ifndef CONTAINERS_SCOPED_REGEX_HPP_
#define CONTAINERS_SCOPED_REGEX_HPP_

#include <regex.h>
#include <string>

#include "errors.hpp"

class scoped_regex_t {
public:
    static const int default_flags = REG_EXTENDED | REG_NOSUB;
    scoped_regex_t() : compiled(false) { }
    ~scoped_regex_t() { if (is_compiled()) regfree(&r); }

    MUST_USE int compile(const std::string &pat, int flags = default_flags) {
        rassert(!is_compiled());
        int status = regcomp(&r, pat.c_str(), flags);
        compiled = !status;
        return status;
    }

    bool matches(const std::string &str) const {
        rassert(is_compiled()); // Matching an uncompiled regex is an error.
        return !regexec(&r, str.c_str(), 0, 0, 0);
    }

    bool is_compiled() const { return compiled; }
private:
    bool compiled;
    regex_t r;
    DISABLE_COPYING(scoped_regex_t);
};

#endif /* CONTAINERS_SCOPED_REGEX_HPP_ */
