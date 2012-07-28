#ifndef CONTAINERS_SCOPED_REGEX_HPP_
#define CONTAINERS_SCOPED_REGEX_HPP_

#include <regex.h>
#include <string>

#include "errors.hpp"

class scoped_regex_t {
public:
    static const int default_flags = REG_EXTENDED | REG_NOSUB;
    scoped_regex_t(const std::string &pat, int flags = default_flags) :
        compiled(false), valid(true) {
        compile(pat, flags);
    }
    scoped_regex_t() : compiled(false), valid(true) { }
    ~scoped_regex_t() {
        if (is_compiled()) regfree(&r);
    }

    bool compile(const std::string &pat, int flags = default_flags) {
        rassert(!is_compiled());
        status = regcomp(&r, pat.c_str(), flags);
        if (!status) {
            compiled = true;
        } else {
            valid = false;
        }
        return is_valid();
    }

    bool matches(const std::string &str) const {
        if (!is_valid()) return false;
        rassert(is_compiled()); // Matching an uncompiled regex is an error.
        return !regexec(&r, str.c_str(), 0, 0, 0);
    }

    bool is_compiled() const { return compiled; }
    bool is_valid() const { return valid; }
    int get_status() const { return status; }
private:
    int status;
    bool compiled, valid;
    regex_t r;
    DISABLE_COPYING(scoped_regex_t);
};

#endif /* CONTAINERS_SCOPED_REGEX_HPP_ */
