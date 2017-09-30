// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_RESPONSE_HPP_
#define RDB_PROTOCOL_RESPONSE_HPP_

#include <string>
#include <vector>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/ql2proto.hpp"

namespace ql {

class response_t {
public:
    response_t();
    void clear();

    // Setters
    void set_type(Response::ResponseType _type);
    void set_data(const ql::datum_t &_data);
    void set_data(std::vector<ql::datum_t> &&_data);
    void set_profile(const ql::datum_t &_profile);
    void add_note(Response::ResponseNote note);

    // Getters
    Response::ResponseType type() const;
    const optional<Response::ErrorType> &error_type() const;
    const std::vector<ql::datum_t> &data() const;
    const optional<ql::datum_t> &profile() const;
    const std::vector<Response::ResponseNote> &notes() const;
    const optional<ql::datum_t> &backtrace() const;

    // Clears the response and writes a valid error
    void fill_error(Response::ResponseType _type,
                    Response::ErrorType _error_type,
                    const std::string &message,
                    const ql::datum_t &_backtrace);

private:
    // Used to make sure type_ is set before we get any of the other fields
    bool type_is_initialized_;
    Response::ResponseType type_;
    std::vector<ql::datum_t> data_;
    std::vector<Response::ResponseNote> notes_;
    optional<Response::ErrorType> error_type_;
    optional<ql::datum_t> backtrace_;
    optional<ql::datum_t> profile_;

    DISABLE_COPYING(response_t);
};

} // namespace ql

#endif // RDB_PROTOCOL_RESPONSE_HPP_
