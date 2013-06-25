// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PATHSPEC_HPP_
#define RDB_PROTOCOL_PATHSPEC_HPP_

#include "errors.hpp"
#include <boost/variant.hpp>

#include "rdb_protocol/datum.hpp"

namespace ql {

class pathspec_t {
public:
    enum type_t {
        ATOM,
        VEC,
        MAP
    } type;

    union {
        std::string atom;
        std::vector<pathspec_t> vec;
        std::map<std::string, pathspec_t> map;
    };

    const std::string *as_str() const {
        return (type == ATOM ? &atom : NULL);
    }

    const std::vector<pathspec_t> *as_vec() const {
        return (type == VEC ? &vec: NULL);
    }

    const std::map<std::string, pathspec_t> *as_map() const {
        return (type == MAP ? &map : NULL);
    }
};

enum recurse_flag_t {
    RECURSE,
    DONT_RECURSE
};

/* Limit the datum to only the paths specified by the pathspec. */
counted_t<datum_t> project(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);
/* Limit the datum to only the paths not specified by the pathspec. */
counted_t<datum_t> unproject(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);
/* Return whether or not ALL of the paths in the pathspec exist in the datum. */
bool contains(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse);

} //namespace ql

#endif
