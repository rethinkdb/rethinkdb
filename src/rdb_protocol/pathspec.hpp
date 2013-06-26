// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PATHSPEC_HPP_
#define RDB_PROTOCOL_PATHSPEC_HPP_

#include "errors.hpp"
#include <boost/variant.hpp>

#include "rdb_protocol/datum.hpp"

namespace ql {

class pathspec_t {
public:
    pathspec_t(const pathspec_t &other);
    explicit pathspec_t(const std::string &str);
    explicit pathspec_t(const std::map<std::string, pathspec_t> &map);
    explicit pathspec_t(counted_t<const datum_t> datum);
    const std::string *as_str() const {
        return (type == STR ? &str : NULL);
    }

    const std::vector<pathspec_t> *as_vec() const {
        return (type == VEC ? &vec: NULL);
    }

    const std::map<std::string, pathspec_t> *as_map() const {
        return (type == MAP ? &map : NULL);
    }

    class malformed_pathspec_exc_t : public std::exception {
        const char *what() const throw () {
            return "Couldn't compile pathspec.";
        }
    };

    void print() {
        switch (type) {
        case STR:
            break;
        case VEC:
            break;
        case MAP:
            break;
        }
    }

private:
    enum type_t {
        STR,
        VEC,
        MAP
    } type;

    std::string str;
    std::vector<pathspec_t> vec;
    std::map<std::string, pathspec_t> map;
};

enum recurse_flag_t {
    RECURSE,
    DONT_RECURSE
};

/* Limit the datum to only the paths specified by the pathspec. */
counted_t<const datum_t> project(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);
/* Limit the datum to only the paths not specified by the pathspec. */
counted_t<const datum_t> unproject(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);
/* Return whether or not ALL of the paths in the pathspec exist in the datum. */
bool contains(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);

} //namespace ql

#endif
