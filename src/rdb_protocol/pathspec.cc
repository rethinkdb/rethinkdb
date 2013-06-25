#include "rdb_protocol/pathspec.hpp"

namespace ql {

pathspec_t::pathspec_t(const pathspec_t &other) 
    : type(other.type), str(other.str), 
      vec(other.vec), map(other.map)
{ }

pathspec_t::pathspec_t(const std::string &_str) 
    : type(STR), str(_str) { }

pathspec_t::pathspec_t(const std::map<std::string, pathspec_t> &_map)
    : type(MAP), map(_map) { }

pathspec_t::pathspec_t(counted_t<const datum_t> datum) {
    if (datum->get_type() == datum_t::R_STR) {
        type = STR;
        str = datum->as_str();
    } else if (datum->get_type() == datum_t::R_ARRAY) {
        type = VEC;
        for (size_t i = 0; i < datum->size(); ++i) {
            vec.push_back(pathspec_t(datum->get(i)));
        }
    } else if (datum->get_type() == datum_t::R_OBJECT) {
        for (auto it = datum->as_object().begin();
             it != datum->as_object().end(); ++it) {
            if (it->second->get_type() == datum_t::R_BOOL &&
                it->second->as_bool() == true) {
                vec.push_back(pathspec_t(it->first));
            } else {
                map.insert(std::make_pair(it->first, pathspec_t(it->second)));
            }
        }

        if (vec.empty()) {
            type = MAP;
        } else {
            type = VEC;
            vec.push_back(pathspec_t(map));
            map.clear();
        }
    } else {
        throw malformed_pathspec_exc_t();
    }
}

/* Limit the datum to only the paths specified by the pathspec. */
counted_t<const datum_t> project(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < datum->size(); ++i) {
            res->add(project(datum->get(i), pathspec, DONT_RECURSE));
        }
        return counted_t<const datum_t>(res.release());
    } else {
        scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_OBJECT));
        if (const std::string *str = pathspec.as_str()) {
            if (counted_t<const datum_t> val = datum->get(*str, NOTHROW)) {
                // This bool indicates if things were clobbered. We're fine
                // with things being clobbered so we ignore it.
                UNUSED bool clobbered = res->add(*str, val, CLOBBER);
            }
        } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
            for (auto it = vec->begin(); it != vec->end(); ++it) {
                counted_t<const datum_t> sub_result = project(datum, *it, recurse);
                for (auto jt = sub_result->as_object().begin();
                     jt != sub_result->as_object().end(); ++jt) {
                    UNUSED bool clobbered = res->add(jt->first, jt->second, CLOBBER);
                }
            }
        } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                    counted_t<const datum_t> sub_result = project(val, it->second, RECURSE);
                    /* We know we're clobbering, that's the point. */
                    UNUSED bool clobbered = res->add(it->first, sub_result, CLOBBER);
                }
            }
        } else {
            unreachable();
        }
        return counted_t<const datum_t>(res.release());
    }
}

void unproject_helper(datum_t *datum,
        const pathspec_t &pathspec, recurse_flag_t recurse) {
    if (const std::string *str = pathspec.as_str()) {
        UNUSED bool key_was_deleted = datum->delete_key(*str);
    } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
        for (auto it = vec->begin(); it != vec->end(); ++it) {
            /* This is all some annoying bullshit caused by the fact that
             * counted_t<datum_t> won't automatically convert to
             * counted_t<const datum_t>. */
            unproject_helper(datum, *it, recurse);
        }
    } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
        for (auto it = map->begin(); it != map->end(); ++it) {
            if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                counted_t<const datum_t> sub_result = unproject(val, it->second, RECURSE);
                /* We know we're clobbering, that's the point. */
                UNUSED bool clobbered = datum->add(it->first, sub_result, CLOBBER);
            }
        }
    } else {
        unreachable();
    }
}

/* Limit the datum to only the paths not specified by the pathspec. */
counted_t<const datum_t> unproject(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        scoped_ptr_t<datum_t> res(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < datum->size(); ++i) {
            res->add(unproject(datum->get(i), pathspec, DONT_RECURSE));
        }
        return counted_t<const datum_t>(res.release());
    } else {
        scoped_ptr_t<datum_t> res(new datum_t(datum->as_object()));
        unproject_helper(res.get(), pathspec, recurse);
        return counted_t<const datum_t>(res.release());
    }
}

/* Return whether or not ALL of the paths in the pathspec exist in the datum. */
bool contains(counted_t<const datum_t> datum,
        const pathspec_t &pathspec) {
    bool res = true;
    if (const std::string *str = pathspec.as_str()) {
        res &= datum->get(*str, NOTHROW).has();
    } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
        for (auto it = vec->begin(); it != vec->end(); ++it) {
            res &= contains(datum, *it);
        }
    } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
        for (auto it = map->begin(); it != map->end(); ++it) {
            if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                res &= contains(val, it->second);
            } else {
                res = false;
            }
        }
    } else {
        unreachable();
    }
    return res;
}
} //namespace ql 
