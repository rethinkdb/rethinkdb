#include "containers/scoped_regex.hpp"

#include "containers/scoped.hpp"

scoped_regex_t::~scoped_regex_t() {
    if (is_compiled()) {
        regfree(&r);
    }
}

MUST_USE bool scoped_regex_t::compile(const std::string &pat, int flags) {
    rassert(!is_compiled());
    errcode = regcomp(&r, pat.c_str(), flags);
    return is_compiled();
}

std::string scoped_regex_t::get_error() {
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

