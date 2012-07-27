#ifndef CONTAINERS_SCOPED_REGEX_HPP_
#define CONTAINERS_SCOPED_REGEX_HPP_

#include <regex.h>
#include <string>

#include "errors.hpp"

class scoped_regex_t {
public:
    static const int default_flags = REG_EXTENDED | REG_NOSUB;
    scoped_regex_t(const std::string &pat, int flags=default_flags) : is_set(false) {
        set(pat, flags);
    }
    scoped_regex_t() : is_set(false) { }
    ~scoped_regex_t() {
        if (is_set) regfree(&r);
    }

    int set(const std::string &pat, int flags=default_flags) {
        rassert(!is_set);
        int retval = regcomp(&r, pat.c_str(), flags);
        if (!retval) is_set = true;
        return retval;
    }
    bool matches(const std::string &str) const {
        rassert(is_set);
        return !regexec(&r, str.c_str(), 0, 0, 0);
    }
private:
    bool is_set;
    regex_t r;
    DISABLE_COPYING(scoped_regex_t);
};

#endif /* CONTAINERS_SCOPED_REGEX_HPP_ */
