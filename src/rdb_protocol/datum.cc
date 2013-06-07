// Copyright 2010-2012 RethinkDB, all rights reserved.

#include "rdb_protocol/datum.hpp"

#include <float.h>
#include <math.h>
#include <algorithm>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/proto_utils.hpp"

namespace ql {

datum_t::datum_t(type_t _type, bool _bool) : type(_type), r_bool(_bool) {
    r_sanity_check(_type == R_BOOL);
}
datum_t::datum_t(double _num) : type(R_NUM), r_num(_num) {
    using namespace std; // so we can use `isfinite` in a GCC 4.4.3-compatible way
    rcheck(isfinite(r_num), base_exc_t::GENERIC,
           strprintf("Non-finite number: " DBLPRI, r_num));
}
datum_t::datum_t(const std::string &_str)
    : type(R_STR), r_str(new std::string(_str)) {
    check_str_validity(_str);
}
datum_t::datum_t(const char *cstr)
    : type(R_STR), r_str(new std::string(cstr)) { }
datum_t::datum_t(const std::vector<counted_t<const datum_t> > &_array)
    : type(R_ARRAY), r_array(new std::vector<counted_t<const datum_t> >(_array)) { }
datum_t::datum_t(const std::map<std::string, counted_t<const datum_t> > &_object)
    : type(R_OBJECT),
      r_object(new std::map<std::string, counted_t<const datum_t> >(_object)) { }
datum_t::datum_t(datum_t::type_t _type) : type(_type) {
    r_sanity_check(type == R_ARRAY || type == R_OBJECT || type == R_NULL);
    switch (type) {
    case R_NULL: {
        r_str = NULL; // Zeroing here is probably good for debugging.
    } break;
    case R_BOOL: // fallthru
    case R_NUM: // fallthru
    case R_STR: unreachable();
    case R_ARRAY: {
        r_array = new std::vector<counted_t<const datum_t> >();
    } break;
    case R_OBJECT: {
        r_object = new std::map<std::string, counted_t<const datum_t> >();
    } break;
    default: unreachable();
    }
}

datum_t::~datum_t() {
    switch (type) {
    case R_NULL: //fallthru
    case R_BOOL: //fallthru
    case R_NUM: break;
    case R_STR: {
        r_sanity_check(r_str != NULL);
        delete r_str;
    } break;
    case R_ARRAY: {
        r_sanity_check(r_array != NULL);
        delete r_array;
    } break;
    case R_OBJECT: {
        r_sanity_check(r_object != NULL);
        delete r_object;
    } break;
    default: unreachable();
    }
}

void datum_t::init_str() {
    type = R_STR;
    r_str = new std::string();
}

void datum_t::init_array() {
    type = R_ARRAY;
    r_array = new std::vector<counted_t<const datum_t> >();
}

void datum_t::init_object() {
    type = R_OBJECT;
    r_object = new std::map<std::string, counted_t<const datum_t> >();
}

void datum_t::init_json(cJSON *json, env_t *env) {
    switch (json->type) {
    case cJSON_False: {
        type = R_BOOL;
        r_bool = false;
    } break;
    case cJSON_True: {
        type = R_BOOL;
        r_bool = true;
    } break;
    case cJSON_NULL: {
        type = R_NULL;
    } break;
    case cJSON_Number: {
        type = R_NUM;
        r_num = json->valuedouble;
        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std;  // NOLINT(build/namespaces)
        r_sanity_check(isfinite(r_num));
    } break;
    case cJSON_String: {
        init_str();
        *r_str = json->valuestring;
    } break;
    case cJSON_Array: {
        init_array();
        for (int i = 0; i < cJSON_GetArraySize(json); ++i) {
            add(make_counted<datum_t>(cJSON_GetArrayItem(json, i), env));
        }
    } break;
    case cJSON_Object: {
        init_object();
        for (int i = 0; i < cJSON_GetArraySize(json); ++i) {
            cJSON *el = cJSON_GetArrayItem(json, i);
            bool b = add(el->string, make_counted<datum_t>(el, env));
            r_sanity_check(!b);
        }
    } break;
    default: unreachable();
    }
}

void datum_t::check_str_validity(const std::string &str) {
    size_t null_offset = str.find('\0');
    rcheck(null_offset == std::string::npos,
           base_exc_t::GENERIC,
           // We truncate because lots of other places can call `c_str` on the
           // error message.
           strprintf("String `%.20s` (truncated) contains NULL byte at offset %zu.",
                     str.c_str(), null_offset));
}

datum_t::datum_t(cJSON *json, env_t *env) {
    init_json(json, env);
}
datum_t::datum_t(const boost::shared_ptr<scoped_cJSON_t> &json, env_t *env) {
    init_json(json->get(), env);
}

datum_t::type_t datum_t::get_type() const { return type; }
const char *datum_type_name(datum_t::type_t type) {
    switch (type) {
    case datum_t::R_NULL:   return "NULL";
    case datum_t::R_BOOL:   return "BOOL";
    case datum_t::R_NUM:    return "NUMBER";
    case datum_t::R_STR:    return "STRING";
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
    return as_json()->Print();
}

void datum_t::num_to_str_key(std::string *str_out) const {
    r_sanity_check(type == R_NUM);
    str_out->append("N");
    union {
        double d;
        uint64_t u;
    } packed;
    guarantee(sizeof(packed.d) == sizeof(packed.u));
    packed.d = as_num();
    // Mangle the value so that lexicographic ordering matches double ordering
    if (packed.u & (1ULL << 63)) {
        // If we have a negative double, flip all the bits.  Flipping the
        // highest bit causes the negative doubles to sort below the
        // positive doubles (which will also have their highest bit
        // flipped), and flipping all the other bits causes more negative
        // doubles to sort below less negative doubles.
        packed.u = ~packed.u;
    } else {
        // If we have a non-negative double, flip the highest bit so that it
        // sorts higher than all the negative doubles (which had their
        // highest bit flipped as well).
        packed.u ^= (1ULL << 63);
    }
    // The formatting here is sensitive.  Talk to mlucy before changing it.
    str_out->append(strprintf("%.*" PRIx64, static_cast<int>(sizeof(double)*2), packed.u));
    str_out->append(strprintf("#" DBLPRI, as_num()));
}

void datum_t::str_to_str_key(std::string *str_out) const {
    r_sanity_check(type == R_STR);
    str_out->append("S");
    str_out->append(as_str());
}

void datum_t::bool_to_str_key(std::string *str_out) const {
    r_sanity_check(type == R_BOOL);
    str_out->append("B");
    if (as_bool()) {
        str_out->append("t");
    } else {
        str_out->append("f");
    }
}

// The key for an array is stored as a string of all its elements, each separated by a
//  null character, with another null character at the end to signify the end of the
//  array (this is necessary to prevent ambiguity when nested arrays are involved).
void datum_t::array_to_str_key(std::string *str_out) const {
    r_sanity_check(type == R_ARRAY);
    str_out->append("A");

    for (size_t i = 0; i < size(); ++i) {
        counted_t<const datum_t> item = get(i, NOTHROW);
        r_sanity_check(item.has());

        if (item->type == R_NUM) {
            item->num_to_str_key(str_out);
        } else if (item->type == R_STR) {
            item->str_to_str_key(str_out);
        } else if (item->type == R_BOOL) {
            item->bool_to_str_key(str_out);
        } else if (item->type == R_ARRAY) {
            item->array_to_str_key(str_out);
        } else {
            item->type_error(
                strprintf("Secondary keys must be a number, string, bool, or array "
                          "(got %s of type %s).", item->print().c_str(),
                          datum_type_name(item->type)));
        }

        str_out->append(std::string(1, '\0'));
    }
}

std::string datum_t::print_primary() const {
    std::string s;
    if (type == R_NUM) {
        num_to_str_key(&s);
    } else if (type == R_STR) {
        str_to_str_key(&s);
    } else if (type == R_BOOL) {
        bool_to_str_key(&s);
    } else {
        type_error(strprintf(
            "Primary keys must be either a number, bool, or string (got %s of type %s).",
            print().c_str(), datum_type_name(type)));
    }
    if (s.size() > rdb_protocol_t::MAX_PRIMARY_KEY_SIZE) {
        rfail(base_exc_t::GENERIC,
              "Primary key too long (max %zu characters): %s",
              rdb_protocol_t::MAX_PRIMARY_KEY_SIZE - 1, print().c_str());
    }
    return s;
}

std::string datum_t::print_secondary(const store_key_t &primary_key) const {
    std::string s;
    std::string primary_key_string = key_to_unescaped_str(primary_key);

    if (primary_key_string.length() > rdb_protocol_t::MAX_PRIMARY_KEY_SIZE) {
        rfail(base_exc_t::GENERIC,
              "Primary key too long (max %zu characters): %s",
              rdb_protocol_t::MAX_PRIMARY_KEY_SIZE - 1,
              key_to_debug_str(primary_key).c_str());
    }

    if (type == R_NUM) {
        num_to_str_key(&s);
    } else if (type == R_STR) {
        str_to_str_key(&s);
    } else if (type == R_BOOL) {
        bool_to_str_key(&s);
    } else if (type == R_ARRAY) {
        array_to_str_key(&s);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, or array "
            "(got %s of type %s).",
            print().c_str(), datum_type_name(type)));
    }

    s = s.substr(0, MAX_KEY_SIZE - primary_key_string.length() - 1) +
        std::string(1, '\0') + primary_key_string;

    return s;
}

std::string datum_t::unprint_secondary(
        const std::string &secondary_and_primary) {
    size_t separator = secondary_and_primary.find_last_of('\0');

    return secondary_and_primary.substr(separator + 1, std::string::npos);
}

// This function returns a store_key_t suitable for searching by a secondary-index.
//  This is needed because secondary indexes may be truncated, but the amount truncated
//  depends on the length of the primary key.  Since we do not know how much was truncated,
//  we have to truncate the maximum amount, then return all matches and filter them out later.
store_key_t datum_t::truncated_secondary() const {
    std::string s;
    if (type == R_NUM) {
        num_to_str_key(&s);
    } else if (type == R_STR) {
        str_to_str_key(&s);
    } else if (type == R_BOOL) {
        bool_to_str_key(&s);
    } else if (type == R_ARRAY) {
        array_to_str_key(&s);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, or array "
            "(got %s of type %s).",
            print().c_str(), datum_type_name(type)));
    }

    const size_t max_trunc_size = MAX_KEY_SIZE - rdb_protocol_t::MAX_PRIMARY_KEY_SIZE - 1;

    // If the key does not need truncation, add a null byte at the end to filter out more
    //  potential results
    if (s.length() < max_trunc_size) {
        s += std::string(1, '\0');
    } else {
        s.erase(max_trunc_size);
    }

    return store_key_t(s);
}

void datum_t::check_type(type_t desired, const char *msg) const {
    rcheck_typed_target(
        this, get_type() == desired,
        (msg != NULL)
            ? std::string(msg)
            : strprintf("Expected type %s but found %s.",
                        datum_type_name(desired), datum_type_name(get_type())));
}
void datum_t::type_error(const std::string &msg) const {
    rfail_typed_target(this, "%s", msg.c_str());
}

bool datum_t::as_bool() const {
    if (get_type() == R_BOOL) {
        return r_bool;
    } else {
        return get_type() != R_NULL;
    }
}
double datum_t::as_num() const {
    check_type(R_NUM);
    return r_num;
}

static const double max_dbl_int = 0x1LL << DBL_MANT_DIG;
static const double min_dbl_int = max_dbl_int * -1;
int64_t datum_t::as_int() const {
    static_assert(DBL_MANT_DIG == 53, "ERROR: Doubles are wrong size.");
    double d = as_num();
    rcheck(d <= max_dbl_int, base_exc_t::GENERIC,
           strprintf("Number not an integer (>2^53): " DBLPRI, d));
    rcheck(d >= min_dbl_int, base_exc_t::GENERIC,
           strprintf("Number not an integer (<-2^53): " DBLPRI, d));
    int64_t i = d;
    rcheck(static_cast<double>(i) == d, base_exc_t::GENERIC,
           strprintf("Number not an integer: " DBLPRI, d));
    return i;
}

const std::string &datum_t::as_str() const {
    check_type(R_STR);
    return *r_str;
}

const std::vector<counted_t<const datum_t> > &datum_t::as_array() const {
    check_type(R_ARRAY);
    return *r_array;
}

void datum_t::change(size_t index, counted_t<const datum_t> val) {
    check_type(R_ARRAY);
    rcheck(index < r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     index, r_array->size()));
    (*r_array)[index] = val;
}

void datum_t::insert(size_t index, counted_t<const datum_t> val) {
    check_type(R_ARRAY);
    rcheck(index <= r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     index, r_array->size()));
    r_array->insert(r_array->begin() + index, val);
}

void datum_t::erase(size_t index) {
    check_type(R_ARRAY);
    rcheck(index < r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     index, r_array->size()));
    r_array->erase(r_array->begin() + index);
}

void datum_t::erase_range(size_t start, size_t end) {
    check_type(R_ARRAY);
    rcheck(start < r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     start, r_array->size()));
    rcheck(end < r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     end, r_array->size()));
    rcheck(start <= end,
           base_exc_t::GENERIC,
           strprintf("Start index `%zu` is greater than end index `%zu`.",
                      start, end));
    r_array->erase(r_array->begin() + start, r_array->begin() + end + 1);
}

void datum_t::splice(size_t index, counted_t<const datum_t> values) {
    check_type(R_ARRAY);
    rcheck(index <= r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     index, r_array->size()));
    r_array->insert(r_array->begin() + index,
                    values->as_array().begin(), values->as_array().end());
}

size_t datum_t::size() const {
    return as_array().size();
}

counted_t<const datum_t> datum_t::get(size_t index, throw_bool_t throw_bool) const {
    if (index < size()) {
        return as_array()[index];
    } else if (throw_bool == THROW) {
        rfail(base_exc_t::NON_EXISTENCE, "Index out of bounds: %zu", index);
    } else {
        return counted_t<const datum_t>();
    }
}

counted_t<const datum_t> datum_t::get(const std::string &key,
                                      throw_bool_t throw_bool) const {
    std::map<std::string, counted_t<const datum_t> >::const_iterator it
        = as_object().find(key);
    if (it != as_object().end()) return it->second;
    if (throw_bool == THROW) {
        rfail(base_exc_t::NON_EXISTENCE,
              "No attribute `%s` in object:\n%s", key.c_str(), print().c_str());
    }
    return counted_t<const datum_t>();
}

const std::map<std::string, counted_t<const datum_t> > &datum_t::as_object() const {
    check_type(R_OBJECT);
    return *r_object;
}

cJSON *datum_t::as_raw_json() const {
    switch (get_type()) {
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
    } break;
    case R_OBJECT: {
        scoped_cJSON_t obj(cJSON_CreateObject());
        for (std::map<std::string, counted_t<const datum_t> >::const_iterator
                 it = as_object().begin(); it != as_object().end(); ++it) {
            obj.AddItemToObject(it->first.c_str(), it->second->as_raw_json());
        }
        return obj.release();
    } break;
    default: unreachable();
    }
    unreachable();
}
boost::shared_ptr<scoped_cJSON_t> datum_t::as_json() const {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(as_raw_json()));
}

// TODO: make STR and OBJECT convertible to sequence?
counted_t<datum_stream_t>
datum_t::as_datum_stream(env_t *env,
                         const protob_t<const Backtrace> &backtrace) const {
    switch (get_type()) {
    case R_NULL: //fallthru
    case R_BOOL: //fallthru
    case R_NUM:  //fallthru
    case R_STR:  //fallthru
    case R_OBJECT:
        type_error(strprintf("Cannot convert %s to SEQUENCE",
                             datum_type_name(get_type())));
    case R_ARRAY:
        return make_counted<array_datum_stream_t>(env,
                                                  this->counted_from_this(),
                                                  backtrace);
    default: unreachable();
    }
    unreachable();
};

void datum_t::add(counted_t<const datum_t> val) {
    check_type(R_ARRAY);
    r_sanity_check(val.has());
    r_array->push_back(val);
}

MUST_USE bool datum_t::add(const std::string &key, counted_t<const datum_t> val,
                           clobber_bool_t clobber_bool) {
    check_type(R_OBJECT);
    check_str_validity(key);
    r_sanity_check(val.has());
    bool key_in_obj = r_object->count(key) > 0;
    if (!key_in_obj || (clobber_bool == CLOBBER)) (*r_object)[key] = val;
    return key_in_obj;
}

MUST_USE bool datum_t::delete_key(const std::string &key) {
    return r_object->erase(key);
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs) const {
    scoped_ptr_t<datum_t> d(new datum_t(as_object()));
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        UNUSED bool b = d->add(it->first, it->second, CLOBBER);
    }
    return counted_t<const datum_t>(d.release());
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs, merge_res_f f) const {
    scoped_ptr_t<datum_t> d(new datum_t(as_object()));
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        if (counted_t<const datum_t> left = get(it->first, NOTHROW)) {
            bool b = d->add(it->first,
                            f(it->first, left, it->second, this),
                            CLOBBER);
            r_sanity_check(b);
        } else {
            bool b = d->add(it->first, it->second);
            r_sanity_check(!b);
        }
    }
    return counted_t<const datum_t>(d.release());
}

template<class T>
int derived_cmp(T a, T b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

int datum_t::cmp(const datum_t &rhs) const {
    if (get_type() != rhs.get_type()) {
        return derived_cmp(get_type(), rhs.get_type());
    }
    switch (get_type()) {
    case R_NULL: return 0;
    case R_BOOL: return derived_cmp(as_bool(), rhs.as_bool());
    case R_NUM: return derived_cmp(as_num(), rhs.as_num());
    case R_STR: return derived_cmp(as_str(), rhs.as_str());
    case R_ARRAY: {
        const std::vector<counted_t<const datum_t> >
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
    } unreachable();
    case R_OBJECT: {
        const std::map<std::string, counted_t<const datum_t> > &obj = as_object();
        const std::map<std::string, counted_t<const datum_t> > &rhs_obj
            = rhs.as_object();
        auto it = obj.begin();
        auto it2 = rhs_obj.begin();
        while (it != obj.end() && it2 != rhs_obj.end()) {
            int key_cmpval = derived_cmp(it->first, it2->first);
            if (key_cmpval) {
                return key_cmpval;
            }
            int val_cmpval = it->second->cmp(*it2->second);
            if (val_cmpval) {
                return val_cmpval;
            }
            ++it;
            ++it2;
        }
        if (it != obj.end()) return 1;
        if (it2 != rhs_obj.end()) return -1;
        return 0;
    } unreachable();
    default: unreachable();
    }
}

bool datum_t::operator== (const datum_t &rhs) const { return cmp(rhs) == 0;  }
bool datum_t::operator!= (const datum_t &rhs) const { return cmp(rhs) != 0;  }
bool datum_t::operator<  (const datum_t &rhs) const { return cmp(rhs) == -1; }
bool datum_t::operator<= (const datum_t &rhs) const { return cmp(rhs) != 1;  }
bool datum_t::operator>  (const datum_t &rhs) const { return cmp(rhs) == 1;  }
bool datum_t::operator>= (const datum_t &rhs) const { return cmp(rhs) != -1; }

datum_t::datum_t(const Datum *d, env_t *env) {
    switch (d->type()) {
    case Datum_DatumType_R_NULL: {
        type = R_NULL;
    } break;
    case Datum_DatumType_R_BOOL: {
        type = R_BOOL;
        r_bool = d->r_bool();
    } break;
    case Datum_DatumType_R_NUM: {
        type = R_NUM;
        r_num = d->r_num();
        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std;  // NOLINT(build/namespaces)
        rcheck(isfinite(r_num),
               base_exc_t::GENERIC,
               strprintf("Illegal non-finite number `" DBLPRI "`.", r_num));
    } break;
    case Datum_DatumType_R_STR: {
        init_str();
        *r_str = d->r_str();
        check_str_validity(*r_str);
    } break;
    case Datum_DatumType_R_ARRAY: {
        init_array();
        for (int i = 0; i < d->r_array_size(); ++i) {
            r_array->push_back(make_counted<datum_t>(&d->r_array(i), env));
        }
    } break;
    case Datum_DatumType_R_OBJECT: {
        init_object();
        for (int i = 0; i < d->r_object_size(); ++i) {
            const Datum_AssocPair *ap = &d->r_object(i);
            const std::string &key = ap->key();
            check_str_validity(key);
            rcheck(r_object->count(key) == 0,
                   base_exc_t::GENERIC,
                   strprintf("Duplicate key %s in object.", key.c_str()));
            (*r_object)[key] = make_counted<datum_t>(&ap->val(), env);
        }
    } break;
    default: unreachable();
    }
}

void datum_t::write_to_protobuf(Datum *d) const {
    switch (get_type()) {
    case R_NULL: {
        d->set_type(Datum_DatumType_R_NULL);
    } break;
    case R_BOOL: {
        d->set_type(Datum_DatumType_R_BOOL);
        d->set_r_bool(r_bool);
    } break;
    case R_NUM: {
        d->set_type(Datum_DatumType_R_NUM);
        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std;  // NOLINT(build/namespaces)
        r_sanity_check(isfinite(r_num));
        d->set_r_num(r_num);
    } break;
    case R_STR: {
        d->set_type(Datum_DatumType_R_STR);
        d->set_r_str(*r_str);
    } break;
    case R_ARRAY: {
        d->set_type(Datum_DatumType_R_ARRAY);
        for (size_t i = 0; i < r_array->size(); ++i) {
            (*r_array)[i]->write_to_protobuf(d->add_r_array());
        }
    } break;
    case R_OBJECT: {
        d->set_type(Datum_DatumType_R_OBJECT);
        // We use rbegin and rend so that things print the way we expect.
        for (std::map<std::string, counted_t<const datum_t> >::const_reverse_iterator
                 it = r_object->rbegin(); it != r_object->rend(); ++it) {
            Datum_AssocPair *ap = d->add_r_object();
            ap->set_key(it->first);
            it->second->write_to_protobuf(ap->mutable_val());
        }
    } break;
    default: unreachable();
    }
}

counted_t<const datum_t> wire_datum_t::get() const {
    r_sanity_check(state == COMPILED);
    return ptr;
}
counted_t<const datum_t> wire_datum_t::reset(counted_t<const datum_t> ptr2) {
    r_sanity_check(state == COMPILED);
    counted_t<const datum_t> tmp = ptr;
    ptr = ptr2;
    return tmp;
}

counted_t<const datum_t> wire_datum_t::compile(env_t *env) {
    if (state == COMPILED) return ptr;
    r_sanity_check(state != INVALID);
    ptr = make_counted<datum_t>(&ptr_pb, env);
    ptr_pb = Datum();
    return ptr;
}
void wire_datum_t::finalize() {
    if (state == SERIALIZABLE) {
        return;
    }
    r_sanity_check(state == COMPILED);
    ptr->write_to_protobuf(&ptr_pb);
    ptr.reset();
    state = SERIALIZABLE;
}

bool wire_datum_map_t::has(counted_t<const datum_t> key) {
    r_sanity_check(state == COMPILED);
    return map.count(key) > 0;
}

counted_t<const datum_t> wire_datum_map_t::get(counted_t<const datum_t> key) {
    r_sanity_check(state == COMPILED);
    r_sanity_check(has(key));
    return map[key];
}

void wire_datum_map_t::set(counted_t<const datum_t> key, counted_t<const datum_t> val) {
    r_sanity_check(state == COMPILED);
    map[key] = val;
}

void wire_datum_map_t::compile(env_t *env) {
    if (state == COMPILED) return;
    while (!map_pb.empty()) {
        map[make_counted<datum_t>(&map_pb.back().first, env)] =
            make_counted<datum_t>(&map_pb.back().second, env);
        map_pb.pop_back();
    }
    state = COMPILED;
}
void wire_datum_map_t::finalize() {
    if (state == SERIALIZABLE) return;
    r_sanity_check(state == COMPILED);
    while (!map.empty()) {
        map_pb.push_back(std::make_pair(Datum(), Datum()));
        map.begin()->first->write_to_protobuf(&map_pb.back().first);
        map.begin()->second->write_to_protobuf(&map_pb.back().second);
        map.erase(map.begin());
    }
    state = SERIALIZABLE;
}

counted_t<const datum_t> wire_datum_map_t::to_arr() const {
    r_sanity_check(state == COMPILED);
    scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
    for (auto it = map.begin(); it != map.end(); ++it) {
        scoped_ptr_t<datum_t> obj(new datum_t(datum_t::R_OBJECT));
        bool b1 = obj->add("group", it->first);
        bool b2 = obj->add("reduction", it->second);
        r_sanity_check(!b1 && !b2);
        arr->add(counted_t<datum_t>(obj.release()));
    }
    return counted_t<const datum_t>(arr.release());
}

void wire_datum_map_t::rdb_serialize(write_message_t &msg /* NOLINT */) const {
    r_sanity_check(state == SERIALIZABLE);
    msg << map_pb;
}

archive_result_t wire_datum_map_t::rdb_deserialize(read_stream_t *s) {
    archive_result_t res = deserialize(s, &map_pb);
    if (res) return res;
    state = SERIALIZABLE;
    return ARCHIVE_SUCCESS;
}


} //namespace ql
