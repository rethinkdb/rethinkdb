// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/local.hpp"

#include "utils.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "clustering/administration/issues/local_issue_aggregator.hpp"

local_issue_t::local_issue_t() : issue_t(nil_uuid()) { }

local_issue_t::local_issue_t(const issue_id_t &id) :
    issue_t(id) { }

local_issue_t::~local_issue_t() { }

void local_issue_t::add_server(const server_id_t &server) {
    affected_server_ids.push_back(server);
}

