// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP

#include "containers/optional.hpp"
#include "containers/tribool.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/serialize_macros.hpp"

namespace auth {

class permissions_t
{
public:
    permissions_t();
    // The two constructors below are for the non-global and global scopes
    // respectively
    permissions_t(
        tribool read,
        tribool write,
        tribool config);
    permissions_t(
        tribool read,
        tribool write,
        tribool config,
        tribool connect);
    permissions_t(ql::datum_t const &datum, bool global);

    void merge(ql::datum_t const &datum);

    tribool get_read() const;
    tribool get_write() const;
    tribool get_config() const;
    tribool get_connect() const;

    bool is_indeterminate() const;

    void set_read(tribool);
    void set_write(tribool);
    void set_config(tribool);
    void set_connect(tribool);

    ql::datum_t to_datum() const;

    bool operator<(permissions_t const &rhs) const;
    bool operator==(permissions_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(permissions_t);

private:
    std::tuple<int8_t, int8_t, int8_t, optional<int8_t>> to_tuple() const;

    tribool m_read;
    tribool m_write;
    tribool m_config;

    // The `connect` permission is only available at the global scope
    optional<tribool> m_connect;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP
