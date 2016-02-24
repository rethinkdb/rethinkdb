// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/permissions.hpp"

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

void permissions_t::merge(ql::datum_t const &datum) {
    std::set<datum_string_t> keys;
    for (size_t i = 0; i < datum.obj_size(); ++i) {
        keys.insert(datum.get_pair(i).first);
    }

    if (datum.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object, got " + datum.print(), query_state_t::FAILED);
    }

    ql::datum_t read = datum.get_field("read", ql::NOTHROW);
    if (read.has()) {
        keys.erase(datum_string_t("read"));

        if (read.get_type() == ql::datum_t::R_BOOL) {
            set_read(read.as_bool());
        } else if (read.get_type() == ql::datum_t::R_NULL) {
            set_read(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `read`, got " + read.print(),
                query_state_t::FAILED);
        }
    }

    ql::datum_t write = datum.get_field("write", ql::NOTHROW);
    if (write.has()) {
        keys.erase(datum_string_t("write"));

        if (write.get_type() == ql::datum_t::R_BOOL) {
            set_write(write.as_bool());
        } else if (write.get_type() == ql::datum_t::R_NULL) {
            set_write(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `write`, got " + write.print(),
                query_state_t::FAILED);
        }
    }

    ql::datum_t config = datum.get_field("config", ql::NOTHROW);
    if (config.has()) {
        keys.erase(datum_string_t("config"));

        if (config.get_type() == ql::datum_t::R_BOOL) {
            set_config(config.as_bool());
        } else if (config.get_type() == ql::datum_t::R_NULL) {
            set_config(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `config`, got " + config.print(),
                query_state_t::FAILED);
        }
    }

    ql::datum_t connect = datum.get_field("connect", ql::NOTHROW);
    if (connect.has()) {
        if (static_cast<bool>(m_connect)) {
            keys.erase(datum_string_t("connect"));

            if (connect.get_type() == ql::datum_t::R_BOOL) {
                set_connect(connect.as_bool());
            } else if (connect.get_type() == ql::datum_t::R_NULL) {
                set_connect(boost::indeterminate);
            } else {
                throw admin_op_exc_t(
                    "Expected a boolean or null for `connect`, got " + connect.print(),
                    query_state_t::FAILED);
            }
        } else {
            throw admin_op_exc_t(
                "The `connect` permission is only valid at the global scope",
                query_state_t::FAILED);
        }
    }

    if (!keys.empty()) {
        throw admin_op_exc_t(
            "Unexpected keys, got " + datum.print(), query_state_t::FAILED);
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
    guarantee(static_cast<bool>(m_connect));
    return *m_connect;
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

void permissions_t::to_datum(
        ql::datum_object_builder_t *datum_object_builder_out) const {
    if (!indeterminate(get_read())) {
        datum_object_builder_out->overwrite(
            "read", ql::datum_t::boolean(get_read()));
    }

    if (!indeterminate(get_write())) {
        datum_object_builder_out->overwrite(
            "write", ql::datum_t::boolean(get_write()));
    }

    if (!indeterminate(get_config())) {
        datum_object_builder_out->overwrite(
            "config", ql::datum_t::boolean(get_config()));
    }

    if (static_cast<bool>(m_connect) && !(indeterminate(get_connect()))) {
        datum_object_builder_out->overwrite(
            "connect", ql::datum_t::boolean(get_connect()));
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
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(permissions_t);

}  // namespace auth
