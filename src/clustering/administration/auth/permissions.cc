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
      m_config(config) {
}

permissions_t::permissions_t(ql::datum_t const &datum) {
    if (datum.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object, got " + datum.print(), query_state_t::FAILED);
    }

    ql::datum_t read = datum.get_field("$read", ql::NOTHROW);
    if (read.has()) {
        if (read.get_type() == ql::datum_t::R_BOOL) {
            set_read(read.as_bool());
        } else if (read.get_type() == ql::datum_t::R_NULL) {
            set_read(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `$read`, got " + read.print(),
                query_state_t::FAILED);
        }
    } else {
        set_read(boost::indeterminate);
    }

    ql::datum_t write = datum.get_field("$write", ql::NOTHROW);
    if (write.has()) {
        if (write.get_type() == ql::datum_t::R_BOOL) {
            set_write(write.as_bool());
        } else if (write.get_type() == ql::datum_t::R_NULL) {
            set_write(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `$write`, got " + write.print(),
                query_state_t::FAILED);
        }
    } else {
        set_write(boost::indeterminate);
    }

    ql::datum_t config = datum.get_field("$config", ql::NOTHROW);
    if (config.has()) {
        if (config.get_type() == ql::datum_t::R_BOOL) {
            set_config(config.as_bool());
        } else if (config.get_type() == ql::datum_t::R_NULL) {
            set_config(boost::indeterminate);
        } else {
            throw admin_op_exc_t(
                "Expected a boolean or null for `$config`, got " + config.print(),
                query_state_t::FAILED);
        }
    } else {
        set_config(boost::indeterminate);
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

void permissions_t::set_read(boost::tribool read) {
    m_read = read;
}

void permissions_t::set_write(boost::tribool write) {
    m_write = write;
}

void permissions_t::set_config(boost::tribool config) {
    m_config = config;
}

void permissions_t::to_datum(
        ql::datum_object_builder_t *datum_object_builder_out) const {
    if (!indeterminate(get_read())) {
        datum_object_builder_out->overwrite(
            "$read", ql::datum_t::boolean(get_read()));
    }
    if (!indeterminate(get_write())) {
        datum_object_builder_out->overwrite(
            "$write", ql::datum_t::boolean(get_write()));
    }
    if (!indeterminate(get_config())) {
        datum_object_builder_out->overwrite(
            "$config", ql::datum_t::boolean(get_config()));
    }
}

bool permissions_t::operator==(permissions_t const &rhs) const {
    return
        m_read == rhs.m_read &&
        m_write == rhs.m_write &&
        m_config == rhs.m_config;
}

RDB_IMPL_SERIALIZABLE_3(permissions_t, m_read, m_write, m_config);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(permissions_t);

}  // namespace auth
