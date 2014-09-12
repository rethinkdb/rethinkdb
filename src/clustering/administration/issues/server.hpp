// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_

#include <vector>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

class server_down_issue_t : public issue_t {
public:
    explicit server_down_issue_t(const machine_id_t &_down_server_id,
                                 const std::vector<machine_id_t> &_affected_server_ids);
    const datum_string_t &get_name() const { return server_down_issue_type; }
    bool is_critical() const { return true; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    static const datum_string_t server_down_issue_type;
    static const issue_id_t base_issue_id;
    const machine_id_t down_server_id;
    const std::vector<machine_id_t> affected_server_ids;
};

class server_ghost_issue_t : public issue_t {
public:
    explicit server_ghost_issue_t(const machine_id_t &_ghost_server_id,
                                  const std::vector<machine_id_t> &_affected_server_ids);
    const datum_string_t &get_name() const { return server_ghost_issue_type; }
    bool is_critical() const { return false; }

private:
    void build_info_and_description(const metadata_t &metadata,
                                    ql::datum_t *info_out,
                                    datum_string_t *desc_out) const;

    static const datum_string_t server_ghost_issue_type;
    static const issue_id_t base_issue_id;
    const machine_id_t ghost_server_id;
    const std::vector<machine_id_t> affected_server_ids;
};

class server_issue_tracker_t : public local_issue_tracker_t {
public:
    server_issue_tracker_t(
        local_issue_aggregator_t *_parent,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
            &_machine_to_peer);
    ~server_issue_tracker_t();

private:
    void recompute();

    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
        cluster_sl_view;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
        machine_to_peer;
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::subscription_t
        machine_to_peer_subs;

    DISABLE_COPYING(server_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_SERVER_HPP_ */
