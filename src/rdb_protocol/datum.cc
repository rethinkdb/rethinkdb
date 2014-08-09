// Copyright 2010-2014 RethinkDB, all rights reserved.
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
#include "rdb_protocol/pseudo_binary.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/shards.hpp"
#include "stl_utils.hpp"

namespace ql {

const size_t tag_size = 8;

const std::set<std::string> datum_t::_allowed_pts = std::set<std::string>();

const char *const datum_t::reql_type_string = "$reql_type$";

datum_t::data_wrapper_t::data_wrapper_t(datum_t::construct_null_t) :
    type(R_NULL), r_str(NULL) { }

datum_t::data_wrapper_t::data_wrapper_t(datum_t::construct_boolean_t, bool _bool) :
    type(R_BOOL), r_bool(_bool) { }

datum_t::data_wrapper_t::data_wrapper_t(datum_t::construct_binary_t,
                                        scoped_ptr_t<wire_string_t> _data) :
    type(R_BINARY), r_str(_data.release()) { }

datum_t::data_wrapper_t::data_wrapper_t(double num) :
    type(R_NUM), r_num(num) { }

datum_t::data_wrapper_t::data_wrapper_t(std::string &&str) :
    type(R_STR),
    r_str(wire_string_t::create_and_init(str.size(), str.data()).release()) { }

datum_t::data_wrapper_t::data_wrapper_t(scoped_ptr_t<wire_string_t> str) :
    type(R_STR), r_str(str.release()) { }

datum_t::data_wrapper_t::data_wrapper_t(const char *cstr) :
    type(R_STR),
    r_str(wire_string_t::create_and_init(::strlen(cstr), cstr).release()) { }

datum_t::data_wrapper_t::data_wrapper_t(std::vector<counted_t<const datum_t> > &&array) :
    type(R_ARRAY),
    r_array(new std::vector<counted_t<const datum_t> >(std::move(array))) { }

datum_t::data_wrapper_t::data_wrapper_t(std::map<std::string,
                                                 counted_t<const datum_t> > &&object) :
    type(R_OBJECT),
    r_object(new std::map<std::string, counted_t<const datum_t> >(std::move(object))) { }

datum_t::data_wrapper_t::~data_wrapper_t() {
    switch (type) {
    case R_NULL: // fallthru
    case R_BOOL: // fallthru
    case R_NUM: break;
    case R_BINARY: // fallthru
    case R_STR: {
        delete r_str;
    } break;
    case R_ARRAY: {
        delete r_array;
    } break;
    case R_OBJECT: {
        delete r_object;
    } break;
    default: unreachable();
    }
}

datum_t::datum_t(datum_t::construct_null_t dummy) : data(dummy) { }

datum_t::datum_t(construct_boolean_t dummy, bool _bool) : data(dummy, _bool) { }

datum_t::datum_t(construct_binary_t dummy, scoped_ptr_t<wire_string_t> _data)
    : data(dummy, std::move(_data)) { }

datum_t::datum_t(double _num) : data(_num) {
    // isfinite is a macro on OS X in math.h, so we can't just say std::isfinite.
    using namespace std; // NOLINT(build/namespaces) due to platform variation
    rcheck(isfinite(data.r_num), base_exc_t::GENERIC,
           strprintf("Non-finite number: %" PR_RECONSTRUCTABLE_DOUBLE, data.r_num));
}

datum_t::datum_t(std::string &&_str) : data(std::move(_str)) {
    check_str_validity(data.r_str);
}

datum_t::datum_t(scoped_ptr_t<wire_string_t> str) : data(std::move(str)) {
    check_str_validity(data.r_str);
}

datum_t::datum_t(const char *cstr) : data(cstr) { }

datum_t::datum_t(std::vector<counted_t<const datum_t> > &&_array,
                 const configured_limits_t &limits)
    : data(std::move(_array)) {
    rcheck_array_size(*data.r_array, limits, base_exc_t::GENERIC);
}

datum_t::datum_t(std::vector<counted_t<const datum_t> > &&_array,
                 no_array_size_limit_check_t) : data(std::move(_array)) { }

datum_t::datum_t(std::map<std::string, counted_t<const datum_t> > &&_object,
                 const std::set<std::string> &allowed_pts)
    : data(std::move(_object)) {
    maybe_sanitize_ptype(allowed_pts);
}

datum_t::datum_t(std::map<std::string, counted_t<const datum_t> > &&_object,
                 no_sanitize_ptype_t)
    : data(std::move(_object)) { }

counted_t<const datum_t>
to_datum_for_client_serialization(grouped_data_t &&gd,
                                  reql_version_t reql_version,
                                  const configured_limits_t &limits) {
    std::map<std::string, counted_t<const datum_t> > map;
    map[datum_t::reql_type_string] = make_counted<const datum_t>("GROUPED_DATA");

    {
        datum_array_builder_t arr(limits);
        arr.reserve(gd.size());
        iterate_ordered_by_version(
                reql_version,
                gd,
                [&arr, &limits](const counted_t<const datum_t> &key,
                                counted_t<const datum_t> &value) {
                    arr.add(make_counted<const datum_t>(
                            std::vector<counted_t<const datum_t> >{
                                key, std::move(value) },
                            limits));
                });
        map["data"] = std::move(arr).to_counted();
    }

    // We don't sanitize the ptype because this is a fake ptype that should only
    // be used for serialization.
    // TODO(2014-08): This is a bad thing.
    return make_counted<datum_t>(std::move(map), datum_t::no_sanitize_ptype_t());
}

datum_t::~datum_t() {
}

counted_t<const datum_t> datum_t::empty_array() {
    return make_counted<datum_t>(std::vector<counted_t<const datum_t> >(),
                                 no_array_size_limit_check_t());
}

counted_t<const datum_t> datum_t::empty_object() {
    return make_counted<datum_t>(std::map<std::string, counted_t<const datum_t> >());
}

counted_t<const datum_t> datum_t::null() {
    return make_counted<datum_t>(construct_null_t());
}

counted_t<const datum_t> datum_t::boolean(bool value) {
    return make_counted<datum_t>(construct_boolean_t(), value);
}

counted_t<const datum_t> datum_t::binary(scoped_ptr_t<wire_string_t> _data) {
    return make_counted<datum_t>(construct_binary_t(), std::move(_data));
}

counted_t<const datum_t> to_datum(cJSON *json, const configured_limits_t &limits) {
    switch (json->type) {
    case cJSON_False: {
        return datum_t::boolean(false);
    } break;
    case cJSON_True: {
        return datum_t::boolean(true);
    } break;
    case cJSON_NULL: {
        return datum_t::null();
    } break;
    case cJSON_Number: {
        return make_counted<datum_t>(json->valuedouble);
    } break;
    case cJSON_String: {
        return make_counted<datum_t>(json->valuestring);
    } break;
    case cJSON_Array: {
        std::vector<counted_t<const datum_t> > array;
        json_array_iterator_t it(json);
        while (cJSON *item = it.next()) {
            array.push_back(to_datum(item, limits));
        }
        return make_counted<datum_t>(std::move(array), limits);
    } break;
    case cJSON_Object: {
        std::map<std::string, counted_t<const datum_t> > map;
        json_object_iterator_t it(json);
        while (cJSON *item = it.next()) {
            auto res = map.insert(std::make_pair(item->string, to_datum(item, limits)));
            rcheck_datum(res.second, base_exc_t::GENERIC,
                         strprintf("Duplicate key `%s` in JSON.", item->string));
        }
        const std::set<std::string> pts = { pseudo::literal_string };
        return make_counted<datum_t>(std::move(map), pts);
    } break;
    default: unreachable();
    }
}


void check_str_validity(const char *bytes, size_t count) {
    const char *pos = static_cast<const char *>(memchr(bytes, 0, count));
    rcheck_datum(pos == NULL,
                 base_exc_t::GENERIC,
                 // We truncate because lots of other places can call `c_str` on the
                 // error message.
                 strprintf("String `%.20s` (truncated) contains NULL byte at offset %zu.",
                           bytes, pos - bytes));
}

void datum_t::check_str_validity(const wire_string_t *str) {
    ::ql::check_str_validity(str->data(), str->size());
}

void datum_t::check_str_validity(const std::string &str) {
    ::ql::check_str_validity(str.data(), str.size());
}

datum_t::type_t datum_t::get_type() const { return data.type; }

bool datum_t::is_ptype() const {
    return get_type() == R_BINARY ||
        (get_type() == R_OBJECT && std_contains(*data.r_object, reql_type_string));
}

bool datum_t::is_ptype(const std::string &reql_type) const {
    return (reql_type == "") ? is_ptype() : is_ptype() && get_reql_type() == reql_type;
}

std::string datum_t::get_reql_type() const {
    r_sanity_check(is_ptype());
    if (get_type() == R_BINARY) {
        return "BINARY";
    }

    auto maybe_reql_type = data.r_object->find(reql_type_string);
    r_sanity_check(maybe_reql_type != data.r_object->end());
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
    case datum_t::R_BINARY: return std::string("PTYPE<") + pseudo::binary_string + ">";
    case datum_t::R_BOOL:   return "BOOL";
    case datum_t::R_NUM:    return "NUMBER";
    case datum_t::R_STR:    return "STRING";
    case datum_t::R_ARRAY:  return "ARRAY";
    case datum_t::R_OBJECT: return "OBJECT";
    default: unreachable();
    }
}

std::string datum_t::get_type_name() const {
    if (is_ptype()) {
        return "PTYPE<" + get_reql_type() + ">";
    } else {
        return raw_type_name(get_type());
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
    } else if (get_reql_type() == pseudo::geometry_string) {
        rfail(base_exc_t::GENERIC,
              "Cannot use a geometry value as a key value in a primary or "
              " non-geospatial secondary index.");
    } else {
        rfail(base_exc_t::GENERIC,
              "Cannot use pseudotype %s as a primary or secondary key value .",
              get_type_name().c_str());
    }
}

void datum_t::num_to_str_key(std::string *str_out) const {
    r_sanity_check(get_type() == R_NUM);
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

void datum_t::binary_to_str_key(std::string *str_out) const {
    // We need to prepend "P" and append a character less than [a-zA-Z] so that
    // different pseudotypes sort correctly.
    const std::string binary_key_prefix("PBINARY:");
    const wire_string_t &key = as_binary();

    str_out->append(binary_key_prefix);
    size_t to_append = std::min(MAX_KEY_SIZE - str_out->size(), key.size());

    // Escape null bytes so we don't cause key ambiguity when used in an array
    // We do this by replacing \x00 with \x01\x01 and replacing \x01 with \x01\x02
    for (size_t i = 0; i < to_append; ++i) {
        if (key.data()[i] == '\x00') {
            str_out->append("\x01\x01");
        } else if (key.data()[i] == '\x01') {
            str_out->append("\x01\x02");
        } else {
            str_out->append(1, key.data()[i]);
        }
    }
}

void datum_t::str_to_str_key(std::string *str_out) const {
    r_sanity_check(get_type() == R_STR);
    str_out->append("S");
    size_t to_append = std::min(MAX_KEY_SIZE - str_out->size(), as_str().size());
    str_out->append(as_str().data(), to_append);
}

void datum_t::bool_to_str_key(std::string *str_out) const {
    r_sanity_check(get_type() == R_BOOL);
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
    r_sanity_check(get_type() == R_ARRAY);
    str_out->append("A");

    for (size_t i = 0; i < size() && str_out->size() < MAX_KEY_SIZE; ++i) {
        counted_t<const datum_t> item = get(i, NOTHROW);
        r_sanity_check(item.has());

        switch (item->get_type()) {
        case R_NUM: item->num_to_str_key(str_out); break;
        case R_STR: item->str_to_str_key(str_out); break;
        case R_BINARY: item->binary_to_str_key(str_out); break;
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
                strprintf("Array keys can only contain numbers, strings, bools, "
                          " pseudotypes, or arrays (got %s of type %s).",
                          item->print().c_str(), item->get_type_name().c_str()));
            break;
        default:
            unreachable();
        }
        str_out->append(std::string(1, '\0'));
    }
}

int datum_t::pseudo_cmp(reql_version_t reql_version, const datum_t &rhs) const {
    r_sanity_check(is_ptype());
    if (get_type() == R_BINARY) {
        return as_binary().compare(rhs.as_binary());
    } else if (get_reql_type() == pseudo::time_string) {
        return pseudo::time_cmp(reql_version, *this, rhs);
    }

    rfail(base_exc_t::GENERIC, "Incomparable type %s.", get_type_name().c_str());
}

bool datum_t::pseudo_compares_as_obj() const {
    r_sanity_check(is_ptype());
    if (get_reql_type() == pseudo::geometry_string) {
        // We compare geometry by its object representation.
        // That's not especially meaningful, but works for indexing etc.
        return true;
    } else {
        return false;
    }
}

void datum_t::maybe_sanitize_ptype(const std::set<std::string> &allowed_pts) {
    if (is_ptype()) {
        std::string s = get_reql_type();
        if (s == pseudo::time_string) {
            pseudo::sanitize_time(this);
            return;
        }
        if (s == pseudo::literal_string) {
            rcheck(std_contains(allowed_pts, pseudo::literal_string),
                   base_exc_t::GENERIC,
                   "Stray literal keyword found, literal can only be present inside "
                   "merge and cannot nest inside other literals.");
            pseudo::rcheck_literal_valid(this);
            return;
        }
        if (s == pseudo::geometry_string) {
            // Semantic geometry validation is handled separately whenever a
            // geometry object is created (or used, when necessary).
            // This just performs a basic syntactic check.
            pseudo::sanitize_geometry(this);
            return;
        }
        if (s == pseudo::binary_string) {
            // Clear the pseudotype data and convert it to binary data
            scoped_ptr_t<std::map<std::string, counted_t<const datum_t> > >
                obj_data(data.r_object);
            data.r_object = NULL;

            data.r_str = pseudo::decode_base64_ptype(*obj_data.get()).release();
            data.type = R_BINARY;
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
    // drop_literals will never create arrays larger than those in the
    // existing datum; so checking (and thus threading the limits
    // parameter) is unnecessary here.
    const ql::configured_limits_t & limits = ql::configured_limits_t::unlimited;
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
    counted_t<const datum_t> copied_result;

    if (get_type() == R_OBJECT) {
        const std::map<std::string, counted_t<const datum_t> > &obj = as_object();
        datum_object_builder_t builder;

        for (auto it = obj.begin(); it != obj.end(); ++it) {
            bool encountered_literal;
            counted_t<const datum_t> val =
                it->second->drop_literals(&encountered_literal);

            if (encountered_literal && !need_to_copy) {
                // We have encountered the first field with a literal.
                // This means we have to create a copy in `result_copy`.
                need_to_copy = true;
                // Copy everything up to now into the builder.
                for (auto copy_it = obj.begin(); copy_it != it; ++copy_it) {
                    bool conflict = builder.add(copy_it->first, copy_it->second);
                    r_sanity_check(!conflict);
                }
            }

            if (need_to_copy) {
                if (val.has()) {
                    bool conflict = builder.add(it->first, val);
                    r_sanity_check(!conflict);
                } else {
                    // If `it->second` was a literal without a value, ignore it
                }
            }
        }

        copied_result = std::move(builder).to_counted();

    } else if (get_type() == R_ARRAY) {
        const std::vector<counted_t<const datum_t> > &arr = as_array();
        datum_array_builder_t builder(limits);

        for (auto it = arr.begin(); it != arr.end(); ++it) {
            bool encountered_literal;
            counted_t<const datum_t> val
                = (*it)->drop_literals(&encountered_literal);

            if (encountered_literal && !need_to_copy) {
                // We have encountered the first element with a literal.
                // This means we have to create a copy in `result_copy`.
                need_to_copy = true;
                // Copy everything up to now into the builder.
                for (auto copy_it = arr.begin(); copy_it != it; ++copy_it) {
                    builder.add(*copy_it);
                }
            }

            if (need_to_copy) {
                if (val.has()) {
                    builder.add(val);
                } else {
                    // If `*it` was a literal without a value, ignore it
                }
            }
        }

        copied_result = std::move(builder).to_counted();
    }

    if (need_to_copy) {
        *encountered_literal_out = true;
        rassert(copied_result.has());
        return copied_result;
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
    case R_BINARY: binary_to_str_key(&s); break;
    case R_BOOL: bool_to_str_key(&s); break;
    case R_ARRAY: array_to_str_key(&s); break;
    case R_OBJECT:
        if (is_ptype()) {
            pt_to_str_key(&s);
            break;
        }
        // fallthru
    case R_NULL:
        type_error(strprintf(
            "Primary keys must be either a number, string, bool, pseudotype "
            "or array (got type %s):\n%s",
            get_type_name().c_str(), trunc_print().c_str()));
        break;
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

std::string datum_t::encode_tag_num(uint64_t tag_num) {
    static_assert(sizeof(tag_num) == tag_size,
            "tag_size constant is assumed to be the size of a uint64_t.");
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "This piece of code will break on big-endian systems.");
#endif
    return std::string(reinterpret_cast<const char *>(&tag_num), tag_size);
}

std::string datum_t::compose_secondary(const std::string &secondary_key,
        const store_key_t &primary_key, boost::optional<uint64_t> tag_num) {
    std::string primary_key_string = key_to_unescaped_str(primary_key);

    if (primary_key_string.length() > rdb_protocol::MAX_PRIMARY_KEY_SIZE) {
        throw exc_t(base_exc_t::GENERIC,
            strprintf(
                "Primary key too long (max %zu characters): %s",
                rdb_protocol::MAX_PRIMARY_KEY_SIZE - 1,
                key_to_debug_str(primary_key).c_str()),
            NULL);
    }

    std::string tag_string;
    if (tag_num) {
        tag_string = encode_tag_num(tag_num.get());
    }

    const std::string truncated_secondary_key =
        secondary_key.substr(0, trunc_size(primary_key_string.length()));

    return mangle_secondary(truncated_secondary_key, primary_key_string, tag_string);
}

std::string datum_t::print_secondary(reql_version_t reql_version,
                                     const store_key_t &primary_key,
                                     boost::optional<uint64_t> tag_num) const {
    std::string secondary_key_string;

    // Reserve max key size to reduce reallocations
    secondary_key_string.reserve(MAX_KEY_SIZE);

    if (get_type() == R_NUM) {
        num_to_str_key(&secondary_key_string);
    } else if (get_type() == R_STR) {
        str_to_str_key(&secondary_key_string);
    } else if (get_type() == R_BINARY) {
        binary_to_str_key(&secondary_key_string);
    } else if (get_type() == R_BOOL) {
        bool_to_str_key(&secondary_key_string);
    } else if (get_type() == R_ARRAY) {
        array_to_str_key(&secondary_key_string);
    } else if (get_type() == R_OBJECT && is_ptype()) {
        pt_to_str_key(&secondary_key_string);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, pseudotype, "
            "or array (got type %s):\n%s",
            get_type_name().c_str(), trunc_print().c_str()));
    }

    switch (reql_version) {
    case reql_version_t::v1_13:
        break;
    case reql_version_t::v1_14_is_latest:
        secondary_key_string.append(1, '\x00');
        break;
    default:
        unreachable();
    }

    return compose_secondary(secondary_key_string, primary_key, tag_num);
}

struct components_t {
    std::string secondary;
    std::string primary;
    boost::optional<uint64_t> tag_num;
};

void parse_secondary(const std::string &key,
                     components_t *components) {
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
    if (get_type() == R_NUM) {
        num_to_str_key(&s);
    } else if (get_type() == R_STR) {
        str_to_str_key(&s);
    } else if (get_type() == R_BINARY) {
        binary_to_str_key(&s);
    } else if (get_type() == R_BOOL) {
        bool_to_str_key(&s);
    } else if (get_type() == R_ARRAY) {
        array_to_str_key(&s);
    } else if (get_type() == R_OBJECT && is_ptype()) {
        pt_to_str_key(&s);
    } else {
        type_error(strprintf(
            "Secondary keys must be a number, string, bool, pseudotype, "
            "or array (got %s of type %s).",
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
        return data.r_bool;
    } else {
        return get_type() != R_NULL;
    }
}
double datum_t::as_num() const {
    check_type(R_NUM);
    return data.r_num;
}

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

const wire_string_t &datum_t::as_binary() const {
    check_type(R_BINARY);
    return *data.r_str;
}

const wire_string_t &datum_t::as_str() const {
    check_type(R_STR);
    return *data.r_str;
}

const std::vector<counted_t<const datum_t> > &datum_t::as_array() const {
    check_type(R_ARRAY);
    return *data.r_array;
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
    return *data.r_object;
}

cJSON *datum_t::as_json_raw() const {
    switch (get_type()) {
    case R_NULL: return cJSON_CreateNull();
    case R_BINARY: return pseudo::encode_base64_ptype(as_binary()).release();
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
                 it = data.r_object->begin(); it != data.r_object->end(); ++it) {
            obj.AddItemToObject(it->first.c_str(), it->second->as_json_raw());
        }
        return obj.release();
    } break;
    default: unreachable();
    }
    unreachable();
}

scoped_cJSON_t datum_t::as_json() const {
    return scoped_cJSON_t(as_json_raw());
}

// TODO: make BINARY, STR, and OBJECT convertible to sequence?
counted_t<datum_stream_t>
datum_t::as_datum_stream(const protob_t<const Backtrace> &backtrace) const {
    switch (get_type()) {
    case R_NULL:   // fallthru
    case R_BINARY: // fallthru
    case R_BOOL:   // fallthru
    case R_NUM:    // fallthru
    case R_STR:    // fallthru
    case R_OBJECT: // fallthru
        type_error(strprintf("Cannot convert %s to SEQUENCE",
                             get_type_name().c_str()));
    case R_ARRAY:
        return make_counted<array_datum_stream_t>(this->counted_from_this(),
                                                  backtrace);
    default: unreachable();
    }
    unreachable();
};

MUST_USE bool datum_t::add(const std::string &key, counted_t<const datum_t> val,
                           clobber_bool_t clobber_bool) {
    check_type(R_OBJECT);
    check_str_validity(key);
    r_sanity_check(val.has());
    bool key_in_obj = data.r_object->count(key) > 0;
    if (!key_in_obj || (clobber_bool == CLOBBER)) {
        (*data.r_object)[key] = val;
    }
    return key_in_obj;
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs) const {
    if (get_type() != R_OBJECT || rhs->get_type() != R_OBJECT) {
        return rhs;
    }

    datum_object_builder_t d(as_object());
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        counted_t<const datum_t> sub_lhs = d.try_get(it->first);
        bool is_literal = it->second->is_ptype(pseudo::literal_string);

        if (it->second->get_type() == R_OBJECT && sub_lhs && !is_literal) {
            d.overwrite(it->first, sub_lhs->merge(it->second));
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
            if (val.has()) {
                d.overwrite(it->first, val);
            } else {
                r_sanity_check(is_literal);
                UNUSED bool b = d.delete_field(it->first);
            }
        }
    }
    return std::move(d).to_counted();
}

counted_t<const datum_t> datum_t::merge(counted_t<const datum_t> rhs,
                                        merge_resoluter_t f,
                                        const configured_limits_t &limits) const {
    datum_object_builder_t d(as_object());
    const std::map<std::string, counted_t<const datum_t> > &rhs_obj = rhs->as_object();
    for (auto it = rhs_obj.begin(); it != rhs_obj.end(); ++it) {
        if (counted_t<const datum_t> left = get(it->first, NOTHROW)) {
            d.overwrite(it->first, f(it->first, left, it->second, limits));
        } else {
            bool b = d.add(it->first, it->second);
            r_sanity_check(!b);
        }
    }
    return std::move(d).to_counted();
}

template<class T>
int derived_cmp(T a, T b) {
    if (a == b) return 0;
    return a < b ? -1 : 1;
}

int datum_t::v1_13_cmp(const datum_t &rhs) const {
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
            int cmpval = arr[i]->v1_13_cmp(*rhs_arr[i]);
            if (cmpval != 0) return cmpval;
        }
        guarantee(i <= rhs.as_array().size());
        return i == rhs.as_array().size() ? 0 : -1;
    } unreachable();
    case R_OBJECT: {
        if (is_ptype() && !pseudo_compares_as_obj()) {
            if (get_reql_type() != rhs.get_reql_type()) {
                return derived_cmp(get_reql_type(), rhs.get_reql_type());
            }
            return pseudo_cmp(reql_version_t::v1_13, rhs);
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
                int val_cmpval = it->second->v1_13_cmp(*it2->second);
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
    case R_BINARY: // This should be handled by the ptype code above
    default: unreachable();
    }
}

int datum_t::cmp(reql_version_t reql_version, const datum_t &rhs) const {
    switch (reql_version) {
    case reql_version_t::v1_13:
        return v1_13_cmp(rhs);
    case reql_version_t::v1_14_is_latest:
        return modern_cmp(rhs);
    default:
        unreachable();
    }
}

int datum_t::modern_cmp(const datum_t &rhs) const {
    bool lhs_ptype = is_ptype() && !pseudo_compares_as_obj();
    bool rhs_ptype = rhs.is_ptype() && !rhs.pseudo_compares_as_obj();
    if (lhs_ptype && rhs_ptype) {
        if (get_reql_type() != rhs.get_reql_type()) {
            return derived_cmp(get_reql_type(), rhs.get_reql_type());
        }
        return pseudo_cmp(reql_version_t::v1_14_is_latest, rhs);
    } else if (lhs_ptype || rhs_ptype) {
        return derived_cmp(get_type_name(), rhs.get_type_name());
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
            int cmpval = arr[i]->modern_cmp(*rhs_arr[i]);
            if (cmpval != 0) return cmpval;
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
            int key_cmpval = it->first.compare(it2->first);
            if (key_cmpval != 0) {
                return key_cmpval;
            }
            int val_cmpval = it->second->modern_cmp(*it2->second);
            if (val_cmpval != 0) {
                return val_cmpval;
            }
            ++it;
            ++it2;
        }
        if (it != obj.end()) return 1;
        if (it2 != rhs_obj.end()) return -1;
        return 0;
    } unreachable();
    case R_BINARY: // This should be handled by the ptype code above
    default: unreachable();
    }
}

bool datum_t::operator==(const datum_t &rhs) const { return modern_cmp(rhs) == 0; }
bool datum_t::operator!=(const datum_t &rhs) const { return modern_cmp(rhs) != 0; }
bool datum_t::compare_lt(reql_version_t reql_version, const datum_t &rhs) const {
    return cmp(reql_version, rhs) < 0;
}
bool datum_t::compare_gt(reql_version_t reql_version, const datum_t &rhs) const {
    return cmp(reql_version, rhs) > 0;
}

void datum_t::runtime_fail(base_exc_t::type_t exc_type,
                           const char *test, const char *file, int line,
                           std::string msg) const {
    ql::runtime_fail(exc_type, test, file, line, msg);
}

counted_t<const datum_t> to_datum(const Datum *d, const configured_limits_t &limits) {
    switch (d->type()) {
    case Datum::R_NULL: {
        return datum_t::null();
    } break;
    case Datum::R_BOOL: {
        return datum_t::boolean(d->r_bool());
    } break;
    case Datum::R_NUM: {
        return make_counted<datum_t>(d->r_num());
    } break;
    case Datum::R_STR: {
        return make_counted<datum_t>(std::string(d->r_str()));
    } break;
    case Datum::R_JSON: {
        scoped_cJSON_t cjson(cJSON_Parse(d->r_str().c_str()));
        return to_datum(cjson.get(), limits);
    } break;
    case Datum::R_ARRAY: {
        datum_array_builder_t out(limits);
        out.reserve(d->r_array_size());
        for (int i = 0, e = d->r_array_size(); i < e; ++i) {
            out.add(to_datum(&d->r_array(i), limits));
        }
        return std::move(out).to_counted();
    } break;
    case Datum::R_OBJECT: {
        std::map<std::string, counted_t<const datum_t> > map;
        const int count = d->r_object_size();
        for (int i = 0; i < count; ++i) {
            const Datum_AssocPair *ap = &d->r_object(i);
            const std::string &key = ap->key();
            datum_t::check_str_validity(key);
            auto res = map.insert(std::make_pair(key, to_datum(&ap->val(), limits)));
            rcheck_datum(res.second, base_exc_t::GENERIC,
                         strprintf("Duplicate key %s in object.", key.c_str()));
        }
        const std::set<std::string> pts = { pseudo::literal_string };
        return make_counted<datum_t>(std::move(map), pts);
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
        case R_BINARY: {
            pseudo::write_binary_to_protobuf(d, *data.r_str);
        } break;
        case R_BOOL: {
            d->set_type(Datum::R_BOOL);
            d->set_r_bool(data.r_bool);
        } break;
        case R_NUM: {
            d->set_type(Datum::R_NUM);
            // so we can use `isfinite` in a GCC 4.4.3-compatible way
            using namespace std;  // NOLINT(build/namespaces)
            r_sanity_check(isfinite(data.r_num));
            d->set_r_num(data.r_num);
        } break;
        case R_STR: {
            d->set_type(Datum::R_STR);
            d->set_r_str(data.r_str->data(), data.r_str->size());
        } break;
        case R_ARRAY: {
            d->set_type(Datum::R_ARRAY);
            for (size_t i = 0; i < data.r_array->size(); ++i) {
                (*data.r_array)[i]->write_to_protobuf(d->add_r_array(), use_json);
            }
        } break;
        case R_OBJECT: {
            d->set_type(Datum::R_OBJECT);
            // We use rbegin and rend so that things print the way we expect.
            for (auto it = data.r_object->rbegin(); it != data.r_object->rend(); ++it) {
                Datum_AssocPair *ap = d->add_r_object();
                ap->set_key(it->first);
                it->second->write_to_protobuf(ap->mutable_val(), use_json);
            }
        } break;
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

// `key` is unused because this is passed to `datum_t::merge`, which takes a
// generic conflict resolution function, but this particular conflict resolution
// function doesn't care about they key (although we could add some
// error-checking using the key in the future).
counted_t<const datum_t> stats_merge(UNUSED const std::string &key,
                                     counted_t<const datum_t> l,
                                     counted_t<const datum_t> r,
                                     const configured_limits_t &limits) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        return make_counted<datum_t>(l->as_num() + r->as_num());
    } else if (l->get_type() == datum_t::R_ARRAY && r->get_type() == datum_t::R_ARRAY) {
        if (l->size() + r->size() > limits.array_size_limit()) {
            datum_array_builder_t arr(limits);
            size_t so_far = 0;
            for (size_t i = 0; i < l->size() && so_far < limits.array_size_limit(); ++i, ++so_far) {
                arr.add(l->get(i));
            }
            for (size_t i = 0; i < r->size() && so_far < limits.array_size_limit(); ++i, ++so_far) {
                arr.add(r->get(i));
            }
            return std::move(arr).to_counted();
        } else {
            datum_array_builder_t arr(limits);
            for (size_t i = 0; i < l->size(); ++i) {
                arr.add(l->get(i));
            }
            for (size_t i = 0; i < r->size(); ++i) {
                arr.add(r->get(i));
            }
            return std::move(arr).to_counted();
        }
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

bool datum_object_builder_t::add(const std::string &key, counted_t<const datum_t> val) {
    datum_t::check_str_validity(key);
    r_sanity_check(val.has());
    auto res = map.insert(std::make_pair(key, std::move(val)));
    // Return _false_ if the insertion actually happened.  Because we are being
    // backwards to the C++ convention.
    return !res.second;
}

void datum_object_builder_t::overwrite(std::string key,
                                       counted_t<const datum_t> val) {
    datum_t::check_str_validity(key);
    r_sanity_check(val.has());
    map[std::move(key)] = std::move(val);
}

void datum_object_builder_t::add_error(const char *msg) {
    // Insert or update the "errors" entry.
    {
        counted_t<const datum_t> *errors_entry = &map["errors"];
        double ecount = (errors_entry->has() ? (*errors_entry)->as_num() : 0) + 1;
        *errors_entry = make_counted<datum_t>(ecount);
    }

    // If first_error already exists, nothing gets inserted.
    map.insert(std::make_pair("first_error", make_counted<datum_t>(msg)));
}

MUST_USE bool datum_object_builder_t::delete_field(const std::string &key) {
    return 0 != map.erase(key);
}


counted_t<const datum_t> datum_object_builder_t::at(const std::string &key) const {
    return map.at(key);
}

counted_t<const datum_t> datum_object_builder_t::try_get(const std::string &key) const {
    auto it = map.find(key);
    return it == map.end() ? counted_t<const datum_t>() : it->second;
}

counted_t<const datum_t> datum_object_builder_t::to_counted() RVALUE_THIS {
    return make_counted<const datum_t>(std::move(map));
}

counted_t<const datum_t> datum_object_builder_t::to_counted(
        const std::set<std::string> &permissible_ptypes) RVALUE_THIS {
    return make_counted<const datum_t>(std::move(map), permissible_ptypes);
}

datum_array_builder_t::datum_array_builder_t(std::vector<counted_t<const datum_t> > &&v,
                                             const configured_limits_t &_limits)
    : vector(std::move(v)), limits(_limits) {
    rcheck_array_size_datum(vector, limits, base_exc_t::GENERIC);
}

void datum_array_builder_t::reserve(size_t n) { vector.reserve(n); }

void datum_array_builder_t::add(counted_t<const datum_t> val) {
    vector.push_back(std::move(val));
    rcheck_array_size_datum(vector, limits, base_exc_t::GENERIC);
}

void datum_array_builder_t::change(size_t index, counted_t<const datum_t> val) {
    rcheck_datum(index < vector.size(),
                 base_exc_t::NON_EXISTENCE,
                 strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                           index, vector.size()));
    vector[index] = std::move(val);
}

void datum_array_builder_t::insert(reql_version_t reql_version, size_t index,
                                   counted_t<const datum_t> val) {
    rcheck_datum(index <= vector.size(),
                 base_exc_t::NON_EXISTENCE,
                 strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                           index, vector.size()));
    vector.insert(vector.begin() + index, std::move(val));

    switch (reql_version) {
    case reql_version_t::v1_13:
        break;
    case reql_version_t::v1_14_is_latest:
        rcheck_array_size_datum(vector, limits, base_exc_t::GENERIC);
        break;
    default:
        unreachable();
    }
}

void datum_array_builder_t::splice(reql_version_t reql_version, size_t index,
                                   counted_t<const datum_t> values) {
    rcheck_datum(index <= vector.size(),
                 base_exc_t::NON_EXISTENCE,
                 strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                           index, vector.size()));

    const std::vector<counted_t<const datum_t> > &arr = values->as_array();
    vector.insert(vector.begin() + index, arr.begin(), arr.end());

    switch (reql_version) {
    case reql_version_t::v1_13:
        break;
    case reql_version_t::v1_14_is_latest:
        rcheck_array_size_datum(vector, limits, base_exc_t::GENERIC);
        break;
    default:
        unreachable();
    }
}

void datum_array_builder_t::erase_range(reql_version_t reql_version,
                                        size_t start, size_t end) {

    // See https://github.com/rethinkdb/rethinkdb/issues/2696 about the backwards
    // compatible implementation for v1_13.

    switch (reql_version) {
    case reql_version_t::v1_13:
        rcheck_datum(start < vector.size(),
                     base_exc_t::NON_EXISTENCE,
                     strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                               start, vector.size()));
        break;
    case reql_version_t::v1_14_is_latest:
        rcheck_datum(start <= vector.size(),
                     base_exc_t::NON_EXISTENCE,
                     strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                               start, vector.size()));
        break;
    default:
        unreachable();
    }


    rcheck_datum(end <= vector.size(),
                 base_exc_t::NON_EXISTENCE,
                 strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                           end, vector.size()));
    rcheck_datum(start <= end,
                 base_exc_t::GENERIC,
                 strprintf("Start index `%zu` is greater than end index `%zu`.",
                           start, end));
    vector.erase(vector.begin() + start, vector.begin() + end);
}

void datum_array_builder_t::erase(size_t index) {
    rcheck_datum(index < vector.size(),
                 base_exc_t::NON_EXISTENCE,
                 strprintf("Index `%zu` out of bounds for array of size: `%zu`.",
                           index, vector.size()));
    vector.erase(vector.begin() + index);
}

counted_t<const datum_t> datum_array_builder_t::to_counted() RVALUE_THIS {
    // We call the non-checking constructor.  See
    // https://github.com/rethinkdb/rethinkdb/issues/2697 for more information --
    // insert and splice don't check the array size limit, because of a bug (as
    // reported in the issue).  This maintains that broken ReQL behavior because of
    // the generic reasons you would do so: secondary index compatibility after an
    // upgrade.
    return make_counted<datum_t>(std::move(vector),
                                 datum_t::no_array_size_limit_check_t());
}






} // namespace ql
