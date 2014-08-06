// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/datum_adapter.hpp"

counted_t<const ql::datum_t> convert_name_to_datum(
        const name_string_t &value) {
    return make_counted<const ql::datum_t>(std::string(value.str()));
}

bool convert_name_from_datum(
        counted_t<const ql::datum_t> datum,
        const std::string &what,
        name_string_t *value_out,
        std::string *error_out) {
    if (datum->get_type() != ql::datum_t::R_STR) {
        *error_out = "Expected a " + what + "; got " + datum->print();
        return false;
    }
    if (!value_out->assign_value(datum->as_str())) {
        *error_out = datum->print() + " is not a valid " + what + "; " +
            std::string(name_string_t::valid_char_msg);
        return false;
    }
    return true;
}

counted_t<const ql::datum_t> convert_uuid_to_datum(
        const uuid_u &value) {
    return make_counted<const ql::datum_t>(uuid_to_str(value));
}

bool convert_uuid_from_datum(
        counted_t<const ql::datum_t> datum,
        uuid_u *value_out,
        std::string *error_out) {
    if (datum->get_type() != ql::datum_t::R_STR) {
        *error_out = "Expected a UUID; got " + datum->print();
        return false;
    }
    if (!str_to_uuid(datum->as_str().to_std(), value_out)) {
        *error_out = "Expected a UUID; got " + datum->print();
        return false;
    }
    return true;
}

bool converter_from_datum_object_t::init(
        counted_t<const ql::datum_t> _datum,
        std::string *error_out) {
    if (_datum->get_type() != ql::datum_t::R_OBJECT) {
        *error_out = "Expected an object; got " + _datum->print();
        return false;
    }
    datum = _datum;
    const std::map<std::string, counted_t<const ql::datum_t> > &map = datum->as_object();
    for (auto it = map.begin(); it != map.end(); ++it) {
        extra_keys.insert(it->first);
    }
    return true;
}

bool converter_from_datum_object_t::get(
        const std::string &key,
        counted_t<const ql::datum_t> *value_out,
        std::string *error_out) {
    extra_keys.erase(key);
    *value_out = datum->get(key, ql::NOTHROW);
    if (!value_out->has()) {
        *error_out = "Expected a field named `" + key + "`.";
        return false;
    }
    return true;
}

void converter_from_datum_object_t::get_optional(
        const std::string &key,
        counted_t<const ql::datum_t> *value_out) {
    extra_keys.erase(key);
    *value_out = datum->get(key, ql::NOTHROW);
}

bool converter_from_datum_object_t::check_no_extra_keys(std::string *error_out) {
    if (!extra_keys.empty()) {
        *error_out = "Unexpected key(s):";
        for (const std::string &key : extra_keys) {
            (*error_out) += " " + key;
        }
        return false;
    }
    return true;
}

