// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/permissions.hpp"

#include "errors.hpp"
#include <boost/algorithm/string/join.hpp>

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

permissions_t::permissions_t() {
}

permissions_t::permissions_t(
        boost::tribool read,
        boost::tribool write,
        boost::tribool config)
    : m_read(read),
      m_write(write),
      m_config(config),
      m_connect(boost::none) {
}

permissions_t::permissions_t(
        boost::tribool read,
        boost::tribool write,
        boost::tribool config,
        boost::tribool connect)
    : m_read(read),
      m_write(write),
      m_config(config),
      m_connect(connect) {
}

permissions_t::permissions_t(ql::datum_t const &datum, bool global)
    : m_read(boost::indeterminate),
      m_write(boost::indeterminate),
      m_config(boost::indeterminate),
      m_connect(
        global
            ? boost::make_optional(boost::tribool(boost::indeterminate))
            : boost::none) {
    merge(datum);
}

boost::tribool datum_to_tribool(ql::datum_t const &datum, std::string const &field) {
    if (datum.get_type() == ql::datum_t::R_BOOL) {
        return datum.as_bool();
    } else if (datum.get_type() == ql::datum_t::R_NULL) {
        return boost::indeterminate;
    } else {
        throw admin_op_exc_t(
            "Expected a boolean or null for `" + field + "`, got " + datum.print(),
            query_state_t::FAILED);
    }
}

void permissions_t::merge(ql::datum_t const &datum) {
    if (datum.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object, got " + datum.print(), query_state_t::FAILED);
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
                "The `connect` permission is only valid at the global scope",
                query_state_t::FAILED);
        }
    }

    if (!keys.empty()) {
        throw admin_op_exc_t(
            "Unexpected key(s) `" + boost::algorithm::join(keys, "`, `") + "`",
            query_state_t::FAILED);
    }
}

boost::tribool permissions_t::get_read() const {
    return m_read;
}

boost::tribool permissions_t::get_write() const {
    return m_write;
}

boost::tribool permissions_t::get_config() const {
    return m_config;
}

boost::tribool permissions_t::get_connect() const {
    // `get_value_or` is deprecated, but we're using an ancient version of boost
    return m_connect.get_value_or(boost::indeterminate);
}

bool permissions_t::is_indeterminate() const {
    return
        boost::indeterminate(m_read) &&
        boost::indeterminate(m_write) &&
        boost::indeterminate(m_config) && (
            !static_cast<bool>(m_connect) || boost::indeterminate(*m_connect));
}

void permissions_t::set_read(boost::tribool read) {
    m_read = read;
}

void permissions_t::set_write(boost::tribool write) {
    m_write = write;
}

void permissions_t::set_config(boost::tribool config) {
    m_config = config;
}

void permissions_t::set_connect(boost::tribool connect) {
    guarantee(static_cast<bool>(m_connect));
    m_connect = connect;
}

ql::datum_t permissions_t::to_datum() const {
    ql::datum_object_builder_t datum_object_builder;

    if (!indeterminate(get_read())) {
        datum_object_builder.overwrite(
            "read", ql::datum_t::boolean(get_read()));
    }

    if (!indeterminate(get_write())) {
        datum_object_builder.overwrite(
            "write", ql::datum_t::boolean(get_write()));
    }

    if (!indeterminate(get_config())) {
        datum_object_builder.overwrite(
            "config", ql::datum_t::boolean(get_config()));
    }

    if (static_cast<bool>(m_connect) && !(indeterminate(get_connect()))) {
        datum_object_builder.overwrite(
            "connect", ql::datum_t::boolean(get_connect()));
    }

    if (datum_object_builder.empty()) {
        return ql::datum_t::null();
    } else {
        return std::move(datum_object_builder).to_datum();
    }
}

bool permissions_t::operator==(permissions_t const &rhs) const {
    // We can't use boost::optional<boost::tribool>::operator== due to implicit
    // conversion to bool operator not playing nice with boost::optional::operator==
    bool m_connect_equal =
        static_cast<bool>(m_connect) == static_cast<bool>(rhs.m_connect) && (
            !static_cast<bool>(m_connect) || *m_connect == *rhs.m_connect);

    return
        m_read == rhs.m_read &&
        m_write == rhs.m_write &&
        m_config == rhs.m_config &&
        m_connect_equal;
}

RDB_IMPL_SERIALIZABLE_4(permissions_t, m_read, m_write, m_config, m_connect);
INSTANTIATE_SERIALIZABLE_SINCE_v2_3(permissions_t);

}  // namespace auth
