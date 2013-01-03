// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "math.h"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/datum.hpp"

namespace ql {

datum_t::datum_t() : type(R_NULL) { }
datum_t::datum_t(bool _bool) : type(R_BOOL), r_bool(_bool) { }
datum_t::datum_t(double _num) : type(R_NUM), r_num(_num) {
    rcheck(std::isfinite(r_num), strprintf("Non-finite number: %lf", r_num));
}
datum_t::datum_t(const std::string &_str) : type(R_STR), r_str(_str) { }
datum_t::datum_t(const std::vector<const datum_t *> &_array)
    : type(R_ARRAY), r_array(_array) { }
datum_t::datum_t(const std::map<const std::string, const datum_t *> &_object)
    : type(R_OBJECT), r_object(_object) { }
datum_t::datum_t(datum_t::type_t _type) : type(_type) {
    r_sanity_check(type == R_ARRAY || type == R_OBJECT || type == R_NULL);
}

void datum_t::init_json(cJSON *json, ptr_bag_t *alloc) {
    switch(json->type) {
    case cJSON_False: {
        type = R_BOOL; r_bool = false;
    }; break;
    case cJSON_True: {
        type = R_BOOL; r_bool = true;
    }; break;
    case cJSON_NULL: {
        type = R_NULL;
    }; break;
    case cJSON_Number: {
        type = R_NUM; r_num = json->valuedouble;
    }; break;
    case cJSON_String: {
        type = R_STR; r_str = json->valuestring;
    }; break;
    case cJSON_Array: {
        type = R_ARRAY;
        for (int i = 0; i < cJSON_GetArraySize(json); ++i) {
            add(alloc->add(new datum_t(cJSON_GetArrayItem(json, i), alloc)));
        }
    }; break;
    case cJSON_Object: {
        type = R_OBJECT;
        for (int i = 0; i < cJSON_GetArraySize(json); ++i) {
            cJSON *el = cJSON_GetArrayItem(json, i);
            bool b = add(el->string, alloc->add(new datum_t(el, alloc)));
            rcheck(!b, strprintf("Duplicate key: %s", el->string));
        }
    }; break;
    default: unreachable();
    }
}

datum_t::datum_t(cJSON *json, ptr_bag_t *alloc) { init_json(json, alloc); }
datum_t::datum_t(boost::shared_ptr<scoped_cJSON_t> json, ptr_bag_t *alloc) {
    init_json(json->get(), alloc);
}

datum_t::type_t datum_t::get_type() const { return type; }
const char *datum_type_name(datum_t::type_t type) {
    switch(type) {
    case datum_t::R_NULL:   return "NULL";
    case datum_t::R_BOOL:   return "BOOL";
    case datum_t::R_NUM:    return "NUM";
    case datum_t::R_STR:    return "STR";
    case datum_t::R_ARRAY:  return "ARRAY";
    case datum_t::R_OBJECT: return "OBJECT";
    default: unreachable();
    }
    unreachable();
}

const char *datum_t::get_type_name() const {
    return datum_type_name(get_type());
}
std::string datum_t::print() const {
    //TODO: Fix
    return "UNIMPLEMENTED";
}


void datum_t::check_type(type_t desired) const {
    rcheck(get_type() == desired,
           strprintf("Wrong type: expected %s but got %s.",
                     datum_type_name(desired), datum_type_name(get_type())));
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

cJSON *datum_t::as_raw_json() const {
    switch(get_type()) {
    case R_NULL: return cJSON_CreateNull();
    case R_BOOL: return cJSON_CreateBool(as_bool());
    case R_NUM: return cJSON_CreateNumber(as_num());
    case R_STR: return cJSON_CreateString(as_str().c_str());
    case R_ARRAY: {
        scoped_cJSON_t arr(cJSON_CreateArray());
        for (size_t i = 0; i < as_array().size(); ++i) {
            arr.AddItemToArray(as_array()[i]->as_raw_json());
        }
        return arr.release();
    }; break;
    case R_OBJECT: {
        scoped_cJSON_t obj(cJSON_CreateObject());
        for (std::map<const std::string, const datum_t *>::const_iterator
                 it = as_object().begin(); it != as_object().end(); ++it) {
            obj.AddItemToObject(it->first.c_str(), it->second->as_raw_json());
        }
        return obj.release();
    }; break;
    default: unreachable();
    }
    unreachable();
}
boost::shared_ptr<scoped_cJSON_t> datum_t::as_json() const {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(as_raw_json()));
}

void datum_t::add(const datum_t *val) {
    check_type(R_ARRAY);
    r_array.push_back(val);
}

MUST_USE bool datum_t::add(const std::string &key, const datum_t *val, bool clobber) {
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

datum_t::datum_t(const Datum *d, ptr_bag_t *alloc) {
    switch(d->type()) {
    case Datum_DatumType_R_NULL: {
        type = R_NULL;
    }; break;
    case Datum_DatumType_R_BOOL: {
        type = R_BOOL;
        r_bool = d->r_bool();
    }; break;
    case Datum_DatumType_R_NUM: {
        type = R_NUM;
        r_num = d->r_num();
    }; break;
    case Datum_DatumType_R_STR: {
        type = R_STR;
        r_str = d->r_str();
    }; break;
    case Datum_DatumType_R_ARRAY: {
        type = R_ARRAY;
        for (int i = 0; i < d->r_array_size(); ++i) {
            r_array.push_back(alloc->add(new datum_t(&d->r_array(i), alloc)));
        }
    }; break;
    case Datum_DatumType_R_OBJECT: {
        type = R_OBJECT;
        for (int i = 0; i < d->r_object_size(); ++i) {
            const Datum_AssocPair *ap = &d->r_object(i);
            const std::string &key = ap->key();
            rcheck(r_object.count(key) == 0,
                   strprintf("Duplicate key %s in object.", key.c_str()));
            r_object[key] = alloc->add(new datum_t(&ap->val(), alloc));
        }
    }; break;
    default: unreachable();
    }
}

void datum_t::write_to_protobuf(Datum *d) const {
    switch(get_type()) {
    case R_NULL: {
        d->set_type(Datum_DatumType_R_NULL);
    }; break;
    case R_BOOL: {
        d->set_type(Datum_DatumType_R_BOOL);
        d->set_r_bool(r_bool);
    }; break;
    case R_NUM: {
        d->set_type(Datum_DatumType_R_NUM);
        d->set_r_num(r_num);
    }; break;
    case R_STR: {
        d->set_type(Datum_DatumType_R_STR);
        d->set_r_str(r_str);
    }; break;
    case R_ARRAY: {
        d->set_type(Datum_DatumType_R_ARRAY);
        for (size_t i = 0; i < r_array.size(); ++i) {
            r_array[i]->write_to_protobuf(d->add_r_array());
        }
    }; break;
    case R_OBJECT: {
        d->set_type(Datum_DatumType_R_OBJECT);
        for (std::map<const std::string, const datum_t *>::const_iterator
                 it = r_object.begin(); it != r_object.end(); ++it) {
            Datum_AssocPair *ap = d->add_r_object();
            ap->set_key(it->first);
            it->second->write_to_protobuf(ap->mutable_val());
        }
    }; break;
    default: unreachable();
    }
}
} //namespace ql
