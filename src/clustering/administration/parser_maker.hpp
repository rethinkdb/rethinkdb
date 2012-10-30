// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_PARSER_MAKER_HPP_
#define CLUSTERING_ADMINISTRATION_PARSER_MAKER_HPP_

#include <string>

#include "errors.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/issue_subscription.hpp"

template<class protocol_t, class parser_t>
class parser_maker_t {
public:
    parser_maker_t(mailbox_manager_t *,
                   boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > >,
                   int port_offset,
                   namespace_repo_t<protocol_t> *repo,
                   local_issue_tracker_t *_local_issue_tracker,
                   perfmon_collection_repo_t *_perfmon_collection_repo);

private:
    class ns_record_t {
    public:
        explicit ns_record_t(int p) : port(p) { }
        int port;
        cond_t stopper;
    };

    void on_change();
    void serve_queries(std::string ns_name, namespace_id_t ns, int port, auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *mailbox_manager;
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > namespaces_semilattice_metadata;
    int port_offset;
    namespace_repo_t<protocol_t> *repo;

    boost::ptr_map<namespace_id_t, ns_record_t> namespaces_being_handled;

    auto_drainer_t drainer;

    typename semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > >::subscription_t namespaces_subscription;
    perfmon_collection_repo_t *perfmon_collection_repo;

    local_issue_tracker_t *local_issue_tracker;
};

#include "clustering/administration/parser_maker.tcc"

#endif /* CLUSTERING_ADMINISTRATION_PARSER_MAKER_HPP_ */
