// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/pathspec.hpp"

#include "rdb_protocol/term.hpp"

namespace ql {

pathspec_t::pathspec_t(const pathspec_t &other) {
    init_from(other);
}

pathspec_t& pathspec_t::operator=(const pathspec_t &other) {
    // Delay freeing the memory we allocated in case &other == this or other
    // is a field inside of *this.
    type_t type_old = type;
    union {
        std::string *str_old;
        std::vector<pathspec_t> *vec_old;
        std::map<std::string, pathspec_t> *map_old;
    };
    switch (type) {
    case STR:
        str_old = str;
        break;
    case VEC:
        vec_old = vec;
        break;
    case MAP:
        map_old = map;
        break;
    default:
        unreachable();
    }

    init_from(other);

    switch (type_old) {
    case STR:
        delete str_old;
        break;
    case VEC:
        delete vec_old;
        break;
    case MAP:
        delete map_old;
        break;
    default:
        unreachable();
    }

    return *this;
}

pathspec_t::pathspec_t(const std::string &_str, const term_t *_creator)
    : type(STR), str(new std::string(_str)), creator(_creator) { }

pathspec_t::pathspec_t(const std::map<std::string, pathspec_t> &_map,
                       const term_t *_creator)
    : type(MAP), map(new std::map<std::string, pathspec_t>(_map)), creator(_creator) { }

pathspec_t::pathspec_t(counted_t<const datum_t> datum, const term_t *_creator)
    : creator(_creator)
{
    if (datum->get_type() == datum_t::R_STR) {
        type = STR;
        str = new std::string(datum->as_str().to_std());
    } else if (datum->get_type() == datum_t::R_ARRAY) {
        type = VEC;
        vec = new std::vector<pathspec_t>;
        for (size_t i = 0; i < datum->size(); ++i) {
            vec->push_back(pathspec_t(datum->get(i), creator));
        }
    } else if (datum->get_type() == datum_t::R_OBJECT) {
        scoped_ptr_t<std::vector<pathspec_t> > local_vec(new std::vector<pathspec_t>);
        scoped_ptr_t<std::map<std::string, pathspec_t> > local_map(new std::map<std::string, pathspec_t>);
        for (auto it = datum->as_object().begin();
             it != datum->as_object().end(); ++it) {
            if (it->second->get_type() == datum_t::R_BOOL &&
                it->second->as_bool() == true) {
                local_vec->push_back(pathspec_t(it->first, creator));
            } else {
                local_map->insert(std::make_pair(it->first, pathspec_t(it->second, creator)));
            }
        }

        if (local_vec->empty()) {
            type = MAP;
            map = local_map.release();
        } else {
            type = VEC;
            vec = local_vec.release();
            if (!local_map->empty()) {
                vec->push_back(pathspec_t(*local_map, creator));
            }
        }
    } else {
        rfail_target(creator, base_exc_t::GENERIC, "Invalid path argument `%s`.",
                datum->print().c_str());
    }

    if (type == VEC && vec->size() == 1) {
        *this = (*vec)[0];
    }
}

pathspec_t::~pathspec_t() {
    switch (type) {
    case STR:
        delete str;
        break;
    case VEC:
        delete vec;
        break;
    case MAP:
        delete map;
        break;
    default:
        unreachable();
    }
}

void pathspec_t::init_from(const pathspec_t &other) {
    type = other.type;
    switch (type) {
    case STR:
        str = new std::string(*other.str);
        break;
    case VEC:
        vec = new std::vector<pathspec_t>(*other.vec);
        break;
    case MAP:
        map = new std::map<std::string, pathspec_t>(*other.map);
        break;
    default:
        unreachable();
    }
    creator = other.creator;
}


/* Limit the datum to only the paths specified by the pathspec. */
counted_t<const datum_t> project(counted_t<const datum_t> datum,
                                 const pathspec_t &pathspec, recurse_flag_t recurse,
                                 const configured_limits_t &limits) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        datum_array_builder_t res(limits);
        res.reserve(datum->size());
        for (const counted_t<const datum_t> &value : datum->as_array()) {
            res.add(project(value, pathspec, DONT_RECURSE, limits));
        }
        return std::move(res).to_counted();
    } else {
        datum_object_builder_t res;
        if (pathspec.as_str() != NULL) {
            std::string str = static_cast<std::string>(*pathspec.as_str());
            if (counted_t<const datum_t> val = datum->get(str, NOTHROW)) {
                res.overwrite(std::move(str), val);
            }
        } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
            for (auto it = vec->begin(); it != vec->end(); ++it) {
                counted_t<const datum_t> sub_result = project(datum, *it, recurse, limits);
                for (auto jt = sub_result->as_object().begin();
                     jt != sub_result->as_object().end(); ++jt) {
                    res.overwrite(jt->first, jt->second);
                }
            }
        } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                    try {
                        counted_t<const datum_t> sub_result =
                            project(val, it->second, RECURSE, limits);
                        res.overwrite(it->first, sub_result);
                    } catch (const datum_exc_t &e) {
                        // do nothing
                    }
                }
            }
        } else {
            unreachable();
        }
        return std::move(res).to_counted();
    }
}

void unproject_helper(datum_object_builder_t *datum,
                      const pathspec_t &pathspec,
                      recurse_flag_t recurse,
                      const configured_limits_t &limits) {
    if (const std::string *str = pathspec.as_str()) {
        UNUSED bool key_was_deleted = datum->delete_field(*str);
    } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
        for (auto it = vec->begin(); it != vec->end(); ++it) {
            unproject_helper(datum, *it, recurse, limits);
        }
    } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
        for (auto it = map->begin(); it != map->end(); ++it) {
            if (counted_t<const datum_t> val = datum->try_get(it->first)) {
                try {
                    counted_t<const datum_t> sub_result =
                        unproject(val, it->second, RECURSE, limits);
                    datum->overwrite(it->first, sub_result);
                } catch (const datum_exc_t &e) {
                    // do nothing
                }
            }
        }
    } else {
        unreachable();
    }
}

/* Limit the datum to only the paths not specified by the pathspec. */
counted_t<const datum_t> unproject(counted_t<const datum_t> datum,
                                   const pathspec_t &pathspec, recurse_flag_t recurse,
                                   const configured_limits_t &limits) {
    if (datum->get_type() == datum_t::R_ARRAY && recurse == RECURSE) {
        datum_array_builder_t res(limits);
        res.reserve(datum->size());
        for (const counted_t<const datum_t> &value : datum->as_array()) {
            res.add(unproject(value, pathspec, DONT_RECURSE, limits));
        }
        return std::move(res).to_counted();
    } else {
        datum_object_builder_t res(datum->as_object());
        unproject_helper(&res, pathspec, recurse, limits);
        return std::move(res).to_counted();
    }
}

/* Return whether or not ALL of the paths in the pathspec exist in the datum. */
bool contains(counted_t<const datum_t> datum,
        const pathspec_t &pathspec) {
    try {
        bool res = true;
        if (const std::string *str = pathspec.as_str()) {
            if (!(res &= (datum->get(*str, NOTHROW).has() &&
                          datum->get(*str)->get_type() != datum_t::R_NULL))) {
                return res;
            }
        } else if (const std::vector<pathspec_t> *vec = pathspec.as_vec()) {
            for (auto it = vec->begin(); it != vec->end(); ++it) {
                if (!(res &= contains(datum, *it))) {
                    return res;
                }
            }
        } else if (const std::map<std::string, pathspec_t> *map = pathspec.as_map()) {
            for (auto it = map->begin(); it != map->end(); ++it) {
                if (counted_t<const datum_t> val = datum->get(it->first, NOTHROW)) {
                    if (!(res &= contains(val, it->second))) {
                        return res;
                    }
                } else {
                    return false;
                }
            }
        } else {
            unreachable();
        }
        return res;
    } catch (const datum_exc_t &e) {
        return false;
    }
}
} // namespace ql
