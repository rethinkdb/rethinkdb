// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_

#include <vector>
#include <set>
#include <string>

#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_string.hpp"

class name_string_t;
class issue_multiplexer_t;
class cluster_semilattice_metadata_t;

class issue_t {
public:
    explicit issue_t(const issue_id_t &_issue_id) : issue_id(_issue_id) { }
    virtual ~issue_t() { }

    /* `to_datum()` has to generate info and description data, which depends on the
    current cluster configuration, so it takes the cluster configuration as a parameter.
    Based on the cluster configuration, it may decide that the issue is no longer
    relevant, in which case `to_datum()` will return `false`. */
    typedef cluster_semilattice_metadata_t metadata_t;
    bool to_datum(const metadata_t &metadata, ql::datum_t *datum_out) const;

    virtual bool is_critical() const = 0;
    virtual const datum_string_t &get_name() const = 0;
    issue_id_t get_id() const { return issue_id; }

    static const std::string get_server_name(const metadata_t &metadata,
                                             const server_id_t &server_id);

    // Utility function for generating deterministic UUIDs via a hash of the args
    template<class... Args>
    static uuid_u from_hash(const uuid_u &base, const Args&... args) {
        return uuid_u::from_hash(base, concat(args...));
    }

    issue_id_t issue_id;

protected:
    /* Note that `build_info()` can return an empty `ql::datum_t` if it decided that the
    issue is no longer relevant based on the contents of the metadata. If it returns an
    empty `ql::datum_t`, the user will not see the issue. */
    virtual ql::datum_t build_info(const metadata_t &metadata) const = 0;
    virtual datum_string_t build_description(const ql::datum_t &info) const = 0;

private:
    // Terminal concat call
    static std::string concat() { return std::string(""); }

    template <typename T, typename... Args>
    static std::string concat(const T& arg1, const Args&... args) {
        return item_to_str(arg1) + concat(args...);
    }

    template <typename T>
    static std::string item_to_str(const std::vector<T> &vec) {
        std::string res;
        for (auto const &item : vec) {
            res += item_to_str(item);
        }
        return res;
    }

    static std::string item_to_str(const name_string_t &str);
    static std::string item_to_str(const std::string &str);
    static std::string item_to_str(const uuid_u &id);
};

class issue_tracker_t {
public:
    issue_tracker_t() { }
    virtual ~issue_tracker_t() { }
    virtual std::vector<scoped_ptr_t<issue_t> > get_issues() const = 0;
private:
    DISABLE_COPYING(issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_ISSUE_HPP_ */
