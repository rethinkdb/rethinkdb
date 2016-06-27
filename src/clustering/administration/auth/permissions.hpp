// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP

#include <string>

#include "errors.hpp"
#include <boost/logic/tribool.hpp>

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
        boost::tribool read,
        boost::tribool write,
        boost::tribool config);
    permissions_t(
        boost::tribool read,
        boost::tribool write,
        boost::tribool config,
        boost::tribool connect);
    permissions_t(ql::datum_t const &datum, bool global);

    void merge(ql::datum_t const &datum);

    boost::tribool get_read() const;
    boost::tribool get_write() const;
    boost::tribool get_config() const;
    boost::tribool get_connect() const;

    bool is_indeterminate() const;

    void set_read(boost::tribool);
    void set_write(boost::tribool);
    void set_config(boost::tribool);
    void set_connect(boost::tribool);

    ql::datum_t to_datum() const;

    bool operator==(permissions_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(permissions_t);

private:
    boost::tribool m_read;
    boost::tribool m_write;
    boost::tribool m_config;

    // The `connect` permission is only available at the global scope
    boost::optional<boost::tribool> m_connect;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_PERMISSIONS_HPP
