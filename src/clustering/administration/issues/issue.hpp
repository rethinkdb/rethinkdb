// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_

#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_string.hpp"

class cluster_semilattice_metadata_t;
class issue_multiplexer_t;
class name_string_t;
class server_config_client_t;
class table_meta_client_t;

class issue_t {
public:
    explicit issue_t(const issue_id_t &_issue_id) : issue_id(_issue_id) { }
    virtual ~issue_t() { }

    /* `to_datum()` has to generate info and description data, which depends on the
    current cluster configuration, so it takes the cluster configuration as a parameter.
    Based on the cluster configuration, it may decide that the issue is no longer
    relevant, in which case `to_datum()` will return `false`. */
    typedef cluster_semilattice_metadata_t metadata_t;
    bool to_datum(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *datum_out) const;

    virtual bool is_critical() const = 0;
    virtual const datum_string_t &get_name() const = 0;
    issue_id_t get_id() const { return issue_id; }

    // Utility function for generating deterministic UUIDs via a hash of the args
    template<class... Args>
    static uuid_u from_hash(const uuid_u &base, const Args&... args) {
        return uuid_u::from_hash(base, concat({ item_to_str(args)... }));
    }

    issue_id_t issue_id;

protected:
    /* `build_info_and_description()` can return `false` if it decides based on the
    contents of the metadata that the issue is no longer relevant. */
    virtual bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const = 0;

private:
    template <typename T>
    static std::string concat_internal(T items) {
        return std::accumulate(items.begin(), items.end(), std::string(),
            [](const std::string &acc, typename T::value_type const &val) {
                return acc + item_to_str(val);
            });
    }

    template <typename T>
    static std::string concat(const std::initializer_list<T> &list) {
        return concat_internal(list);
    }

    template <typename T>
    static std::string item_to_str(const std::vector<T> &vec) {
        return concat_internal(vec);
    }

    static std::string item_to_str(const name_string_t &str);
    static std::string item_to_str(const std::string &str);
    static std::string item_to_str(const uuid_u &id);
};

class issue_tracker_t {
public:
    issue_tracker_t() { }
    virtual ~issue_tracker_t() { }
    virtual std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const = 0;
private:
    DISABLE_COPYING(issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_ */
