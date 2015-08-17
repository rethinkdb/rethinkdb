// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PATHSPEC_HPP_
#define RDB_PROTOCOL_PATHSPEC_HPP_

#include <map>
#include <string>
#include <vector>

#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/datum.hpp"
#include "utils.hpp"

namespace ql {
class term_t;

class pathspec_t {
public:
    pathspec_t(const pathspec_t &other);
    pathspec_t &operator=(const pathspec_t &other);
    pathspec_t(const datum_string_t &str, const term_t *creator);
    pathspec_t(const std::map<datum_string_t, pathspec_t> &map, const term_t *creator);
    pathspec_t(datum_t datum, const term_t *creator);
    ~pathspec_t();
    const datum_string_t *as_str() const {
        return (type == STR ? str : NULL);
    }

    const std::vector<pathspec_t> *as_vec() const {
        return (type == VEC ? vec: NULL);
    }

    const std::map<datum_string_t, pathspec_t> *as_map() const {
        return (type == MAP ? map : NULL);
    }

    std::string print(int tabs = 0) const {
        std::string res;
        switch (type) {
        case STR:
            res += std::string(tabs * 2, ' ') + strprintf("STR: %s\n",
                                                          str->to_std().c_str());
            break;
        case VEC:
            res += std::string(tabs * 2, ' ') + strprintf("VEC:\n");
            for (auto it = vec->begin(); it != vec->end(); ++it) {
                res += it->print(tabs + 1);
            }
            break;
        case MAP:
            res += std::string(tabs * 2, ' ') + strprintf("MAP:\n");
            for (auto it = map->begin(); it != map->end(); ++it) {
                res += std::string(tabs * 2, ' ')
                    + strprintf("%s:\n", it->first.to_std().c_str());
                res += it->second.print(tabs + 2);
            }
            break;
        default:
            unreachable();
            break;
        }
        return res;
    }

private:
    void init_from(const pathspec_t &other);

    enum type_t {
        STR,
        VEC,
        MAP
    } type;

    union {
        datum_string_t *str;
        std::vector<pathspec_t> *vec;
        std::map<datum_string_t, pathspec_t> *map;
    };

    const term_t *creator;
};

enum recurse_flag_t {
    RECURSE,
    DONT_RECURSE
};

/* Limit the datum to only the paths specified by the pathspec. */
datum_t project(datum_t datum,
                const pathspec_t &pathspec, recurse_flag_t recurse,
                const configured_limits_t &limits);
/* Limit the datum to only the paths not specified by the pathspec. */
datum_t unproject(datum_t datum,
                  const pathspec_t &pathspec, recurse_flag_t recurse,
                  const configured_limits_t &limits);
/* Return whether or not ALL of the paths in the pathspec exist in the datum. */
bool contains(datum_t datum,
        const pathspec_t &pathspec);

} // namespace ql

#endif  // RDB_PROTOCOL_PATHSPEC_HPP_
