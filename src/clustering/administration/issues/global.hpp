// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_

#include <vector>
#include <set>
#include <string>

#include "containers/scoped.hpp"
#include "rpc/semilattice/view.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_string.hpp"

class issue_multiplexer_t;
class cluster_semilattice_metadata_t;

class issue_t {
public:
    explicit issue_t(const issue_id_t &_issue_id) : issue_id(_issue_id) { }
    virtual ~issue_t() { }

    // to_datum has to generate info and description data, which depends on the
    // current cluster configuration
    typedef cluster_semilattice_metadata_t metadata_t;
    void to_datum(const metadata_t &metadata, ql::datum_t *datum_out) const;

    virtual bool is_critical() const = 0;
    virtual const datum_string_t &get_name() const = 0;
    virtual issue_id_t get_id() const { return issue_id; }

    static const std::string get_server_name(const metadata_t &metadata,
                                             const machine_id_t &server_id);

protected:
    virtual void build_info_and_description(const metadata_t &metadata,
                                            ql::datum_t *info_out,
                                            datum_string_t *desc_out) const = 0;
private:
    const issue_id_t issue_id;
    DISABLE_COPYING(issue_t);
};

class issue_tracker_t {
public:
    explicit issue_tracker_t(issue_multiplexer_t *_parent);
    virtual ~issue_tracker_t();

    virtual std::vector<scoped_ptr_t<issue_t> > get_issues() const = 0;

private:
    issue_multiplexer_t *parent;
    DISABLE_COPYING(issue_tracker_t);
};

class issue_multiplexer_t : public home_thread_mixin_t {
public:
    issue_multiplexer_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view);

    void get_issue_ids(std::vector<ql::datum_t> *ids_out) const;
    void get_issue(const issue_id_t &issue_id,
                   ql::datum_t *row_out) const;

private:
    std::vector<scoped_ptr_t<issue_t> > all_issues() const;

    friend class issue_tracker_t;
    std::set<issue_tracker_t *> trackers;
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > cluster_sl_view;
    DISABLE_COPYING(issue_multiplexer_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_ */
