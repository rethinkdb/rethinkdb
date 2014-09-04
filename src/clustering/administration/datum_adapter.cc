// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/datum_adapter.hpp"

#include "rdb_protocol/pseudo_time.hpp"

ql::datum_t convert_name_to_datum(
        const name_string_t &value) {
    return ql::datum_t(value.c_str());
}

bool convert_name_from_datum(
        ql::datum_t datum,
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

ql::datum_t convert_uuid_to_datum(
        const uuid_u &value) {
    return ql::datum_t(datum_string_t(uuid_to_str(value)));
}

bool convert_uuid_from_datum(
        ql::datum_t datum,
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

ql::datum_t convert_port_to_datum(
        uint16_t value) {
    return ql::datum_t(static_cast<double>(value));
}

ql::datum_t convert_microtime_to_datum(
        microtime_t value) {
    return ql::pseudo::make_time(value / 1.0e6, "+00:00");
}

bool converter_from_datum_object_t::init(
        ql::datum_t _datum,
        std::string *error_out) {
    if (_datum->get_type() != ql::datum_t::R_OBJECT) {
        *error_out = "Expected an object; got " + _datum->print();
        return false;
    }
    datum = _datum;
    for (size_t i = 0; i < datum.obj_size(); ++i) {
        std::pair<datum_string_t, ql::datum_t> pair = datum.get_pair(i);
        extra_keys.insert(pair.first);
    }
    return true;
}

bool converter_from_datum_object_t::get(
        const char *key,
        ql::datum_t *value_out,
        std::string *error_out) {
    extra_keys.erase(datum_string_t(key));
    *value_out = datum->get_field(key, ql::NOTHROW);
    if (!value_out->has()) {
        *error_out = strprintf("Expected a field named `%s`.", key);
        return false;
    }
    return true;
}

void converter_from_datum_object_t::get_optional(
        const char *key,
        ql::datum_t *value_out) {
    extra_keys.erase(datum_string_t(key));
    *value_out = datum->get_field(key, ql::NOTHROW);
}

bool converter_from_datum_object_t::check_no_extra_keys(std::string *error_out) {
    if (!extra_keys.empty()) {
        *error_out = "Unexpected key(s):";
        for (const datum_string_t &key : extra_keys) {
            (*error_out) += " " + key.to_std();
        }
        return false;
    }
    return true;
}

