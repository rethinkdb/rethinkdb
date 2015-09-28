// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/response.hpp"

namespace ql {

response_t::response_t() {
    clear();
}

void response_t::fill_error(Response::ResponseType _type,
                            Response::ErrorType _error_type,
                            const std::string &message,
                            const ql::datum_t &_backtrace) {
    clear();
    type_ = _type;
    error_type_ = _error_type;
    backtrace_ = _backtrace;
    data_.push_back(ql::datum_t(datum_string_t(message)));

    rassert(type_ == Response::CLIENT_ERROR ||
            type_ == Response::COMPILE_ERROR ||
            type_ == Response::RUNTIME_ERROR);
}

void response_t::set_type(Response::ResponseType _type) {
    type_ = _type;
}

void response_t::set_data(const ql::datum_t &_data) {
    guarantee(data_.empty());
    data_.push_back(_data);
}

void response_t::set_data(std::vector<ql::datum_t> &&_data) {
    guarantee(data_.empty());
    data_ = std::move(_data);
}

void response_t::set_profile(const ql::datum_t &_profile) {
    guarantee(!profile_);
    profile_ = _profile;
}

void response_t::add_note(Response::ResponseNote note) {
    notes_.push_back(note);
}

void response_t::clear() {
    type_ = static_cast<Response::ResponseType>(-1);
    data_.clear();
    notes_.clear();
    error_type_ = boost::optional<Response::ErrorType>();
    backtrace_ = boost::optional<ql::datum_t>();
    profile_ = boost::optional<ql::datum_t>();
}

Response::ResponseType response_t::type() const {
    guarantee(type_ != -1);
    return type_;
}

const boost::optional<Response::ErrorType> &response_t::error_type() const {
    guarantee(type_ != -1);
    return error_type_;
}

const std::vector<ql::datum_t> &response_t::data() const {
    guarantee(type_ != -1);
    return data_;
}

const std::vector<Response::ResponseNote> &response_t::notes() const {
    guarantee(type_ != -1);
    return notes_;
}

const boost::optional<ql::datum_t> &response_t::backtrace() const {
    guarantee(type_ != -1);
    return backtrace_;
}

const boost::optional<ql::datum_t> &response_t::profile() const {
    guarantee(type_ != -1);
    return profile_;
}

} // namespace ql

