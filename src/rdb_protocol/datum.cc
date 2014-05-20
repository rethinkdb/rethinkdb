// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/datum.hpp"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>

#include "errors.hpp"
#include <boost/detail/endian.hpp>

#include "containers/archive/stl_types.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/shards.hpp"
#include "stl_utils.hpp"

namespace ql {

const size_t tag_size = 8;

const std::set<std::string> datum_t::_allowed_pts = std::set<std::string>();

const char* const datum_t::reql_type_string = "$reql_type$";

datum_t::datum_t(type_t _type, bool _bool) : type(_type), r_bool(_bool) {
    r_sanity_check(_type == R_BOOL);
}

datum_t::datum_t(double _num) : type(R_NUM), r_num(_num) {
    // so we can use `isfinite` in a GCC 4.4.3-compatible way
    using namespace std;  // NOLINT(build/namespaces)
    rcheck(isfinite(r_num), base_exc_t::GENERIC,
           strprintf("Non-finite number: %" PR_RECONSTRUCTABLE_DOUBLE, r_num));
}

datum_t::datum_t(std::string &&_str)
    : type(R_STR), r_str(wire_string_t::create_and_init(_str.size(), _str.data()).release()) {
    check_str_validity(r_str);
}

datum_t::datum_t(scoped_ptr_t<wire_string_t> str)
    : type(R_STR), r_str(str.release()) {
    check_str_validity(r_str);
}

datum_t::datum_t(const char *cstr)
    : type(R_STR), r_str(wire_string_t::create_and_init(::strlen(cstr), cstr).release()) { }

datum_t::datum_t(std::vector<counted_t<const datum_t> > &&_array)
    : type(R_ARRAY),
      r_array(new std::vector<counted_t<const datum_t> >(std::move(_array))) {
    rcheck_array_size(*r_array, base_exc_t::GENERIC);
}

datum_t::datum_t(std::map<std::string, counted_t<const datum_t> > &&_object)
    : type(R_OBJECT),
      r_object(new std::map<std::string, counted_t<const datum_t> >(
                   std::move(_object))) {
    maybe_sanitize_ptype();
}

datum_t::datum_t(grouped_data_t &&gd)
    : type(R_OBJECT),
      r_object(new std::map<std::string, counted_t<const datum_t> >()) {
    (*r_object)[reql_type_string] = make_counted<const datum_t>("GROUPED_DATA");
    std::vector<counted_t<const datum_t> > v;
    v.reserve(gd.size());
    for (auto kv = gd.begin(); kv != gd.end(); ++kv) {
        v.push_back(make_counted<const datum_t>(
                        std::vector<counted_t<const datum_t> >{
                            std::move(kv->first), std::move(kv->second)}));
    }
    (*r_object)["data"] = make_counted<const datum_t>(std::move(v));
    // We don't sanitize the ptype because this is a fake ptype that should only
    // be used for serialization.
}

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
    case UNINITIALIZED: // fallthru
    default: unreachable();
    }
}

datum_t::~datum_t() {
    switch (type) {
    case R_NULL: // fallthru
    case R_BOOL: // fallthru
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
    case UNINITIALIZED: break;
    default: unreachable();
    }
}

void datum_t::init_str(size_t size, const char *data) {
    type = R_STR;
    r_str = wire_string_t::create_and_init(size, data).release();
}

void datum_t::init_array() {
    type = R_ARRAY;
    r_array = new std::vector<counted_t<const datum_t> >();
}

void datum_t::init_object() {
    type = R_OBJECT;
    r_object = new std::map<std::string, counted_t<const datum_t> >();
}

void datum_t::init_json(cJSON *json) {
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
        rcheck(isfinite(r_num), base_exc_t::GENERIC,
               strprintf("Non-finite value `%lf` in JSON.", r_num));
    } break;
    case cJSON_String: {
        init_str(strlen(json->valuestring), json->valuestring);
        check_str_validity(r_str);
    } break;
    case cJSON_Array: {
        init_array();
        json_array_iterator_t it(json);
        while (cJSON *item = it.next()) {
            add(make_counted<datum_t>(item));
        }
    } break;
    case cJSON_Object: {
        init_object();
        json_object_iterator_t it(json);
        while (cJSON *item = it.next()) {
            bool conflict = add(item->string, make_counted<datum_t>(item));
            rcheck(!conflict, base_exc_t::GENERIC,
                   strprintf("Duplicate key `%s` in JSON.", item->string));
        }
        maybe_sanitize_ptype();
    } break;
    default: unreachable();
    }
}

void datum_t::check_str_validity(const wire_string_t *str) {
    for (size_t i = 0; i < str->size(); ++i) {
        if (str->data()[i] == '\0') {
            rfail(base_exc_t::GENERIC,
                  // We truncate because lots of other places can call `c_str` on the
                  // error message.
                  "String `%.20s` (truncated) contains NULL byte at offset %zu.",
                  str->c_str(), i);
        }
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

datum_t::datum_t(cJSON *json) {
    init_json(json);
}
datum_t::datum_t(const scoped_cJSON_t &json) {
    init_json(json.get());
}

datum_t::type_t datum_t::get_type() const { return type; }

bool datum_t::is_ptype() const {
    return type == R_OBJECT && std_contains(*r_object, reql_type_string);
}

bool datum_t::is_ptype(const std::string &reql_type) const {
    return (reql_type == "") ? is_ptype() : is_ptype() && get_reql_type() == reql_type;
}

std::string datum_t::get_reql_type() const {
    r_sanity_check(get_type() == R_OBJECT);
    auto maybe_reql_type = r_object->find(reql_type_string);
    r_sanity_check(maybe_reql_type != r_object->end());
    rcheck(maybe_reql_type->second->get_type() == R_STR,
           base_exc_t::GENERIC,
           strprintf("Error: Field `%s` must be a string (got `%s` of type %s):\n%s",
                     reql_type_string,
                     maybe_reql_type->second->trunc_print().c_str(),
                     maybe_reql_type->second->get_type_name().c_str(),
                     trunc_print().c_str()));
    return maybe_reql_type->second->as_str().to_std();
}

std::string raw_type_name(datum_t::type_t type) {
    switch (type) {
    case datum_t::R_NULL:   return "NULL";
    case datum_t::R_BOOL:   return "BOOL";
    case datum_t::R_NUM:    return "NUMBER";
    case datum_t::R_STR:    return "STRING";
    case datum_t::R_ARRAY:  return "ARRAY";
    case datum_t::R_OBJECT: return "OBJECT";
    case datum_t::UNINITIALIZED: // fallthru
    default: unreachable();
    }
}

std::string datum_t::get_type_name() const {
    if (is_ptype()) {
        return "PTYPE<" + get_reql_type() + ">";
    } else {
        return raw_type_name(type);
    }
}

std::string datum_t::print() const {
    return as_json().Print();
}

std::string datum_t::trunc_print() const {
    std::string s = print();
    if (s.size() > trunc_len) {
        s.erase(s.begin() + (trunc_len - 3), s.end());
        s += "...";
    }
    return s;
}

void datum_t::pt_to_str_key(std::string *str_out) const {
    r_sanity_check(is_ptype());
    if (get_reql_type() == pseudo::time_string) {
        pseudo::time_to_str_key(*this, str_out);
    } else {
        rfail(base_exc_t::GENERIC,
              "Cannot use psuedotype %s as a primary or secondary key value .",
              get_type_name().c_str());
    }
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
    str_out->append(strprintf("#%" PR_RECONSTRUCTABLE_DOUBLE, as_num()));
}

void datum_t::str_to_str_key(std::string *str_out) const {
    r_sanity_check(type == R_STR);
    str_out->append("S");
    str_out->append(as_str().to_std());
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

        switch (item->get_type()) {
        case R_NUM: item->num_to_str_key(str_out); break;
        case R_STR: item->str_to_str_key(str_out); break;
        case R_BOOL: item->bool_to_str_key(str_out); break;
        case R_ARRAY: item->array_to_str_key(str_out); break;
        case R_OBJECT:
            if (item->is_ptype()) {
                item->pt_to_str_key(str_out);
                break;
            }
            // fallthru
        case R_NULL:
            item->type_error(
                strprintf("Secondary keys must be a number, string, bool, or array "
                          "(got %s of type %s).", item->print().c_str(),
                          item->get_type_name().c_str()));
            break;
        case UNINITIALIZED: // fallthru
        default:
            unreachable();
        }
        str_out->append(std::string(1, '\0'));
    }
}

int datum_t::pseudo_cmp(const datum_t &rhs) const {
    r_sanity_check(is_ptype());
    if (get_reql_type() == pseudo::time_string) {
        return pseudo::time_cmp(*this, rhs);
    }

    rfail(base_exc_t::GENERIC, "Incomparable type %s.", get_type_name().c_str());
}

void datum_t::maybe_sanitize_ptype(const std::set<std::string> &allowed_pts) {
    if (is_ptype()) {
        if (get_reql_type() == pseudo::time_string) {
            pseudo::sanitize_time(this);
            return;
        }
        if (get_reql_type() == pseudo::literal_string) {
            rcheck(std_contains(allowed_pts, pseudo::literal_string),
                   base_exc_t::GENERIC,
                   "Stray literal keyword found, literal can only be present inside "
                   "merge and cannot nest inside other literals.");
            pseudo::rcheck_literal_valid(this);
            return;
        }
        rfail(base_exc_t::GENERIC,
              "Unknown $reql_type$ `%s`.", get_type_name().c_str());
    }
}

void datum_t::rcheck_is_ptype(const std::string s) const {
    rcheck(is_ptype(), base_exc_t::GENERIC,
           (s == ""
            ? strprintf("Not a pseudotype: `%s`.", trunc_print().c_str())
            : strprintf("Not a %s pseudotype: `%s`.",
                        s.c_str(),
                        trunc_print().c_str())));
}

counted_t<const datum_t> datum_t::drop_literals(bool *encountered_literal_out) const {
    rassert(encountered_literal_out != NULL);

    const bool is_literal = is_ptype(pseudo::literal_string);
    if (is_literal) {
        counted_t<const datum_t> val = get(pseudo::value_key, NOTHROW);
        if (val) {
            bool encountered_literal;
            val = val->drop_literals(&encountered_literal);
            // Nested literals should have been caught on the higher QL levels.
            r_sanity_check(!encountered_literal);
        }
        *encountered_literal_out = true;
        return val;
    }

    // The result is either
    // - this->counted_from_this()
    // - or if `need_to_copy` is true, `copied_result`
    bool need_to_copy = false;
    scoped_ptr_t<datum_ptr_t> copied_result;

    if (get_type() == R_OBJECT) {
        const std::map<std::string, counted_t<const datum_t> > &obj = as_object();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            bool encountered_literal;
            counted_t<const datum_t> val =
                it->second->drop_literals(&encountered_literal);

            if (encountered_literal && !need_to_copy) {
                // We have encountered the first field with a literal.
                // This means we have to create a copy in `result_copy`.
                need_to_copy = true;
                // Copy everything up to now into the result
                copied_result.init(new datum_ptr_t(R_OBJECT));
                for (auto copy_it = obj.begin(); copy_it != it; ++copy_it) {
                    bool conflict = copied_result->add(copy_it->first, copy_it->second);
                    r_sanity_check(!conflict);
                }
            }

            if (need_to_copy) {
                if (val) {
                    bool conflict = copied_result->add(it->first, val);
                    r_sanity_check(!conflict);
                } else {
                    // If `it->second` was a literal without a value, ignore it
                }
            }
        }
    } else if (get_type() == R_ARRAY) {
        const std::vector<counted_t<const datum_t> > &arr = as_array();
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            bool encountered_literal;
            counted_t<const datum_t> val = (*it)->drop_literals(&encountered_literal);

            if (encountered_literal && !need_to_copy) {
                // We have encountered the first element with a literal.
                // This means we have to create a copy in `result_copy`.
                need_to_copy = true;
                // Copy everything up to now into the result
                copied_result.init(new datum_ptr_t(R_ARRAY));
                for (auto copy_it = arr.begin(); copy_it != it; ++copy_it) {
                    copied_result->add(*copy_it);
                }
            }

            if (need_to_copy) {
                if (val) {
                    copied_result->add(val);
                } else {
                    // If `*it` was a literal without a value, ignore it
                }
            }
        }
    }

    if (need_to_copy) {
        *encountered_literal_out = true;
        rassert(copied_result.has());
        return copied_result->to_counted();
    } else {
        *encountered_literal_out = false;
        return counted_from_this();
    }
}

void datum_t::rcheck_valid_replace(counted_t<const datum_t> old_val,
                                   counted_t<const datum_t> orig_key,
                                   const std::string &pkey) const {
    counted_t<const datum_t> pk = get(pkey, NOTHROW);
    rcheck(pk.has(), base_exc_t::GENERIC,
           strprintf("Inserted object must have primary key `%s`:\n%s",
                     pkey.c_str(), print().c_str()));
    if (old_val.has()) {
        counted_t<const datum_t> old_pk = orig_key;
        if (old_val->get_type() != R_NULL) {
            old_pk = old_val->get(pkey, NOTHROW);
            r_sanity_check(old_pk.has());
        }
        if (old_pk.has()) {
            rcheck(*old_pk == *pk, base_exc_t::GENERIC,
                   strprintf("Primary key `%s` cannot be changed (`%s` -> `%s`).",
                             pkey.c_str(), old_val->print().c_str(), print().c_str()));
        }
    } else {
        r_sanity_check(!orig_key.has());
    }
}

std::string datum_t::print_primary() const {
    std::string s;
    switch (get_type()) {
    case R_NUM: num_to_str_key(&s); break;
    case R_STR: str_to_str_key(&s); break;
    case R_BOOL: bool_to_str_key(&s); break;
    case R_ARRAY: array_to_str_key(&s); break;
    case R_OBJECT:
        if (is_ptype()) {
            pt_to_str_key(&s);
            break;
        }
        // fallthru
    case R_NULL: // fallthru
        type_error(strprintf(
            "Primary keys must be either a number, bool, pseudotype or string "
            "(got type %s):\n%s",
            get_type_name().c_str(), trunc_print().c_str()));
        break;
    case UNINITIALIZED: // fallthru
    default:
        unreachable();
    }

    if (s.size() > rdb_protocol::MAX_PRIMARY_KEY_SIZE) {
        rfail(base_exc_t::GENERIC,
              "Primary key too long (max %zu characters): %s",
              rdb_protocol::MAX_PRIMARY_KEY_SIZE - 1, print().c_str());
    }
    return s;
}

std::string datum_t::mangle_secondary(const std::string &secondary,
                                      const std::string &primary,
        const std::string &tag) {
    guarantee(secondary.size() < UINT8_MAX);
    guarantee(secondary.size() + primary.size() < UINT8_MAX);

    uint8_t pk_offset = static_cast<uint8_t>(secondary.size()),
            tag_offset = static_cast<uint8_t>(primary.size()) + pk_offset;

    std::string res = secondary + primary + tag +
           std::string(1, pk_offset) + std::string(1, tag_offset);
    guarantee(res.size() <= MAX_KEY_SIZE);
    return res;
}

std::string datum_t::print_secondary(const store_key_t &primary_key,
                                     boost::optional<uint64_t> tag_num) const {
    std::string secondary_key_string;
    std::string primary_key_string = key_to_unescaped_str(primary_key);

    if (primary_key_string.length() > rdb_protocol::MAX_PRIMARY_KEY_SIZE) {
        rfail(base_exc_t::GENERIC,
              "Primary key too long (max %zu characters): %s",
              rdb_protocol::MAX_PRIMARY_KEY_SIZE - 1,
              key_to_debug_str(primary_key).c_str());
    }

    if (type == R_NUM) {
        num_to_str_key(&secondary_key_string);
    } else if (type == R_STR) {
        str_to_str_key(&secondary_key_string);
    } else if (type == R_BOOL) {
        bool_to_str_key(&secondary_key_string);
    } else if (type == R_ARRAY) {
        array_to_str_key(&secondary_key_string);
    } else if (type == R_OBJECT && is_ptype()) {
        pt_to_str_key(&secondary_key_string);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, pseudotype, or array "
            "(got type %s):\n%s",
            get_type_name().c_str(), trunc_print().c_str()));
    }

    std::string tag_string;
    if (tag_num) {
        static_assert(sizeof(*tag_num) == tag_size,
                "tag_size constant is assumed to be the size of a uint64_t.");
#ifndef BOOST_LITTLE_ENDIAN
        static_assert(false, "This piece of code will break on big-endian systems.");
#endif
        tag_string.assign(reinterpret_cast<const char *>(&*tag_num), tag_size);
    }

    secondary_key_string =
        secondary_key_string.substr(0, trunc_size(primary_key_string.length()));

    return mangle_secondary(secondary_key_string, primary_key_string, tag_string);
}

struct components_t {
    std::string secondary;
    std::string primary;
    boost::optional<uint64_t> tag_num;
};

void parse_secondary(const std::string &key, components_t *components) {
    uint8_t start_of_tag = key[key.size() - 1],
            start_of_primary = key[key.size() - 2];

    guarantee(start_of_primary < start_of_tag);

    components->secondary = key.substr(0, start_of_primary);
    components->primary = key.substr(start_of_primary, start_of_tag - start_of_primary);

    std::string tag_str = key.substr(start_of_tag, key.size() - (start_of_tag + 2));
    if (tag_str.size() != 0) {
#ifndef BOOST_LITTLE_ENDIAN
        static_assert(false, "This piece of code will break on little endian systems.");
#endif
        components->tag_num = *reinterpret_cast<const uint64_t *>(tag_str.data());
    }
}

std::string datum_t::extract_primary(const std::string &secondary) {
    components_t components;
    parse_secondary(secondary, &components);
    return components.primary;
}

store_key_t datum_t::extract_primary(const store_key_t &secondary_key) {
    return store_key_t(extract_primary(key_to_unescaped_str(secondary_key)));
}

std::string datum_t::extract_secondary(const std::string &secondary) {
    components_t components;
    parse_secondary(secondary, &components);
    return components.secondary;
}

boost::optional<uint64_t> datum_t::extract_tag(const std::string &secondary) {
    components_t components;
    parse_secondary(secondary, &components);
    return components.tag_num;
}

boost::optional<uint64_t> datum_t::extract_tag(const store_key_t &key) {
    return extract_tag(key_to_unescaped_str(key));
}

// This function returns a store_key_t suitable for searching by a
// secondary-index.  This is needed because secondary indexes may be truncated,
// but the amount truncated depends on the length of the primary key.  Since we
// do not know how much was truncated, we have to truncate the maximum amount,
// then return all matches and filter them out later.
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
    } else if (type == R_OBJECT && is_ptype()) {
        pt_to_str_key(&s);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, or array "
            "(got %s of type %s).",
            print().c_str(), get_type_name().c_str()));
    }

    // Truncate the key if necessary
    if (s.length() >= max_trunc_size()) {
        s.erase(max_trunc_size());
    }

    return store_key_t(s);
}

void datum_t::check_type(type_t desired, const char *msg) const {
    rcheck_typed_target(
        this, get_type() == desired,
        (msg != NULL)
            ? std::string(msg)
            : strprintf("Expected type %s but found %s.",
                        raw_type_name(desired).c_str(), get_type_name().c_str()));
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

bool number_as_integer(double d, int64_t *i_out) {
    static_assert(DBL_MANT_DIG == 53, "Doubles are wrong size.");

    if (min_dbl_int <= d && d <= max_dbl_int) {
        int64_t i = d;
        if (static_cast<double>(i) == d) {
            *i_out = i;
            return true;
        }
    }
    return false;
}

int64_t checked_convert_to_int(const rcheckable_t *target, double d) {
    int64_t i;
    if (number_as_integer(d, &i)) {
        return i;
    } else {
        rfail_target(target, base_exc_t::GENERIC,
                     "Number not an integer%s: %" PR_RECONSTRUCTABLE_DOUBLE,
                     d < min_dbl_int ? " (<-2^53)" :
                         d > max_dbl_int ? " (>2^53)" : "",
                     d);
    }
}

struct datum_rcheckable_t : public rcheckable_t {
    explicit datum_rcheckable_t(const datum_t *_datum) : datum(_datum) { }
    void runtime_fail(base_exc_t::type_t type,
                      const char *test, const char *file, int line,
                      std::string msg) const {
        datum->runtime_fail(type, test, file, line, msg);
    }
    const datum_t *datum;
};

int64_t datum_t::as_int() const {
    datum_rcheckable_t target(this);
    return checked_convert_to_int(&target, as_num());
}

const wire_string_t &datum_t::as_str() const {
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
    rcheck(end <= r_array->size(),
           base_exc_t::NON_EXISTENCE,
           strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                     end, r_array->size()));
    rcheck(start <= end,
           base_exc_t::GENERIC,
           strprintf("Start index `%zu` is greater than end index `%zu`.",
                      start, end));
    r_array->erase(r_array->begin() + start, r_array->begin() + end);
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

cJSON *datum_t::as_json_raw() const {
    switch (get_type()) {
    case R_NULL: return cJSON_CreateNull();
    case R_BOOL: return cJSON_CreateBool(as_bool());
    case R_NUM: return cJSON_CreateNumber(as_num());
    case R_STR: return cJSON_CreateString(as_str().c_str());
    case R_ARRAY: {
        scoped_cJSON_t arr(cJSON_CreateArray());
        for (size_t i = 0; i < as_array().size(); ++i) {
            arr.AddItemToArray(as_array()[i]->as_json_raw());
        }
        return arr.release();
    } break;
    case R_OBJECT: {
        scoped_cJSON_t obj(cJSON_CreateObject());
        for (std::map<std::string, counted_t<const datum_t> >::const_iterator
                 it = r_object->begin(); it != r_object->end(); ++it) {
            obj.AddItemToObject(it->first.c_str(), it->second->as_json_raw());
        }
        return obj.release();
    } break;
    case UNINITIALIZED: // fallthru
    default: unreachable();
    }
    unreachable();
}

scoped_cJSON_t datum_t::as_json() const {
    return scoped_cJSON_t(as_json_raw());
}

// TODO: make STR and OBJECT convertible to sequence?
counted_t<datum_stream_t>
datum_t::as_datum_stream(const protob_t<const Backtrace> &backtrace) const {
    switch (get_type()) {
    case R_NULL: // fallthru
    case R_BOOL: // fallthru
    case R_NUM:  // fallthru
    case R_STR:  // fallthru
    case R_OBJECT: // fallthru
        type_error(strprintf("Cannot convert %s to SEQUENCE",
                             get_type_name().c_str()));
    case R_ARRAY:
        return make_counted<array_datum_stream_t>(this->counted_from_this(),
                                                  backtrace);
    case UNINITIALIZED: // fallthru
    default: unreachable();
    }
    unreachable();
};

void datum_t::add(counted_t<const datum_t> val) {
    check_type(R_ARRAY);
    r_sanity_check(val.has());
    r_array->push_back(val);
    rcheck_array_size(*r_array, base_exc_t::GENERIC);
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

MUST_USE bool datum_t::delete_field(const std::string &key) {
    return r_object->erase(key);
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs) const {
    if (get_type() != R_OBJECT || rhs->get_type() != R_OBJECT) { return rhs; }

    datum_ptr_t d(as_object());
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        counted_t<const datum_t> sub_lhs = d->get(it->first, NOTHROW);
        bool is_literal = it->second->is_ptype(pseudo::literal_string);

        if (it->second->get_type() == R_OBJECT && sub_lhs && !is_literal) {
            UNUSED bool b = d.add(it->first, sub_lhs->merge(it->second), CLOBBER);
        } else {
            counted_t<const datum_t> val =
                is_literal
                ? it->second->get(pseudo::value_key, NOTHROW)
                : it->second;
            if (val) {
                // Since nested literal keywords are forbidden, this should be a no-op
                // if `is_literal == true`.
                bool encountered_literal;
                val = val->drop_literals(&encountered_literal);
                r_sanity_check(!encountered_literal || !is_literal);
            }
            if (val) {
                UNUSED bool b = d.add(it->first, val, CLOBBER);
            } else {
                r_sanity_check(is_literal);
                UNUSED bool b = d.delete_field(it->first);
            }
        }
    }
    return d.to_counted();
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs,
                                        merge_resoluter_t f) const {
    datum_ptr_t d(as_object());
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        if (counted_t<const datum_t> left = get(it->first, NOTHROW)) {
            bool b = d.add(it->first, f(it->first, left, it->second), CLOBBER);
            r_sanity_check(b);
        } else {
            bool b = d.add(it->first, it->second);
            r_sanity_check(!b);
        }
    }
    return d.to_counted();
}

template<class T>
int derived_cmp(T a, T b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;
}


int datum_t::cmp(const datum_t &rhs) const {
    if (is_ptype() && !rhs.is_ptype()) {
        return 1;
    } else if (!is_ptype() && rhs.is_ptype()) {
        return -1;
    }

    if (get_type() != rhs.get_type()) {
        return derived_cmp(get_type(), rhs.get_type());
    }
    switch (get_type()) {
    case R_NULL: return 0;
    case R_BOOL: return derived_cmp(as_bool(), rhs.as_bool());
    case R_NUM: return derived_cmp(as_num(), rhs.as_num());
    case R_STR: return as_str().compare(rhs.as_str());
    case R_ARRAY: {
        const std::vector<counted_t<const datum_t> >
            &arr = as_array(),
            &rhs_arr = rhs.as_array();
        size_t i;
        for (i = 0; i < arr.size(); ++i) {
            if (i >= rhs_arr.size()) return 1;
            int cmpval = arr[i]->cmp(*rhs_arr[i]);
            if (cmpval != 0) return cmpval;
        }
        guarantee(i <= rhs.as_array().size());
        return i == rhs.as_array().size() ? 0 : -1;
    } unreachable();
    case R_OBJECT: {
        if (is_ptype()) {
            if (get_reql_type() != rhs.get_reql_type()) {
                return derived_cmp(get_reql_type(), rhs.get_reql_type());
            }
            return pseudo_cmp(rhs);
        } else {
            const std::map<std::string, counted_t<const datum_t> > &obj = as_object();
            const std::map<std::string, counted_t<const datum_t> > &rhs_obj
                = rhs.as_object();
            auto it = obj.begin();
            auto it2 = rhs_obj.begin();
            while (it != obj.end() && it2 != rhs_obj.end()) {
                int key_cmpval = it->first.compare(it2->first);
                if (key_cmpval != 0) {
                    return key_cmpval;
                }
                int val_cmpval = it->second->cmp(*it2->second);
                if (val_cmpval != 0) {
                    return val_cmpval;
                }
                ++it;
                ++it2;
            }
            if (it != obj.end()) return 1;
            if (it2 != rhs_obj.end()) return -1;
            return 0;
        }
    } unreachable();
    case UNINITIALIZED: // fallthru
    default: unreachable();
    }
}

bool datum_t::operator==(const datum_t &rhs) const { return cmp(rhs) == 0; }
bool datum_t::operator!=(const datum_t &rhs) const { return cmp(rhs) != 0; }
bool datum_t::operator<(const datum_t &rhs) const { return cmp(rhs) < 0; }
bool datum_t::operator<=(const datum_t &rhs) const { return cmp(rhs) <= 0; }
bool datum_t::operator>(const datum_t &rhs) const { return cmp(rhs) > 0; }
bool datum_t::operator>=(const datum_t &rhs) const { return cmp(rhs) >= 0; }

void datum_t::runtime_fail(base_exc_t::type_t exc_type,
                           const char *test, const char *file, int line,
                           std::string msg) const {
    ql::runtime_fail(exc_type, test, file, line, msg);
}

datum_t::datum_t() : type(UNINITIALIZED) { }

datum_t::datum_t(const Datum *d) : type(UNINITIALIZED) {
    init_from_pb(d);
}

void datum_t::init_from_pb(const Datum *d) {
    r_sanity_check(type == UNINITIALIZED);
    switch (d->type()) {
    case Datum::R_NULL: {
        type = R_NULL;
    } break;
    case Datum::R_BOOL: {
        type = R_BOOL;
        r_bool = d->r_bool();
    } break;
    case Datum::R_NUM: {
        type = R_NUM;
        r_num = d->r_num();
        // so we can use `isfinite` in a GCC 4.4.3-compatible way
        using namespace std;  // NOLINT(build/namespaces)
        rcheck(isfinite(r_num),
               base_exc_t::GENERIC,
               strprintf("Illegal non-finite number `%" PR_RECONSTRUCTABLE_DOUBLE "`.",
                         r_num));
    } break;
    case Datum::R_STR: {
        init_str(d->r_str().size(), d->r_str().data());
        check_str_validity(r_str);
    } break;
    case Datum::R_JSON: {
        scoped_cJSON_t cjson(cJSON_Parse(d->r_str().c_str()));
        init_json(cjson.get());
    } break;
    case Datum::R_ARRAY: {
        init_array();
        for (int i = 0; i < d->r_array_size(); ++i) {
            r_array->push_back(make_counted<datum_t>(&d->r_array(i)));
        }
    } break;
    case Datum::R_OBJECT: {
        init_object();
        for (int i = 0; i < d->r_object_size(); ++i) {
            const Datum_AssocPair *ap = &d->r_object(i);
            const std::string &key = ap->key();
            check_str_validity(key);
            rcheck(r_object->count(key) == 0,
                   base_exc_t::GENERIC,
                   strprintf("Duplicate key %s in object.", key.c_str()));
            (*r_object)[key] = make_counted<datum_t>(&ap->val());
        }
        std::set<std::string> allowed_ptypes = { pseudo::literal_string };
        maybe_sanitize_ptype(allowed_ptypes);
    } break;
    default: unreachable();
    }
}

size_t datum_t::max_trunc_size() {
    return trunc_size(rdb_protocol::MAX_PRIMARY_KEY_SIZE);
}

size_t datum_t::trunc_size(size_t primary_key_size) {
    //The 2 in this function is necessary because of the offsets which are
    //included at the end of the key so that we can extract the primary key and
    //the tag num from secondary keys.
    return MAX_KEY_SIZE - primary_key_size - tag_size - 2;
}

bool datum_t::key_is_truncated(const store_key_t &key) {
    std::string key_str = key_to_unescaped_str(key);
    if (extract_tag(key_str)) {
        return key.size() == MAX_KEY_SIZE;
    } else {
        return key.size() == MAX_KEY_SIZE - tag_size;
    }
}

void datum_t::write_to_protobuf(Datum *d, use_json_t use_json) const {
    switch (use_json) {
    case use_json_t::NO: {
        switch (get_type()) {
        case R_NULL: {
            d->set_type(Datum::R_NULL);
        } break;
        case R_BOOL: {
            d->set_type(Datum::R_BOOL);
            d->set_r_bool(r_bool);
        } break;
        case R_NUM: {
            d->set_type(Datum::R_NUM);
            // so we can use `isfinite` in a GCC 4.4.3-compatible way
            using namespace std;  // NOLINT(build/namespaces)
            r_sanity_check(isfinite(r_num));
            d->set_r_num(r_num);
        } break;
        case R_STR: {
            d->set_type(Datum::R_STR);
            d->set_r_str(r_str->data(), r_str->size());
        } break;
        case R_ARRAY: {
            d->set_type(Datum::R_ARRAY);
            for (size_t i = 0; i < r_array->size(); ++i) {
                (*r_array)[i]->write_to_protobuf(d->add_r_array(), use_json);
            }
        } break;
        case R_OBJECT: {
            d->set_type(Datum::R_OBJECT);
            // We use rbegin and rend so that things print the way we expect.
            for (auto it = r_object->rbegin(); it != r_object->rend(); ++it) {
                Datum_AssocPair *ap = d->add_r_object();
                ap->set_key(it->first);
                it->second->write_to_protobuf(ap->mutable_val(), use_json);
            }
        } break;
        case UNINITIALIZED: // fallthru
        default: unreachable();
        }
    } break;
    case use_json_t::YES: {
        d->set_type(Datum::R_JSON);
        d->set_r_str(as_json().PrintUnformatted());
    } break;
    default: unreachable();
    }
}

enum class datum_serialized_type_t {
    R_ARRAY = 1,
    R_BOOL = 2,
    R_NULL = 3,
    DOUBLE = 4,
    R_OBJECT = 5,
    R_STR = 6,
    INT_NEGATIVE = 7,
    INT_POSITIVE = 8,
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(datum_serialized_type_t, int8_t,
                                      datum_serialized_type_t::R_ARRAY,
                                      datum_serialized_type_t::INT_POSITIVE);

// This must be kept in sync with serialize(write_message_t *, const counted_t<const
// datum_t> &).
size_t serialized_size(const counted_t<const datum_t> &datum) {
    r_sanity_check(datum.has());
    size_t sz = 1; // 1 byte for the type
    switch (datum->get_type()) {
    case datum_t::R_ARRAY: {
        sz += serialized_size(datum->as_array());
    } break;
    case datum_t::R_BOOL: {
        sz += serialized_size_t<bool>::value;
    } break;
    case datum_t::R_NULL: break;
    case datum_t::R_NUM: {
        double d = datum->as_num();
        int64_t i;
        if (number_as_integer(d, &i)) {
            sz += varint_uint64_serialized_size(i < 0 ? -i : i);
        } else {
            sz += serialized_size_t<double>::value;
        }
    } break;
    case datum_t::R_OBJECT: {
        sz += serialized_size(datum->as_object());
    } break;
    case datum_t::R_STR: {
        sz += serialized_size(datum->as_str());
    } break;
    case datum_t::UNINITIALIZED:  // fall through
    default:
        unreachable();
    }
    return sz;
}

void serialize(write_message_t *wm, const counted_t<const datum_t> &datum) {
    r_sanity_check(datum.has());
    switch (datum->get_type()) {
    case datum_t::R_ARRAY: {
        serialize(wm, datum_serialized_type_t::R_ARRAY);
        const std::vector<counted_t<const datum_t> > &value = datum->as_array();
        serialize(wm, value);
    } break;
    case datum_t::R_BOOL: {
        serialize(wm, datum_serialized_type_t::R_BOOL);
        bool value = datum->as_bool();
        serialize(wm, value);
    } break;
    case datum_t::R_NULL: {
        serialize(wm, datum_serialized_type_t::R_NULL);
    } break;
    case datum_t::R_NUM: {
        double value = datum->as_num();
        int64_t i;
        if (number_as_integer(value, &i)) {
            // We serialize the signed-zero double, -0.0, with INT_NEGATIVE.

            // so we can use `signbit` in a GCC 4.4.3-compatible way
            using namespace std;  // NOLINT(build/namespaces)
            if (signbit(value)) {
                serialize(wm, datum_serialized_type_t::INT_NEGATIVE);
                serialize_varint_uint64(wm, -i);
            } else {
                serialize(wm, datum_serialized_type_t::INT_POSITIVE);
                serialize_varint_uint64(wm, i);
            }
        } else {
            serialize(wm, datum_serialized_type_t::DOUBLE);
            serialize(wm, value);
        }
    } break;
    case datum_t::R_OBJECT: {
        serialize(wm, datum_serialized_type_t::R_OBJECT);
        serialize(wm, datum->as_object());
    } break;
    case datum_t::R_STR: {
        serialize(wm, datum_serialized_type_t::R_STR);
        const wire_string_t &value = datum->as_str();
        serialize(wm, value);
    } break;
    case datum_t::UNINITIALIZED:  // fall through
    default:
        unreachable();
    }
}

archive_result_t deserialize(read_stream_t *s, counted_t<const datum_t> *datum) {
    datum_serialized_type_t type;
    archive_result_t res = deserialize(s, &type);
    if (bad(res)) {
        return res;
    }

    switch (type) {
    case datum_serialized_type_t::R_ARRAY: {
        std::vector<counted_t<const datum_t> > value;
        res = deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            datum->reset(new datum_t(std::move(value)));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_BOOL: {
        bool value;
        res = deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            datum->reset(new datum_t(datum_t::R_BOOL, value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_NULL: {
        datum->reset(new datum_t(datum_t::R_NULL));
    } break;
    case datum_serialized_type_t::DOUBLE: {
        double value;
        res = deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            datum->reset(new datum_t(value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::INT_NEGATIVE:  // fall through
    case datum_serialized_type_t::INT_POSITIVE: {
        uint64_t unsigned_value;
        res = deserialize_varint_uint64(s, &unsigned_value);
        if (bad(res)) {
            return res;
        }
        if (unsigned_value > max_dbl_int) {
            return archive_result_t::RANGE_ERROR;
        }
        const double d = unsigned_value;
        double value;
        if (type == datum_serialized_type_t::INT_NEGATIVE) {
            // This might deserialize the signed-zero double, -0.0.
            value = -d;
        } else {
            value = d;
        }
        try {
            datum->reset(new datum_t(value));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_OBJECT: {
        std::map<std::string, counted_t<const datum_t> > value;
        res = deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        try {
            datum->reset(new datum_t(std::move(value)));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    case datum_serialized_type_t::R_STR: {
        scoped_ptr_t<wire_string_t> value;
        res = deserialize(s, &value);
        if (bad(res)) {
            return res;
        }
        rassert(value.has());
        try {
            datum->reset(new datum_t(std::move(value)));
        } catch (const base_exc_t &) {
            return archive_result_t::RANGE_ERROR;
        }
    } break;
    default:
        return archive_result_t::RANGE_ERROR;
    }

    return archive_result_t::SUCCESS;
}

void serialize(write_message_t *wm,
               const empty_ok_t<const counted_t<const datum_t> > &datum) {
    const counted_t<const datum_t> *pointer = datum.get();
    const bool has = pointer->has();
    serialize(wm, has);
    if (has) {
        serialize(wm, *pointer);
    }
}

archive_result_t deserialize(read_stream_t *s,
                             empty_ok_ref_t<counted_t<const datum_t> > datum) {
    bool has;
    archive_result_t res = deserialize(s, &has);
    if (bad(res)) {
        return res;
    }

    counted_t<const datum_t> *pointer = datum.get();

    if (!has) {
        pointer->reset();
        return archive_result_t::SUCCESS;
    } else {
        return deserialize(s, pointer);
    }
}

// `key` is unused because this is passed to `datum_t::merge`, which takes a
// generic conflict resolution function, but this particular conflict resolution
// function doesn't care about they key (although we could add some
// error-checking using the key in the future).
counted_t<const datum_t> stats_merge(UNUSED const std::string &key,
                                     counted_t<const datum_t> l,
                                     counted_t<const datum_t> r) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        return make_counted<datum_t>(l->as_num() + r->as_num());
    } else if (l->get_type() == datum_t::R_ARRAY && r->get_type() == datum_t::R_ARRAY) {
        datum_ptr_t arr(datum_t::R_ARRAY);
        for (size_t i = 0; i < l->size(); ++i) {
            arr.add(l->get(i));
        }
        for (size_t i = 0; i < r->size(); ++i) {
            arr.add(r->get(i));
        }
        return arr.to_counted();
    }

    // Merging a string is left-preferential, which is just a no-op.
    rcheck_datum(
        l->get_type() == datum_t::R_STR && r->get_type() == datum_t::R_STR,
        base_exc_t::GENERIC,
        strprintf("Cannot merge statistics `%s` (type %s) and `%s` (type %s).",
                  l->trunc_print().c_str(), l->get_type_name().c_str(),
                  r->trunc_print().c_str(), r->get_type_name().c_str()));
    return l;
}

} // namespace ql
