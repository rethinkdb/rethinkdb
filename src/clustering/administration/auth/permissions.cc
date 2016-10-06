// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/permissions.hpp"

#include "errors.hpp"
#include <boost/algorithm/string/join.hpp>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

permissions_t::permissions_t()
    : m_read(tribool::Indeterminate),
      m_write(tribool::Indeterminate),
      m_config(tribool::Indeterminate),
      m_connect(boost::none) { }

permissions_t::permissions_t(
        tribool read,
        tribool write,
        tribool config)
    : m_read(read),
      m_write(write),
      m_config(config),
      m_connect(boost::none) {
}

permissions_t::permissions_t(
        tribool read,
        tribool write,
        tribool config,
        tribool connect)
    : m_read(read),
      m_write(write),
      m_config(config),
      m_connect(connect) {
}

permissions_t::permissions_t(ql::datum_t const &datum, bool global)
    : m_read(tribool::Indeterminate),
      m_write(tribool::Indeterminate),
      m_config(tribool::Indeterminate),
      m_connect(
        global
            ? boost::make_optional(tribool::Indeterminate)
            : boost::none) {
    merge(datum);
}

tribool datum_to_tribool(ql::datum_t const &datum, std::string const &field) {
    if (datum.get_type() == ql::datum_t::R_BOOL) {
        return datum.as_bool() ? tribool::True : tribool::False;
    } else if (datum.get_type() == ql::datum_t::R_NULL) {
        return tribool::Indeterminate;
    } else {
        throw admin_op_exc_t(
            "Expected a boolean or null for `" + field + "`, got " + datum.print() + ".",
            query_state_t::FAILED);
    }
}

void permissions_t::merge(ql::datum_t const &datum) {
    if (datum.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object, got " + datum.print() + ".", query_state_t::FAILED);
    }

    std::set<std::string> keys;
    for (size_t i = 0; i < datum.obj_size(); ++i) {
        keys.insert(datum.get_pair(i).first.to_std());
    }

    ql::datum_t read = datum.get_field("read", ql::NOTHROW);
    if (read.has()) {
        keys.erase("read");
        set_read(datum_to_tribool(read, "read"));
    }

    ql::datum_t write = datum.get_field("write", ql::NOTHROW);
    if (write.has()) {
        keys.erase("write");
        set_write(datum_to_tribool(write, "write"));
    }

    ql::datum_t config = datum.get_field("config", ql::NOTHROW);
    if (config.has()) {
        keys.erase("config");
        set_config(datum_to_tribool(config, "config"));
    }

    ql::datum_t connect = datum.get_field("connect", ql::NOTHROW);
    if (connect.has()) {
        if (static_cast<bool>(m_connect)) {
            keys.erase("connect");
            set_connect(datum_to_tribool(connect, "connect"));
        } else {
            throw admin_op_exc_t(
                "The `connect` permission is only valid at the global scope.",
                query_state_t::FAILED);
        }
    }

    if (!keys.empty()) {
        throw admin_op_exc_t(
            "Unexpected key(s) `" + boost::algorithm::join(keys, "`, `") + "`.",
            query_state_t::FAILED);
    }
}

tribool permissions_t::get_read() const {
    return m_read;
}

tribool permissions_t::get_write() const {
    return m_write;
}

tribool permissions_t::get_config() const {
    return m_config;
}

tribool permissions_t::get_connect() const {
    // `get_value_or` is deprecated, but we're using an ancient version of boost
    return m_connect.get_value_or(tribool::Indeterminate);
}

bool permissions_t::is_indeterminate() const {
    return
        m_read == tribool::Indeterminate &&
        m_write == tribool::Indeterminate &&
        m_config == tribool::Indeterminate && (
            !static_cast<bool>(m_connect) || *m_connect == tribool::Indeterminate);
}

void permissions_t::set_read(tribool read) {
    m_read = read;
}

void permissions_t::set_write(tribool write) {
    m_write = write;
}

void permissions_t::set_config(tribool config) {
    m_config = config;
}

void permissions_t::set_connect(tribool connect) {
    guarantee(static_cast<bool>(m_connect));
    m_connect = connect;
}

ql::datum_t permissions_t::to_datum() const {
    ql::datum_object_builder_t datum_object_builder;

    if (get_read() != tribool::Indeterminate) {
        datum_object_builder.overwrite(
            "read", ql::datum_t::boolean(get_read() == tribool::True));
    }

    if (get_write() != tribool::Indeterminate) {
        datum_object_builder.overwrite(
            "write", ql::datum_t::boolean(get_write() == tribool::True));
    }

    if (get_config() != tribool::Indeterminate) {
        datum_object_builder.overwrite(
            "config", ql::datum_t::boolean(get_config() == tribool::True));
    }

    if (static_cast<bool>(m_connect) && m_connect.get() != tribool::Indeterminate) {
        datum_object_builder.overwrite(
            "connect", ql::datum_t::boolean(m_connect.get() == tribool::True));
    }

    if (datum_object_builder.empty()) {
        return ql::datum_t::null();
    } else {
        return std::move(datum_object_builder).to_datum();
    }
}

bool tribool_equals(tribool const &lhs, tribool const &rhs) {
    return static_cast<int8_t>(lhs) == static_cast<int8_t>(rhs);
}

bool permissions_t::operator==(permissions_t const &rhs) const {
    // We can't use boost::optional<boost::tribool>::operator== due to implicit
    // conversion to bool operator not playing nice with boost::optional::operator==

    // ^ False due to change to enum class tribool; not touching code.
    bool m_connect_equal =
        static_cast<bool>(m_connect) == static_cast<bool>(rhs.m_connect) && (
            !static_cast<bool>(m_connect) || tribool_equals(*m_connect, *rhs.m_connect));

    return
        tribool_equals(m_read, rhs.m_read) &&
        tribool_equals(m_write, rhs.m_write) &&
        tribool_equals(m_config, rhs.m_config) &&
        m_connect_equal;
}

RDB_IMPL_SERIALIZABLE_4(permissions_t, m_read, m_write, m_config, m_connect);
INSTANTIATE_SERIALIZABLE_SINCE_v2_3(permissions_t);

}  // namespace auth
