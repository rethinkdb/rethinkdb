// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/global_permissions.hpp"

#include "clustering/administration/admin_op_exc.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

global_permissions_t::global_permissions_t() {
}

global_permissions_t::global_permissions_t(
        boost::tribool read,
        boost::tribool write,
        boost::tribool config,
        boost::tribool connect)
    : m_permissions(read, write, config),
      m_connect(connect) {
}

global_permissions_t::global_permissions_t(ql::datum_t const &datum)
    : m_permissions(datum) {
    // FIXME, check datum is an object?

    ql::datum_t connect = datum.get_field("$connect", ql::NOTHROW);
    if (connect.has()) {
        if (connect.get_type() == ql::datum_t::R_BOOL) {
            set_connect(connect.as_bool());
        } else {
            throw admin_op_exc_t(
                "Expected a boolean for `$connect`, got " + connect.print(),
                query_state_t::FAILED);
        }
    } else {
        set_connect(boost::indeterminate);
    }
}

boost::tribool global_permissions_t::get_read() const {
    return m_permissions.get_read();
}

boost::tribool global_permissions_t::get_write() const {
    return m_permissions.get_write();
}

boost::tribool global_permissions_t::get_config() const {
    return m_permissions.get_config();
}

boost::tribool global_permissions_t::get_connect() const {
    return m_connect;
}

void global_permissions_t::set_read(boost::tribool read) {
    m_permissions.set_read(read);
}

void global_permissions_t::set_write(boost::tribool write) {
    m_permissions.set_write(write);
}

void global_permissions_t::set_config(boost::tribool config) {
    m_permissions.set_config(config);
}

void global_permissions_t::set_connect(boost::tribool connect) {
    m_connect = connect;
}

void global_permissions_t::to_datum(
        ql::datum_object_builder_t *datum_object_builder_out) const {
    m_permissions.to_datum(datum_object_builder_out);

    if (!indeterminate(get_connect())) {
        datum_object_builder_out->overwrite(
            "$connect", ql::datum_t::boolean(get_connect()));
    }
}

bool global_permissions_t::operator==(global_permissions_t const &rhs) const {
    return
        m_permissions == rhs.m_permissions &&
        m_connect == rhs.m_connect;
}

RDB_IMPL_SERIALIZABLE_2(global_permissions_t, m_permissions, m_connect);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(global_permissions_t);

}  // namespace auth
