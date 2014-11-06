// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/local_issue_aggregator.hpp"

#include "utils.hpp"
#include "containers/archive/stl_types.hpp"

RDB_IMPL_SERIALIZABLE_3(log_write_issue_t,
                        issue_id, affected_server_ids, message);
RDB_IMPL_EQUALITY_COMPARABLE_3(log_write_issue_t,
                               issue_id, affected_server_ids, message);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(log_write_issue_t);

RDB_IMPL_SERIALIZABLE_3(outdated_index_issue_t,
                        issue_id, affected_server_ids, indexes);
RDB_IMPL_EQUALITY_COMPARABLE_3(outdated_index_issue_t,
                               issue_id, affected_server_ids, indexes);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(outdated_index_issue_t);

RDB_IMPL_SERIALIZABLE_3(server_down_issue_t,
                        issue_id, affected_server_ids, down_server_id);
RDB_IMPL_EQUALITY_COMPARABLE_3(server_down_issue_t,
                               issue_id, affected_server_ids, down_server_id);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_down_issue_t);

RDB_IMPL_SERIALIZABLE_4(server_ghost_issue_t,
                        issue_id, ghost_server_id, hostname, pid);
RDB_IMPL_EQUALITY_COMPARABLE_4(server_ghost_issue_t,
                               issue_id, ghost_server_id, hostname, pid);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(server_ghost_issue_t);

RDB_IMPL_SERIALIZABLE_4(local_issues_t,
                        log_write_issues,
                        server_down_issues,
                        server_ghost_issues,
                        outdated_index_issues);
RDB_IMPL_EQUALITY_COMPARABLE_4(local_issues_t,
                               log_write_issues,
                               server_down_issues,
                               server_ghost_issues,
                               outdated_index_issues);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(local_issues_t);

local_issue_aggregator_t::local_issue_aggregator_t() :
    issues_watchable(local_issues_t()) { }

clone_ptr_t<watchable_t<local_issues_t> >
local_issue_aggregator_t::get_issues_watchable() {
    assert_thread();
    return issues_watchable.get_watchable();
}
