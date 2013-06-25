#include "rdb_protocol/pathspec.hpp"

namespace ql {

/* Limit the datum to only the paths specified by the pathspec. */
counted_t<datum_t> project(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        counted_t<datum_t> res = make_counted<datum_t>(datum_t::R_ARRAY);
        for (size_t i = 0; i < datum->size(); ++i) {
            res->add(project(datum->get(i), pathspec, DONT_RECURSE));
        }
        return res;
    } else {
        counted_t<datum_t> res = make_counted<datum_t>(datum_t::R_OBJECT);
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
        return res;
    }
}

/* Limit the datum to only the paths not specified by the pathspec. */
counted_t<datum_t> unproject(counted_t<const datum_t> datum,
        const pathspec_t &pathspec, recurse_flag_t recurse) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        counted_t<datum_t> res = make_counted<datum_t>(datum_t::R_ARRAY);
        for (size_t i = 0; i < datum->size(); ++i) {
            res->add(unproject(datum->get(i), pathspec, DONT_RECURSE));
        }
        return res;
    } else {
        counted_t<datum_t> res = make_counted<datum_t>(datum->as_object());
        if (const std::string *str = pathspec.as_str()) {
            UNUSED bool key_was_deleted = res->delete_key(*str);
        } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
            for (auto it = vec->begin(); it != vec->end(); ++it) {
                /* This is all some annoying bullshit caused by the fact that
                 * counted_t<datum_t> won't automatically convert to
                 * counted_t<const datum_t>. */
                res = unproject(make_counted<const datum_t>(res->as_object()), *it, recurse);
            }
        } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                    counted_t<const datum_t> sub_result = unproject(val, it->second, RECURSE);
                    /* We know we're clobbering, that's the point. */
                    UNUSED bool clobbered = res->add(it->first, sub_result, CLOBBER);
                }
            }
        } else {
            unreachable();
        }
        return res;
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
