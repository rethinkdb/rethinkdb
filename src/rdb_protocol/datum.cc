// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "math.h"

#include "rdb_protocol/ql2.hpp"

namespace ql {

datum_t::datum_t() : type(R_NULL) { }
datum_t::datum_t(bool _bool) : type(R_BOOL), r_bool(_bool) { }
datum_t::datum_t(double _num) : type(R_NUM), r_num(_num) {
    runtime_check(isfinite(r_num),
                  strprintf("Non-finite number: %lf", r_num));
}
datum_t::datum_t(const std::string &_str) : type(R_STR), r_str(_str) { }
datum_t::datum_t(const std::vector<const datum_t *> &_array)
    : type(R_ARRAY), r_array(_array) { }
datum_t::datum_t(const std::map<const std::string, const datum_t *> &_object)
    : type(R_OBJECT), r_object(_object) { }

datum_t::type_t datum_t::get_type() const { return type; }
void datum_t::check_type(type_t desired) const {
    runtime_check(get_type() == desired,
                  strprintf("Wrong type: expected %s but got %s.",
                            name(desired), name(get_type())));
}

bool datum_t::as_bool() const {
    check_type(R_BOOL);
    return r_bool;
}
double datum_t::as_num() const {
    check_type(R_NUM);
    return r_num;
}
const std::string &datum_t::as_str() const {
    check_type(R_STR);
    return r_str;
}
const std::vector<const datum_t *> &datum_t::as_array() const {
    check_type(R_ARRAY);
    return r_array;
}
const std::map<const std::string, const datum_t *> &datum_t::as_object() const {
    check_type(R_OBJECT);
    return r_object;
}

void datum_t::add(datum_t *val) {
    check_type(R_ARRAY);
    r_array.push_back(val);
}

MUST_USE bool datum_t::add(const std::string &key, datum_t *val, bool clobber) {
    check_type(R_OBJECT);
    bool key_in_obj = r_object.count(key) > 0;
    if (!key_in_obj || clobber) r_object[key] = val;
    return key_in_obj;
}

template<class T>
int derived_cmp(T a, T b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

int datum_t::cmp(const datum_t &rhs) const {
    if (get_type() != rhs.get_type()) return get_type() - rhs.get_type();
    switch(get_type()) {
    case R_NULL: return 0;
    case R_BOOL: return derived_cmp(as_bool(), rhs.as_bool());
    case R_NUM: return derived_cmp(as_num(), rhs.as_num());
    case R_STR: return derived_cmp(as_str(), rhs.as_str());
    case R_ARRAY: {
        const std::vector<const datum_t *>
            &arr = as_array(),
            &rhs_arr = rhs.as_array();
        size_t i;
        for (i = 0; i < arr.size(); ++i) {
            if (i >= rhs_arr.size()) return 1;
            int cmpval = arr[i]->cmp(*rhs_arr[i]);
            if (cmpval) return cmpval;
        }
        guarantee(i <= rhs.as_array().size());
        return i == rhs.as_array().size() ? 0 : -1;
    }; unreachable();
    case R_OBJECT: {
        const std::map<const std::string, const datum_t *>
            &obj = as_object(),
            &rhs_obj = rhs.as_object();
        std::map<const std::string, const datum_t *>::const_iterator
            it = obj.begin(),
            it2 = rhs_obj.begin();
        while (it != obj.end() && it2 != rhs_obj.end()) {
            int key_cmpval = derived_cmp(it->first, it2->first);
            if (key_cmpval) return key_cmpval;
            int val_cmpval = it->second->cmp(*it2->second);
            if (val_cmpval) return val_cmpval;
            ++it;
            ++it2;
        }
        if (it != obj.end()) return 1;
        if (it2 != obj.end()) return -1;
        return 0;
    }; unreachable();
    default: unreachable();
    }
}

bool datum_t::operator== (const datum_t &rhs) const { return cmp(rhs) == 0;  }
bool datum_t::operator!= (const datum_t &rhs) const { return cmp(rhs) != 0;  }
bool datum_t::operator<  (const datum_t &rhs) const { return cmp(rhs) == -1; }
bool datum_t::operator<= (const datum_t &rhs) const { return cmp(rhs) != 1;  }
bool datum_t::operator>  (const datum_t &rhs) const { return cmp(rhs) == 1;  }
bool datum_t::operator>= (const datum_t &rhs) const { return cmp(rhs) != -1; }

} //namespace ql
