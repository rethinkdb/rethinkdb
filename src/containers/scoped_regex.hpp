#ifndef CONTAINERS_SCOPED_REGEX_HPP_
#define CONTAINERS_SCOPED_REGEX_HPP_

#include <regex.h>
#include <string>

#include "containers/scoped.hpp"
#include "errors.hpp"

/* It's safe to reuse the REG_NOMATCH error code constant because it's
   only returned by regexec, not regcomp. */
const int REG_UNCOMPILED = REG_NOMATCH;
class scoped_regex_t {
public:
    static const int default_flags = REG_EXTENDED | REG_NOSUB;
    scoped_regex_t() : errcode(REG_UNCOMPILED) { }
    ~scoped_regex_t() { if (is_compiled()) regfree(&r); }

    MUST_USE bool compile(const std::string &pat, int flags = default_flags) {
        rassert(!is_compiled());
        errcode = regcomp(&r, pat.c_str(), flags);
        return is_compiled();
    }

    bool matches(const std::string &str) const {
        rassert(is_compiled()); // Matching an uncompiled regex is an error.
        return !regexec(&r, str.c_str(), 0, 0, 0);
    }

    bool is_compiled() const { return !errcode; }
    std::string get_error() {
        rassert(errcode);
        if (errcode == REG_UNCOMPILED) {
            return "Regular expression never compiled.";
        } else {
            size_t length = regerror(errcode, &r, 0, 0);
            scoped_array_t<char> raw(length);
            rassert(raw.data());
            regerror(errcode, &r, raw.data(), length);
            std::string out = raw.data();
            return out;
        }
    }
private:
    int errcode;
    regex_t r;
    DISABLE_COPYING(scoped_regex_t);
};

#endif /* CONTAINERS_SCOPED_REGEX_HPP_ */
